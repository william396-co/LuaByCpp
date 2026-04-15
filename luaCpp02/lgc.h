#pragma once

#include "lobject.h"

// GCState
#define GCSPause        0
#define GCSPropagate    1
#define GCSatomic       2
#define GCSinsideatomic 3
#define GCSsweepallgc   4
#define GCSsweepend     5

// Color
#define WHITE0BIT 0
#define WHITE1BIT 1
#define BLACKBIT  2

// Bit operation
#define bitmask(b) (1<<b)
#define bit2mask(b1,b2) (bitmask(b1)|bitmask(b2))

#define resetbits(x,m) ((x)&= cast(lu_byte,~(m)))
#define setbits(x,m) ((x) |= (m))
#define testbits(x,m) ((x) &(m))
#define resetbit(x,b) resetbits(x,bitmask(b))
#define l_setbit(x,b) setbits(x,bitmask(b))
#define testbit(x,b) testbits(x,bitmask(b))

#define WHITEBITS bit2mask(WHITE0BIT,WHITE1BIT)
#define luaC_white(g) (g->currentwhite & WHITEBITS)
#define otherwhite(g) (g->currentwhite ^ WHITEBITS)

#define iswhite(o) testbits((o)->marked,WHITEBITS)
#define isgray(o) (!testbits((o)->marked,bitmask(BLACKBIT) | WHITEBITS))
#define isblack(o) testbit((o)->marked,bitmask(BLACKBIT))
#define isdeadm(ow,m) (!((m ^ WHITEBITS)& (ow)))


#define obj2gco(o) (&cast(union GCUnion*,o)->gc)
#define gco2th(o) check_exp((o)->tt_ == LUA_TTHREAD, &cast(union GCUnion*,o)->th)
#define gcvalue(o) ((o)->value_.gc)

#define markobject(L,o)  if(iswhite(o)) { L->reallymarkobject(obj2gco(o));}
#define markvalue(L,o) if(iswhite(gcvalue(o))) { L->reallymarkobject(gcvalue(o));}
#define linkgclist(gco, prev) { (gco)->gclist = prev; prev = obj2gco(gco);}


// try trigger gc
#define luaC_condgc(pre,L,pos) if(G(L)->GCdebt >0) { pre; L->luaC_step(); pos;}
#define luaC_checkgc(L) luaC_condgc((void)0,L,(void)0)

#if 0

struct GCObject* luaC_newobj(lua_State* L, int tt_, size_t size);
void luaC_step(lua_State* L);
void reallymarkobject(lua_State* L, struct GCObject* gc);
void luaC_freeallobjects(lua_State* L);
#endif