#include "luaaux.h"

LuaState* luaL_newstate()
{
	return new LuaState();
}

void luaL_close(LuaState* L)
{
	delete L;
}

void luaL_pushinteger(LuaState* L, int integer)
{
	L->Pushinteger(integer);
}

void luaL_pushnumber(LuaState* L, float number)
{
	L->Pushnumber(number);
}

void luaL_pushlightuserdata(LuaState* L, void* userdata)
{
	L->Pushlightuserdata(userdata);
}

void luaL_pushnil(LuaState* L)
{
	L->Pushnil();
}

void luaL_pushcfunction(LuaState* L, Lua_CFunction const& f)
{
	L->Pushcfunction(f);
}

void luaL_pushboolean(LuaState* L, bool boolean)
{
	L->Pushboolean(boolean);
}

// function call
struct Calls {
	StkId func{};
	int nresults{};
};
int f_call(LuaState* L, void* ud) {
	auto c = static_cast<Calls*>(ud);
	L->luaD_call(c->func, c->nresults);
	return ErrorCode::Lua_Ok;
}

int luaL_pcall(LuaState* L, int narg, int nresults)
{
	auto status = ErrorCode::Lua_Ok;
	Calls c{};
	c.func = L->GetStackTop() - (narg + 1);
	//status = L->luaD_call(&f_call, &c, L->Savestack(L->Gettop()), 0);
	return status;
}

bool luaL_checkinteger(LuaState* L, int idx)
{
	bool isOk{};
	L->ToIntegerx(idx, isOk);
	return isOk;
}

Lua_Integer luaL_tointeger(LuaState* L, int idx)
{
	bool isOk{};
	return L->ToIntegerx(idx,isOk);
}

Lua_Number luaL_tonumber(LuaState* L, int idx)
{
	bool isOk{};
	return L->ToNumberx(idx,isOk);
}

void* luaL_touserdata(LuaState* L, int idx)
{
	//TODO
	return nullptr;
}

bool luaL_toboolean(LuaState* L, int idx)
{
	return L->ToBoolean(idx);
}

bool luaL_isnil(LuaState* L, int idx)
{
	return L->IsNil(idx);
}

void luaL_pop(LuaState* L)
{
	L->Pop();
}

int luaL_stacksize(LuaState* L)
{
	return L->Gettop();
}

