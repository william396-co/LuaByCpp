#include "luaaux.h"
#include "ldo.h"

static void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;

	//printf("l_alloc nsize:%lld\n",nsize);
	if (0 == nsize) {
		free(ptr);
		return NULL;
	}
	return realloc(ptr, nsize);
}

lua_State* luaL_newstate()
{
	return lua_newstate(&l_alloc, nullptr);
}

void luaL_close(lua_State* L)
{
	lua_close(L);
}

void luaL_pushinteger(lua_State* L, int integer)
{
	L->lua_pushinteger(integer);
}

void luaL_pushnumber(lua_State* L, float number)
{
	L->lua_pushnumber(number);
}

void luaL_pushlightuserdata(lua_State* L, void* userdata)
{
	L->lua_pushlightuserdata(userdata);
}

void luaL_pushnil(lua_State* L)
{
	L->lua_pushnil();
}

void luaL_pushcfunction(lua_State* L, lua_CFunction f)
{
	L->lua_pushcfunction(f);
}

void luaL_pushboolean(lua_State* L, bool boolean)
{
	L->lua_pushboolean(boolean);
}

// function call
typedef struct Calls {
	StkId func;
	int nresult;
}Calls;

static int f_call(lua_State* L, void* ud) {
	Calls* c = cast(Calls*, ud);
	L->luaD_call(c->func, c->nresult);
	return LUA_OK;
}

int luaL_pcall(lua_State* L, int narg, int nresults)
{
	int status = LUA_OK;
	Calls c;
	c.func = L->top - (narg + 1);
	c.nresult = nresults;

	status = L->luaD_pcall(&f_call, &c, savestack(L, L->top), 0);
	return status;
}

bool luaL_checkinteger(lua_State* L, int idx)
{
	int isnum = 0;
	L->lua_tointegerx(idx, &isnum);
	if (isnum) {
		return true;
	}
	return false;
}

lua_Integer luaL_tointeger(lua_State* L, int idx)
{
	int isnum = 0;
	lua_Integer ret = L->lua_tointegerx(idx, &isnum);
	return ret;
}

lua_Number luaL_tonumber(lua_State* L, int idx)
{
	int isnum = 0;
	lua_Number ret = L->lua_tonumberx(idx, &isnum);
	return ret;
}

void* luaL_touserdata(lua_State* L, int idx)
{
	// TODO
	return NULL;
}

bool luaL_toboolean(lua_State* L, int idx)
{
	return L->lua_toboolean(idx);
}

int luaL_isnil(lua_State* L, int idx)
{
	return L->lua_isnil(idx);
}

TValue* luaL_index2addr(lua_State* L, int idx)
{
	return L->index2addr(idx);
}

void luaL_pop(lua_State* L)
{
	L->lua_pop();
}

int luaL_stacksize(lua_State* L)
{
	return L->lua_gettop();
}