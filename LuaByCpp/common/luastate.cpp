#include "luastate.h"

#include <cassert>

#define LUA_TRY(c,a) if(_setjmp((c)->b) == 0){ a }

#ifdef _WIN32
#define LUA_THROW(c) longjmp((c)->b,1)
#else
#define LUA_THROW(c) _longjmp((c)->b,1)
#endif

struct LX {
	lua_byte extra_[LUA_EXTRASPACE];
	LuaState l;
};

struct LG {
	LX l;
	GlobalState g;
};

struct LuaLongJmp {
	struct LuaLongJmp* previous{};
	jmp_buf b;
	int status;
};

LuaState::LuaState()
{
	StackInit();
}

void LuaState::Seterrobj(int error)
{
	Pushinteger(error);
}

void LuaState::luaD_checkstack(int need)
{
	if (top_ + need > stack_last_) {
		luaD_growstack(need);
	}
}

void LuaState::luaD_growstack(int size)
{
	if (stack_size_ > LUA_MAXSTACK) {
		luaD_throw(ErrorCode::Lua_ErrErr);
	}

	auto new_stack_size = stack_size_ * 2;
	auto need_size = int(top_ - stack_) + size + LUA_EXTRASTACK;
	if (new_stack_size < need_size) {
		new_stack_size = need_size;
	}

	auto old_stack = stack_;
	stack_ = (TValue*)realloc(stack_, new_stack_size * sizeof(TValue));
	stack_size_ = new_stack_size;
	stack_last_ = stack_ + new_stack_size - LUA_EXTRASPACE;
	auto top_diff = top_ - old_stack;
	top_ = RestoreStack(top_diff);

	struct CallInfo* ci;
	ci = &base_ci_;
	while (ci) {
		int func_diff =  ci->func - old_stack;
		int top_diff =  ci->top - old_stack;
		ci->func = RestoreStack(func_diff);
		ci->top = RestoreStack(top_diff);
		ci = ci->next;
	}
}

void LuaState::luaD_throw(int error)
{
	if (errorjmp_) {
		errorjmp_->status = error;
		LUA_THROW(errorjmp_);
	}
	else {
		/*if (g->painc) {
			(*g->painc)(L);
		}*/
		abort();
	}
}

int LuaState::luaD_rawunprotected(Pfunc const& f, void* ud)
{
	auto old_calls = ncalls_;
	struct LuaLongJmp lj {};
	lj.previous = errorjmp_;
	lj.status = ErrorCode::Lua_Ok;
	errorjmp_ = &lj;

	LUA_TRY(
		errorjmp_,
		(f)(ud);
	)

	errorjmp_ = lj.previous;
	ncalls_ = old_calls;
	return lj.status;
}

/*
* prepare for function call
* if we call a c function, just directly call it
* if we call a lua function, just prepare for call it
*/
int LuaState::luaD_precall(StkId func, int nresult)
{
	switch (func->tt_)
	{
	case LUA_TLCF:// c function
	{
		Lua_CFunction f = std::get<Lua_CFunction>(func->value_);
		ptrdiff_t func_diff = SaveStack(func);
		func = RestoreStack(func_diff);

		Next_ci(func, nresult);
		int n = (f)(this);
		assert(ci_->func + n < ci_->top);
		luaD_poscall(top_ - n, n);
		return 1;
	}
	default:
		break;
	}
	return 0;
}

