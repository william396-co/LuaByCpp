#include "clib/luaaux.h"

#include "common/luaobject.h"
#include <iostream>

static int add_op(LuaState* L) {
	auto left = luaL_tointeger(L, -2);
	auto right = luaL_tointeger(L, -1);
	luaL_pushinteger(L, left + right);
	return 1;
}

void lua_call_example() {
	auto L = luaL_newstate();
		
	luaL_pushcfunction(L, &add_op);
	luaL_pushinteger(L, 5);// left
	luaL_pushinteger(L, 3);// right
	luaL_pcall(L, 2, 1);

	int result = luaL_tointeger(L, -1);
	std::cout << "result is " << result << "\n";
	luaL_pop(L);

	std::cout << "final stack size " << luaL_stacksize(L) << "\n";

	luaL_close(L);
}

int main() {

	lua_call_example();

	return 0;
}