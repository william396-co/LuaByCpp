#pragma once

#include "../common/luastate.h"

class LuaState* luaL_newstate();
void luaL_close(LuaState* L);

void luaL_pushinteger(LuaState* L, int integer);
void luaL_pushnumber(LuaState* L, float number);
void luaL_pushlightuserdata(LuaState* L, void* userdata);
void luaL_pushnil(LuaState* L);
void luaL_pushcfunction(LuaState* L, Lua_CFunction f);
void luaL_pushboolean(LuaState* L, bool boolean);
int luaL_pcall(LuaState* L, int narg, int nresults);


bool luaL_checkinteger(LuaState* L, int idx);
Lua_Integer luaL_tointeger(LuaState* L, int idx);
Lua_Number luaL_tonumber(LuaState* L, int idx);
void* luaL_touserdata(LuaState* L, int idx);
bool luaL_toboolean(LuaState* L, int idx);
bool luaL_isnil(LuaState* L, int idx);

void luaL_pop(LuaState* L);
int luaL_stacksize(LuaState* L);