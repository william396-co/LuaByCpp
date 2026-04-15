#pragma once

#include "lstate.h"

#define luaM_free(L,ptr,osize) luaM_realloc(L,ptr,osize,0)

void* luaM_realloc(lua_State* L, void* ptr, size_t osize, size_t nsize);