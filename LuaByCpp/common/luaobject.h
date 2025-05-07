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

// 获取Value对应类型的值
template<typename T>
inline T GetValue2T(Value v) {
	return std::get<T>(v);
}

template<typename T>
inline void SetValue(Value v, T val) {
	if constexpr (std::is_same_v<T, bool>) {
		v.emplace<0>(val);
	}
	else if constexpr (std::is_same_v<T, void*>) {
		v.emplace<1>(val);
	}
	else if constexpr (std::is_same_v<T, Lua_Integer>) {
		v.emplace<2>(val);
	}
	else if constexpr (std::is_same_v<T, Lua_Number>) {	
		v.emplace<3>(val);
	}
	else if constexpr (std::is_same_v<T, Lua_CFunction>) {
		v.emplace<4>(val);
	}
}


struct Lua_TValue {
	Value value_{};
	int tt_{};
};
using TValue = Lua_TValue;