int LuaState::luaD_poscall(StkId first_result, int nresult)
{
	auto func = ci_->func;
	int nwant = ci_->nresults;
	switch (nwant)
	{
	case 0: {
		top_ = ci_->func;
		break;
	}
	case 1: {
		if (0 == nresult) {
			first_result->value_ = nullptr;
			first_result->tt_ = Lua_TNil;
		}
		Setobj(func, first_result);
		first_result->value_ = nullptr;
		first_result->tt_ = Lua_TNil;

		top_ = func + nwant;
		debug_info(__PRETTY_FUNCTION__, "&top",top_);
		break;
	}
	case LUA_MULRET: {
		int nres = top_ - first_result;
		for (int i = 0; i < nres; ++i) {
			auto cur = first_result + i;
			Setobj(func + i, cur);
			cur->value_ = nullptr;
			cur->tt_ = Lua_TNil;
		}
		top_ = func + nres;
		break;
	}
	default:
		if (nwant > nresult) {
			for (int i = 0; i < nwant; ++i) {
				if (i < nresult) {
					auto cur = first_result + i;
					Setobj(func + i, cur);
					cur->value_ = nullptr;
					cur->tt_ = Lua_TNil;
				}
				else {
					auto stk = func + i;
					stk->tt_ = Lua_TNil;
				}
			}
			top_ = func + nwant;
		}
		else {
			for (int i = 0; i < nresult; ++i) {
				if (i < nwant) {
					auto cur = first_result + i;
					Setobj(func + i, cur);
					cur->value_ = nullptr;
					cur->tt_ = Lua_TNil;
				}
				else {
					auto stk = func + i;
					stk->value_ = nullptr;
					stk->tt_ = Lua_TNil;
				}
			}
			top_ = func + nresult;
		}
		break;
	}
	auto cur_ci = ci_;
	ci_ = ci_->previous;
	ci_->next = nullptr;

	// because we have not implement gc, so we should free ci manually
	delete cur_ci;
	return ErrorCode::Lua_Ok;
}

int LuaState::luaD_call(StkId func, int nresult)
{
	if (++ncalls_ > LUA_MAXCALLS) {
		luaD_throw(0);
	}
	if (!luaD_precall(func, nresult)) {
		// TODO luaV_execute()
	}
	ncalls_--;
	return ErrorCode::Lua_Ok;
}

int LuaState::luaD_pcall(Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef)
{
	auto old_ci = ci_;
	ptrdiff_t old_errorfunc = errorfunc_;
	auto status = luaD_rawunprotected(f, ud);
	if (status != ErrorCode::Lua_Ok) {
		// because we have not implement gc, so we should free ci manually
		auto free_ci = ci_;
		while (free_ci) {
			if (free_ci == old_ci) {
				free_ci = free_ci->next;
				continue;
			}

			auto previous = free_ci->previous;
			previous->next = nullptr;

			auto next = free_ci->next;
			delete free_ci;
			free_ci = next;
		}

		Reset_unuse_stack(oldtop);
		ci_ = old_ci;
		top_ = RestoreStack(oldtop);
		Seterrobj(status);
	}
	errorfunc_ = old_errorfunc;
	return status;
}

CallInfo* LuaState::Next_ci(StkId func, int nresult)
{
	auto new_ci = new CallInfo;
	new_ci->previous = ci_;
	new_ci->next = ci_;
	new_ci->nresults = nresult;
	new_ci->callstatus = ErrorCode::Lua_Ok;
	new_ci->func = func;
	new_ci->top = top_ + LUA_MINSTACK;
	ci_ = new_ci;
	return ci_;
}

void LuaState::Reset_unuse_stack(ptrdiff_t old_top)
{
	auto new_top = RestoreStack(old_top);
	for (; new_top < top_; ++new_top) {
		if (std::get<void*>(new_top->value_)) {
			free(std::get<void*>(new_top->value_));
			new_top->value_ = nullptr;
		}
		new_top->tt_ = Lua_TNil;
	}
}

void LuaState::Pushcfunction(Lua_CFunction f)
{
	Setfvalue(top_, f);
	Increase_top();
}

void LuaState::Pushinteger(int integer)
{
	Setivalue(top_, integer);
	Increase_top();
}

void LuaState::Pushnumber(float number)
{
	Setfltvalue(top_, number);
	Increase_top();
}

void LuaState::Pushboolean(bool b)
{
	Setbvalue(top_, b);
	Increase_top();
}

void LuaState::Pushnil()
{
	Setnilvalue(top_);
	Increase_top();
}

void LuaState::Pushlightuserdata(void* p)
{
	Setpvalue(top_, p);
	Increase_top();
}

Lua_Integer LuaState::ToIntegerx(int idx, bool& isOk)const
{
	auto addr = index2addr(idx);
	isOk = (addr && addr->tt_ == LUA_NUMINT);
	return isOk ? std::get<Lua_Integer>(addr->value_) : Lua_Integer{};
}

