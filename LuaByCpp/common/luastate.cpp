#include "luastate.h"

#include <cassert>

struct LX {
	lua_byte extra_[LUA_EXTRASPACE];
	LuaState l;
};

struct LG {
	LX l;
	GlobalState g;
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
	if (top + need > stack_last) {
		luaD_growstack(need);
	}
}

void LuaState::luaD_growstack(int size)
{
	if (stack_size > LUA_MAXSTACK) {
		luaD_throw(ErrorCode::Lua_ErrErr);
	}

	auto new_stack_size = stack_size * 2;
	auto need_size = int(top - stack) + size + LUA_EXTRASTACK;
	if (new_stack_size < need_size) {
		new_stack_size = need_size;
	}

	auto old_stack = stack;
	stack = (TValue*)realloc(stack, new_stack_size * sizeof(TValue));
	stack_size = new_stack_size;
	stack_last = stack + new_stack_size - LUA_EXTRASPACE;
	auto top_diff = top - old_stack;
	top = Restorestack(top_diff);

	struct CallInfo* ci;
	ci = base_ci;
	while (ci) {
		int func_diff =  ci->func - old_stack;
		int top_diff =  ci->top - old_stack;
		ci->func = Restorestack(func_diff);
		ci->top = Restorestack(top_diff);
		ci = ci->next;
	}
}

void LuaState::luaD_throw(int error)
{
}

int LuaState::luaD_rawunprotected(Pfunc const& f, void* ud)
{
	return 0;
}

int LuaState::luaD_precall(StkId func, int nresult)
{
	return 0;
}

int LuaState::luaD_poscall(StkId first_result, int nresult)
{
	return 0;
}

int LuaState::luaD_call(StkId func, int nresult)
{
	return 0;
}

int LuaState::luaD_pcall(Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef)
{
	return 0;
}

void LuaState::Pushcfunction(Lua_CFunction const& f)
{
	Setfvalue(top, f);
	Increase_top();
}

void LuaState::Pushinteger(int integer)
{
	Setivalue(top, integer);
	Increase_top();
}

void LuaState::Pushnumber(float number)
{
	Setfltvalue(top, number);
	Increase_top();
}

void LuaState::Pushboolean(bool b)
{
	Setbvalue(top, b);
	Increase_top();
}

void LuaState::Pushnil()
{
	Setnilvalue(top);
	Increase_top();
}

void LuaState::Pushlightuserdata(void* p)
{
	Setpvalue(top, p);
	Increase_top();
}

Lua_Integer LuaState::ToIntegerx(int idx, bool& isOk)const
{
	auto addr = index2addr(idx);
	isOk = (addr && addr->tt_ == LUA_NUMINT);
	return isOk ? GetValue2T<Lua_Integer>(addr->value_) : Lua_Integer{};
}

Lua_Number LuaState::ToNumberx(int idx, bool& isOk)const
{
	auto addr = index2addr(idx);

	isOk = addr && addr->tt_ == LUA_NUMINT;
	return isOk ? GetValue2T<Lua_Number>(addr->value_) : Lua_Number{};
}

bool LuaState::ToBoolean(int idx) const
{
	auto addr = index2addr(idx);
	return !(addr->tt_ == Lua_TNil || GetValue2T<bool>(addr->value_) == false);
}

bool LuaState::IsNil(int idx) const
{
	auto addr = index2addr(idx);
	return addr ? addr->tt_ == Lua_TNil : false;
}

TValue* LuaState::index2addr(int idx) const
{
	if (idx >= 0) {
		assert(ci->func + idx < ci->top);
		return ci->func + idx;
	}
	else {
		assert(top + idx > ci->func);
		return top + idx;
	}
	return nullptr;
}

void LuaState::StackInit()
{
	stack_size = LUA_STACKSIZE;
	stack = (StkId)malloc(sizeof(TValue) * stack_size);
	stack_last = stack + LUA_STACKSIZE - LUA_EXTRASPACE;
	next = previous = nullptr;
	status = ErrorCode::Lua_Ok;
	errorjmp = nullptr;
	top = stack;
	errorfunc = 0;
	ncalls = 0;
	
	for (int i = 0;i < stack_size;++i) {
		Setnilvalue(stack + i);
	}
	top++;

	ci = base_ci;
	ci->func = stack;
	ci->top = stack + LUA_MINSTACK;
	ci->previous = ci->next = nullptr;
}

void LuaState::Close()
{
}

void LuaState::Increase_top()
{
	top++;
	assert(top <= stack_last);
}

void LuaState::Settop(int idx)
{
	auto curfuc = ci->func;
	if (idx >= 0) {
		assert(idx <= stack_last - (curfuc + 1));
		while (top < (curfuc + 1) + idx) {
			Setnilvalue(top++);
		}
		top = curfuc + 1 + idx;
	}
	else {
		assert(top + idx > curfuc);
		top += idx;
	}
}

int LuaState::Gettop()
{
	return top - (ci->func + 1);
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

void LuaState::Setfvalue(StkId target, Lua_CFunction const& f)
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
}

void LuaState::Setpvalue(StkId target, void* p)
{
	target->tt_ = Lua_TLightUserData;
	target->value_ = p;
}
