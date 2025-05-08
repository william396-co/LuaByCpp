#pragma once

#include "lua.h"

#include <functional>
#include <variant>

class LuaState;

using Lua_Integer = LUA_INTEGER;
using Lua_Number = LUA_NUMBER;
using lua_byte = uint8_t;
using Lua_CFunction = std::function<int(LuaState*)>;
using Lua_Alloc = std::function<void* (void* ud, void* ptr, size_t osize, size_t nsize)>;

// lua number type
constexpr auto LUA_NUMINT = Lua_TNumber | (0 << 4);
constexpr auto LUA_NUMFLT = Lua_TNumber | (1 << 4);

// lua function
constexpr auto LUA_TLCL = Lua_TFunction | (0 << 4);
constexpr auto LUA_TLCF = Lua_TFunction | (1 << 4);
constexpr auto LUA_TCCL = Lua_TFunction | (2 << 4);

// string type
constexpr auto LUA_LNGSTR = Lua_TString | (0 << 4);
constexpr auto LUA_SHRSTR = Lua_TString | (1 << 4);


using Value = std::variant<
	bool,    // b boolean
	void*, // p light userdata
	Lua_Integer,// i
	Lua_Number,// n
	Lua_CFunction // f
>;

struct Lua_TValue {
	Value value_{};
	int tt_{};
};
using TValue = Lua_TValue;
