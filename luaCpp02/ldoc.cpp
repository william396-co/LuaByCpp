#include "lstate.h"
#include "lmem.h"
#include "lstate.h"

#define LUA_TRY(c,a) if(_setjmp((c)->b) ==0){ a }

#ifndef LINUX
#define LUA_THROW(c) longjmp((c)->b,1)
#else
#define LUA_THROW(c) _longjmp((c)->b,1)
#endif

struct lua_longjmp {
	struct lua_longjmp* previous;
	jmp_buf b;
	int status;
};


////////////////////////////////////////////////////////////////////////
/// Lua vm Operation
////////////////////////////////////////////////////////////////////////
void lua_State::seterrobj(int error)
{
	lua_pushinteger(error);
}

void lua_State::luaD_checkstack(int need)
{
	if (top + need > stack_last) {
		luaD_growstack(need);
	}
}

void lua_State::luaD_growstack(int size)
{
	if (this->stack_size > LUA_MAXSTACK) {
		luaD_throw(LUA_ERRERR);
	}

	int stack_size = this->stack_size * 2;// double increase
	int need_size = cast(int, top - stack) + size + LUA_EXTRASTACK;
	if (stack_size < need_size) {
		stack_size = need_size;
	}

	TValue* old_stack = stack;
	stack = (TValue*)luaM_realloc(this, stack, stack_size, stack_size * sizeof(TValue));
	this->stack_size = stack_size;
	stack_last = stack + stack_size - LUA_EXTRASTACK;
	int top_diff = cast(int, top - old_stack);
	top = restorestack(this, top_diff);

	struct CallInfo* ci;
	ci = &base_ci;
	while (ci) {
		int func_diff = cast(int, ci->func - old_stack);
		int top_diff = cast(int, ci->top - old_stack);
		ci->func = restorestack(this, func_diff);
		ci->top = restorestack(this, top_diff);
		ci = ci->next;
	}
}

void lua_State::luaD_throw(int error)
{
	struct global_State* g = G(this);
	if (errorjmp) {
		errorjmp->status = error;
		LUA_THROW(errorjmp);
	}
	else {
		if (g->panic) {
			(*g->panic)(this);
		}
		abort();
	}
}

int lua_State::luaD_rawunprotected(Pfunc f, void* ud)
{
	int old_ncalls = ncalls;
	struct lua_longjmp lj;
	lj.previous = errorjmp;
	lj.status = LUA_OK;
	errorjmp = &lj;

	LUA_TRY(		
		errorjmp,
		(*f)(this, ud);
	)

	errorjmp = lj.previous;
	ncalls = old_ncalls;
	return lj.status;
}

// prepare for function call. 
// if we call a c function, just directly call it
// if we call a lua function, just prepare for call it
int lua_State::luaD_precall(StkId func, int nresult)
{
	switch (func->tt_)
	{
	case LUA_TLCF:
	{
		lua_CFunction f = func->value_.f;

		ptrdiff_t func_diff = savestack(this, func);
		luaD_checkstack(LUA_MINSTACK);
		func = restorestack(this, func_diff);

		next_ci(func, nresult);
		int n = (*f)(this);
		assert(ci->func + n < ci->top);
		luaD_poscall(top - n, n);
		return 1;
	}
	default:
		break;
	}
	return 0;
}

