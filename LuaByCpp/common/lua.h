#pragma once

#include <cstdarg>
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <string>

#if defined(LLONG_MAX)
using LUA_INTEGER = long;
using LUA_NUMBER = double;
#else
using LUA_INTEGER = int;
using LUA_NUMBER = float;
#endif

// ERROR CODE
enum ErrorCode
{
	Lua_Ok = 0,
	Lua_ErrErr = 1,
	Lua_ErrMem = 2,
	Lua_ErrRun = 3,
};

// basic object type
enum ObjectType {
	Lua_TNumber = 1,
	Lua_TLightUserData = 2,
	Lua_TBoolean = 3,
	Lua_TString = 4,
	Lua_TNil = 5,
	Lua_TTable = 6,
	Lua_TFunction = 7,
	Lua_TThread = 8,
	Lua_TNone
};

// stack define
constexpr auto LUA_MINSTACK = 20;
constexpr auto LUA_STACKSIZE = 2 * LUA_MINSTACK;
constexpr auto LUA_EXTRASTACK = 5;
constexpr auto LUA_MAXSTACK = 15000;
constexpr auto LUA_ERRORSTACK = 200;
constexpr auto LUA_MULRET = -1;// 多返回值
constexpr auto LUA_MAXCALLS = 200;// 最大调用次数


// error tips
inline void Lua_Error(std::string const& error) {
	printf("LUA ERROR:%s\n", error.c_str());
}

inline void debug_info(std::string const&func, std::string const& info, void* p) {
	printf("[%s] %s = %p\n", func.c_str(), info.c_str(), p);
}