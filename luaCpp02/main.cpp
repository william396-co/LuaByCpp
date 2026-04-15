#include <iostream>
#include "luaaux.h"
#include "lstate.h"
#include "lgc.h"


#define ELEMENTNUM 5
void p2_test_main()
{
	lua_State* L = luaL_newstate();

	for (int i = 0; i < ELEMENTNUM; ++i) {
		luaL_pushnil(L);
	}

	time_t start_time = time(NULL);

	size_t max_bytes = 0;
	struct global_State* g = G(L);
	for (int i = 0; i < 500000000; ++i) {
		TValue* o = luaL_index2addr(L, (i % ELEMENTNUM) + 1);
		struct GCObject* gco = L->luaC_newobj(LUA_TSTRING, sizeof(TString));
		o->value_.gc = gco;
		o->tt_ = LUA_TSTRING;
		luaC_checkgc(L);

		if ((g->totalbytes + g->GCdebt) > max_bytes) {
			max_bytes = g->totalbytes + g->GCdebt;
		}
		if (i % 1000 == 0) {
			printf("timestamp:%d totalbytes:%f kb \n", (int)time(NULL), (float)(g->totalbytes + g->GCdebt) / 1024.0f);
		}

	}
	time_t end_time = time(NULL);
	printf("finish test start_time:%I64d end_time:%I64d max_bytes:%f kb \n", start_time, end_time, (float)max_bytes / 1024.0f);

	luaL_close(L);
}


int main() {
	p2_test_main();

	return 0;
}