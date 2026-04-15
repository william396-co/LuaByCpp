#pragma once

#include "lstate.h"


lua_State* luaL_newstate();
void luaL_close(lua_State* L);


void luaL_pushinteger(lua_State* L, int integer);
void luaL_pushnumber(lua_State* L, float number);
void luaL_pushlightuserdata(lua_State* L, void* userdata);
void luaL_pushnil(lua_State* L);
void luaL_pushcfunction(lua_State* L, lua_CFunction f);
void luaL_pushboolean(lua_State* L, bool boolean);
int luaL_pcall(lua_State* L, int narg, int nresults);


bool luaL_checkinteger(lua_State* L, int idx);
lua_Integer luaL_tointeger(lua_State* L, int idx);
lua_Number luaL_tonumber(lua_State* L, int idx);
void* luaL_touserdata(lua_State* L, int idx);
bool luaL_toboolean(lua_State* L, int idx);
int luaL_isnil(lua_State* L, int idx);
TValue* luaL_index2addr(lua_State* L, int idx);

void luaL_pop(lua_State* L);
int luaL_stacksize(lua_State* L);