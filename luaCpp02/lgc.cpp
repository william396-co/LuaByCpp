#include "lgc.h"
#include "lobject.h"
#include "lstate.h"
#include "lmem.h"
#include "lua.h"

#define GCMAXSWEEPGCO 25

#define gettotalbytes(g) (g->totalbytes + g->GCdebt)
#define white2gray(o) resetbits((o)->marked,WHITEBITS)
#define gray2black(o) l_setbit((o)->marked,BLACKBIT)
#define black2gray(o) resetbit((o)->marked,BLACKBIT)
#define sweepwholelist(L,list) L->sweeplist(list,MAX_LUMEM)

GCObject* lua_State::luaC_newobj(int tt_, size_t size)
{
    struct global_State* g = G(this);
    struct GCObject* obj = (struct GCObject*)luaM_realloc(this, nullptr, 0, size);
    obj->marked = luaC_white(g);
    obj->next = g->allgc;
    obj->tt_ = tt_;
    g->allgc = obj;
    return obj;
	return nullptr;
}

void lua_State::luaC_step()
{
    struct global_State* g = G(this);
    l_mem debt = get_debt();
    do {
        l_mem work = singlestep();
        debt -= work;
    } while (debt > -GCSTEPSIZE && G(this)->gcstate != GCSPause);

    if (G(this)->gcstate == GCSPause) {
        setpause();
    }
    else {
        debt = g->GCdebt / STEPMULADJ * g->GCstepmul;
        setdebt(debt);
    }
}

void lua_State::reallymarkobject(GCObject* gco)
{
    struct global_State* g = G(this);
    white2gray(gco);

    switch (gco->tt_)
    {
    case LUA_TTHREAD: {
        linkgclist(gco2th(gco), g->gray);
        break;
    }
    case LUA_TSTRING: {// jsut for gc test now
        gray2black(gco);
        g->GCmemtrav += sizeof(struct TString);
        break;
    }
    default:
        break;
    }
}

void lua_State::luaC_freeallobjects()
{
    struct global_State* g = G(this);
    g->currentwhite = WHITEBITS;// all gc objects must reclaim
    sweepwholelist(this, &g->allgc);
}

l_mem lua_State::get_debt()
{
    struct global_State* g = G(this);
    int stepmul = g->GCstepmul;
    l_mem debt = g->GCdebt;
    if (debt <= 0)
    {
        return 0;
    }
    debt = debt / STEPMULADJ + 1;
    debt = debt > (MAX_LMEM / STEPMULADJ) ? MAX_LMEM : debt * stepmul;
    return debt;
}

void lua_State::setdebt(l_mem debt)
{
    struct global_State* g = G(this);
    lu_mem  totalbytes = gettotalbytes(g);
    g->totalbytes = totalbytes - debt;
    g->GCdebt = debt;
}

void lua_State::setpause()
{
    struct global_State* g = G(this);
    l_mem estimate = g->GCestimate / GCPAUSE;
    estimate = (estimate * g->GCstepmul) >= MAX_LMEM ? MAX_LMEM : estimate * g->GCstepmul;

    l_mem debt = g->GCestimate - estimate;
    setdebt(debt);
}

lu_mem lua_State::singlestep()
{
    struct global_State* g = G(this);
    g->GCmemtrav = 0;
    switch (g->gcstate)
    {
    case GCSPause: {
        restart_collection();  // markroot阶段，将lua_State标记为灰色，并push到gray列表中
        g->gcstate = GCSPropagate;
        break;
    }
    case GCSPropagate:
    {
        propagatemark();// 从gray列表中，pop出一个对象，并标记为黑色，同时扫描其关联对象，设置为灰色并放入gray列表。
        if (g->gray == NULL) {
            g->gcstate = GCSatomic;
        }
        break;
    }
    case GCSatomic: {
        if (g->gray) {
            propagateall();// 一次性将gray列表中所有的对象标记和扫描
        }
        atomic();// 一次性将grayagain链表中所有的对象标记和扫描
        entersweep();
        g->GCestimate = gettotalbytes(g);
        break;
    }
    case GCSsweepallgc: {
        sweepstep();
        break;
    }
    case GCSsweepend: {
        g->gcstate = GCSPause;
        break;
    }
    default:
        break;
    }
    return g->GCmemtrav;
}

// mark root
void lua_State::restart_collection()
{
    struct global_State* g = G(this);
    g->gray = g->graygain = NULL;
    markobject(this, g->mainthread);
}

void lua_State::propagatemark()
{
    struct global_State* g = G(this);
    if (!g->gray) {
        return;
    }

    struct GCObject* gco = g->gray;
    gray2black(gco);
    lu_mem size = 0;

    switch (gco->tt_)
    {
    case LUA_TTHREAD:
    {
        black2gray(gco);
        lua_State* th = gco2th(gco);
        g->gray = th->gclist;
        linkgclist(th, g->graygain);
        size = traversethread(th);
        break;
    }
    default:
        break;
    }
    g->GCmemtrav += size;
}

lu_mem lua_State::traversethread(lua_State* th)
{
    for (TValue* o = th->stack; o < th->top; ++o) {
        markvalue(this, o);
    }

    return sizeof(lua_State) + sizeof(TValue) * th->stack_size
        + sizeof(struct CallInfo) * th->nci;
}

void lua_State::propagateall()
{
    struct global_State* g = G(this);
    while (g->gray) {
        propagateall();
    }
}

void lua_State::atomic()
{
    struct global_State* g = G(this);
    g->gray = g->graygain;
    g->graygain = NULL;

    g->gcstate = GCSinsideatomic;
    propagateall();
    g->currentwhite = cast(lu_byte, otherwhite(g));
}

void lua_State::entersweep()
{
    struct global_State* g = G(this);
    g->gcstate = GCSsweepallgc;
    g->sweepgc = sweeplist(&g->allgc, 1);
}

void lua_State::sweepstep()
{
    struct global_State* g = G(this);
    if (g->sweepgc) {
        g->sweepgc = sweeplist(g->sweepgc, GCMAXSWEEPGCO);
        g->GCestimate = gettotalbytes(g);

        if (g->sweepgc)
            return;
    }
    g->gcstate = GCSsweepend;
    g->sweepgc = {};
}

GCObject** lua_State::sweeplist(GCObject** p, size_t count)
{
    struct global_State* g = G(this);
    lu_byte ow = otherwhite(g);
    while (*p != NULL && count > 0) {
        lu_byte marked = (*p)->marked;
        if (isdeadm(ow, marked)) {
            struct GCObject* gco = *p;
            *p = (*p)->next;
            g->GCmemtrav += freeobj(gco);
        }
        else {
            (*p)->marked &= cast(lu_byte, ~(bitmask(BLACKBIT) | WHITEBITS));
            (*p)->marked |= luaC_white(g);
            p = &((*p)->next);
        }
        --count;
    }
    return (*p) == NULL ? NULL : p;
}

lu_mem lua_State::freeobj(GCObject* gco)
{
    switch (gco->tt_)
    {
    case LUA_TSTRING:
    {
        lu_mem sz = sizeof(TString);
        luaM_free(this, gco, sz);
        return sz;
    }
    default: {
        lua_assert(0);
        break;
    }
    }
    return 0;
}