Lua_Number LuaState::ToNumberx(int idx, bool& isOk)const
{
	auto addr = index2addr(idx);

	isOk = addr && addr->tt_ == LUA_NUMINT;
	return isOk ? std::get<Lua_Number>(addr->value_) : Lua_Number{};
}

bool LuaState::ToBoolean(int idx) const
{
	auto addr = index2addr(idx);
	return !(addr->tt_ == Lua_TNil || std::get<bool>(addr->value_) == false);
}

bool LuaState::IsNil(int idx) const
{
	auto addr = index2addr(idx);
	return addr ? addr->tt_ == Lua_TNil : false;
}

StkId LuaState::RestoreStack(ptrdiff_t top_diff)
{
	return stack_ + top_diff;
}

ptrdiff_t LuaState::SaveStack(StkId top) const
{
	return top - stack_;
}

TValue* LuaState::index2addr(int idx) const
{
	if (idx >= 0) {
		assert(ci_->func + idx < ci_->top);
		return ci_->func + idx;
	}
	else {
		assert(top_ + idx > ci_->func);
		return top_ + idx;
	}
	return nullptr;
}

void LuaState::FreeStack()
{
	delete stack_;
	stack_ = stack_last_ = top_ = nullptr;
	stack_size_ = 0;
}

void LuaState::StackInit()
{
	stack_size_ = LUA_STACKSIZE;
	stack_ = new TValue[stack_size_];
	stack_last_ = stack_ + LUA_STACKSIZE - LUA_EXTRASPACE;
	next_ = previous_ = nullptr;
	status_ = ErrorCode::Lua_Ok;
	errorjmp_ = nullptr;
	top_ = stack_;
	errorfunc_ = 0;
	ncalls_ = 0;
	debug_info(__PRETTY_FUNCTION__, "&top", top_);
	debug_info(__PRETTY_FUNCTION__, "&stack", stack_);
	
	for (int i = 0;i < stack_size_;++i) {
		Setnilvalue(stack_ + i);
	}
	++top_;
	debug_info(__PRETTY_FUNCTION__, "&top", top_);

	ci_ = &base_ci_;
	ci_->func = stack_;
	ci_->top = stack_ + LUA_MINSTACK;
	ci_->previous = ci_->next = nullptr;
}

void LuaState::Close()
{
	auto ci = &base_ci_;
	while (ci->next) {
		auto next = ci->next;
		auto free_ci = ci->next;
		delete free_ci;
		ci = next;
	}	
	FreeStack();
}

void LuaState::Increase_top()
{
	++top_;
	debug_info(__PRETTY_FUNCTION__, "&top", top_);
	assert(top_ <= stack_last_);
}

void LuaState::Settop(int idx)
{
	auto curfuc = ci_->func;
	if (idx >= 0) {
		assert(idx <= stack_last_ - (curfuc + 1));
		while (top_ < (curfuc + 1) + idx) {
			Setnilvalue(top_++);
		}
		top_ = curfuc + 1 + idx;
	}
	else {
		assert(top_ + idx > curfuc);
		top_ += idx;
	}
}

int LuaState::StackSize()const
{
	return top_ - (ci_->func + 1);
}

void LuaState::Pop()
{
	Settop(-1);
}

void LuaState::Setobj(StkId target, StkId value)
{
	target->tt_ = value->tt_;
	target->value_ = value->value_;
}

void LuaState::Setivalue(StkId target, int integer)
{
	target->tt_ = LUA_NUMINT;
	target->value_ = integer;
}

void LuaState::Setfvalue(StkId target, Lua_CFunction f)
{
	target->tt_ = LUA_TLCF;
	target->value_ = f;
}

void LuaState::Setfltvalue(StkId target, float number)
{
	target->tt_ = LUA_NUMFLT;
	target->value_ = number;
}

void LuaState::Setbvalue(StkId target, bool b)
{
	target->tt_ = Lua_TBoolean;
	target->value_ = b;
}

void LuaState::Setnilvalue(StkId target)
{
	target->tt_ = Lua_TNil;
	target->value_ = {};
}

void LuaState::Setpvalue(StkId target, void* p)
{
	target->tt_ = Lua_TLightUserData;
	target->value_ = p;
}
