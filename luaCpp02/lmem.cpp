#include "lmem.h"
#include "ldo.h"
#include "lstate.h"

void* luaM_realloc(lua_State* L, void* ptr, size_t osize, size_t nsize)
{
	struct global_State* g = G(L);
	int oldsize = ptr ? osize : 0;

	void* ret = (*g->freealloc)(g->ud, ptr, oldsize, nsize);
	if (!ret) {
		L->luaD_throw(LUA_ERRERR);
	}
	return ret;
}