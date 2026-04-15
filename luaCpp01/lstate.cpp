#include "lstate.h"
#include "lmem.h"

#include <cassert>

typedef struct LX {
	lu_byte extra_[LUA_EXTRASPACE];
	lua_State l;
}LX;

typedef struct LG {
	LX l;
	global_State g;
}LG;

lua_State* lua_newstate(lua_Alloc alloc, void* ud)
{
	struct global_State* g;	

	struct LG* lg = (struct LG*)(*alloc)(ud, nullptr, LUA_TTHREAD, sizeof(struct LG));

	if (!lg) {
		return {};
	}

	g = &lg->g;
	g->ud = ud;
	g->freealloc = alloc;
	g->panic = {};

#if 0
	L = &lg->l.l;
#else
	lua_State* L = new (&lg->l.l)lua_State();//placement new
#endif

	G(L) = g;
	g->mainthread = L;
	L->stack_init();
	return L;
}

#define fromstate(L) (cast(LX*, cast(lu_byte*,(L))- offsetof(LX,l)))
void lua_close(lua_State* L)
{
	struct global_State* g = G(L);
	lua_State* L1 = g->mainthread;// only mainthread can be close

	// because I have not implement gc, so we should free ci manual 
	struct CallInfo* ci = &L1->base_ci;
	while (ci->next) {
		struct CallInfo* next = ci->next->next;
		struct CallInfo* free_ci = ci->next;

		(*g->freealloc)(g->ud, free_ci, sizeof(struct CallInfo), 0);
		ci = next;
	}
	L1->free_stack();
	(*g->freealloc)(g->ud, fromstate(L1), sizeof(LG), 0);
}

void lua_State::free_stack()
{
	global_State* g = G(this);
	(*g->freealloc)(g->ud, stack, sizeof(TValue), 0);
	stack = stack_last = top = {};
	stack_size = 0;
}

void lua_State::stack_init()
{
	stack = (StkId)luaM_realloc(this, nullptr, 0, LUA_STACKSIZE * sizeof(TValue));	
	stack_size = LUA_STACKSIZE;
	stack_last = stack + LUA_STACKSIZE - LUA_EXTRASTACK;
	next = previous = {};
	status = LUA_OK;
	errorjmp = nullptr;
	top = stack;
	errorfunc = 0;
	ncalls = 0;// 调用次数初始化

	debug_info(__PRETTY_FUNCTION__, "&top", top);
	debug_info(__PRETTY_FUNCTION__, "&stack", stack);

	for (int i = 0; i < stack_size; ++i) {
		(stack + i)->setnilvalue();
	}
	++top;
	debug_info(__PRETTY_FUNCTION__, "&top", top);

	ci = &base_ci;
	ci->func = stack;
	ci->top = stack + LUA_MINSTACK;
	ci->previous = ci->next = NULL;
}

lua_State::lua_State()
{
	//stack_init();
}

lua_State::~lua_State()
{
	lua_close(this);
}


////////////////////////////////////////////////////////////////////////
/// Lua Stack Operation
////////////////////////////////////////////////////////////////////////
void lua_State::increase_top()
{
	++top;
	debug_info(__PRETTY_FUNCTION__, "&top", top);
	assert(top <= stack_last);
}

void lua_State::lua_pushcfunction(lua_CFunction f)
{
	top->setfvalue(f);
	increase_top();
}

void lua_State::lua_pushinteger(int integer)
{
	top->setivalue(integer);
	increase_top();
}

void lua_State::lua_pushnumber(float number)
{
	top->setfltvalue(number);
	increase_top();
}

void lua_State::lua_pushboolean(bool b)
{
	top->setbvalue(b);
	increase_top();
}

void lua_State::lua_pushnil()
{
	top->setnilvalue();
	increase_top();
}

void lua_State::lua_pushlightuserdata(void* p)
{
	top->setpvalue(p);
	increase_top();
}

TValue* lua_State::index2addr(int idx)
{
	if (idx >= 0) {
		assert(ci->func + idx < ci->top);
		return ci->func + idx;
	}
	else {
		assert(top + idx > ci->func);
		return top + idx;
	}
}

lua_Integer lua_State::lua_tointegerx(int idx, int* isnum)
{
	lua_Integer ret = 0;
	TValue* addr = index2addr(idx);
	if (addr->tt_ == LUA_NUMINT) {
		ret = addr->value_.i;
		*isnum = 1;
	}
	else {
		*isnum = 0;
		LUA_ERROR(L, "can not convert to integer\n");
	}

	return ret;
}

lua_Number lua_State::lua_tonumberx(int idx, int* isnum)
{
	lua_Integer ret = 0;
	TValue* addr = index2addr(idx);
	if (addr->tt_ == LUA_NUMFLT) {
		ret = addr->value_.n;
		*isnum = 1;
	}
	else {
		*isnum = 0;
		LUA_ERROR(L, "can not convert to number\n");
	}

	return ret;
}

bool lua_State::lua_toboolean(int idx)
{
	TValue* addr = index2addr(idx);
	return !(addr->tt_ == LUA_TNIL || addr->value_.b);
}

int lua_State::lua_isnil(int idx)
{
	TValue* addr = index2addr(idx);
	return addr->tt_ == LUA_TNIL;
}

void lua_State::lua_settop(int idx)
{
	StkId func = ci->func;
	if (idx >= 0) {
		assert(idx <= stack_last - (func + 1));
		while (top < (func + 1) + idx) {			
			(top++)->setnilvalue();
		}
		top = func + 1 + idx;
	}
	else {
		assert(top + idx > func);
		top = top + idx;
	}
}

int lua_State::lua_gettop()
{
	return cast(int, top - (ci->func + 1));
}

void lua_State::lua_pop()
{
	lua_settop(-1);
	debug_info(__PRETTY_FUNCTION__, "&top", top);
}


 