int lua_State::luaD_poscall(StkId first_result, int nresult)
{
	StkId func = ci->func;
	int nwant = ci->nresult;

	switch (nwant)
	{
	case 0: {
		top = ci->func;
		break;
	}
	case 1: {
		if (0 == nresult) {
			first_result->value_.p = NULL;
			first_result->tt_ = LUA_TNIL;
		}
		func->setobj(first_result);
		first_result->value_.p = NULL;
		first_result->tt_ = LUA_TNIL;

		top = func + nwant;
		debug_info(__PRETTY_FUNCTION__, "&top", this->top);
		break;
	}
	case LUA_MULRET: {
		int nres = cast(int, top - first_result);
		for (int i = 0; i < nres; ++i) {
			StkId current = first_result + i;
			(func +i)->setobj(current);
			current->value_.p = NULL;
			current->tt_ = LUA_TNIL;
		}
		top = func + nres;
		break;
	}
	default:
	{
		if (nwant > nresult) {
			for (int i = 0; i < nwant; ++i) {
				if (i < nresult) {
					StkId current = first_result + i;
					(func+i)->setobj(current);
					current->value_.p = NULL;
					current->tt_ = LUA_TNIL;
				}
				else {
					StkId stack = func + i;
					stack->tt_ = LUA_TNIL;
				}
			}
			top = func + nwant;
		}
		else {
			for (int i = 0; i < nresult; ++i) {
				if (i < nwant) {
					StkId current = first_result + i;
					(func+i)->setobj(current);
					current->value_.p = NULL;
					current->tt_ = LUA_TNIL;
				}
				else {
					StkId stack = func + i;
					stack->value_.p = NULL;
					stack->tt_ = LUA_TNIL;
				}
			}
			top = func + nresult;
		}
		break;
	}
	}

	struct CallInfo* ci = this->ci;
	this->ci = ci->previous;
	this->ci->next = {};

	// because we have not implement gc, so we should free ci manually
	struct global_State* g = G(this);
	(*g->freealloc)(g->ud, ci, sizeof(struct CallInfo), 0);

	return LUA_OK;
}

int lua_State::luaD_call(StkId func, int nresult)
{
	if (++ncalls > LUA_MAXCALLS) {
		luaD_throw(0);
	}
	if (!luaD_precall(func, nresult)) {
		// TODO luaV_execuate(L)
	}
	ncalls--;
	return LUA_OK;
}

int lua_State::luaD_pcall(Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef)
{
	int status;
	struct CallInfo* old_ci = ci;
	ptrdiff_t old_errorfunc = errorfunc;

	status = luaD_rawunprotected(f, ud);
	if (status != LUA_OK) {
		// because we have not implement gc, so we should free ci manually
		struct global_State* g = G(this);
		struct CallInfo* free_ci = ci;
		while (free_ci) {
			if (free_ci == old_ci) {
				free_ci = free_ci->next;
				continue;
			}

			struct CallInfo* previous = free_ci->previous;
			previous->next = NULL;

			struct CallInfo* next = free_ci->next;
			(*g->freealloc)(g->ud, free_ci, sizeof(struct CallInfo), 0);
			free_ci = next;
		}

		reset_unuse_stack(oldtop);
		this->ci = old_ci;
		this->top = restorestack(this, oldtop);
		seterrobj(status);
	}

	errorfunc = old_errorfunc;

	return status;
}

void lua_State::reset_unuse_stack(ptrdiff_t old_top)
{
	struct global_State* g = G(this);
	StkId top = restorestack(this, old_top);
	for (; top < this->top; top++) {
		if (top->value_.p) {
			(*g->freealloc)(g->ud, top->value_.p, sizeof(top->value_.p), 0);
			top->value_.p = NULL;
		}
		top->tt_ = LUA_TNIL;
	}
}

CallInfo* lua_State::next_ci(StkId func, int nresult)
{
#if 0
	global_State* g = G(L);
	struct CallInfo* ci;
	ci = (struct CallInfo*)luaM_realloc(L, NULL, 0, sizeof(struct CallInfo));
#else
	auto ci = (struct CallInfo*)luaM_realloc(this, nullptr, 0, sizeof(struct CallInfo));
#endif
	ci->next = nullptr;
	ci->previous = this->ci;
	this->ci->next = ci;
	ci->nresult = nresult;
	ci->callstatus = LUA_OK;
	ci->func = func;
	ci->top = this->top + LUA_MINSTACK;
	this->ci = ci;
	return ci;
}
