#include "lmem.h"
#include "ldo.h"
#include "lstate.h"

#if 0
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
#else

void* lua_State::luaM_realloc(void* ptr, size_t osize, size_t nsize)
{
	struct global_State* g = G(this);
	int oldsize = ptr ? osize : 0;

	void* ret = (*g->freealloc)(g->ud, ptr, oldsize, nsize);
	if (!ret) {
		luaD_throw(LUA_ERRERR);
	}
	return ret;
}
#endif