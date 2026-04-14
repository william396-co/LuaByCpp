#include "lobject.h"

void TValue::setivalue(int integer)
{
	tt_ = LUA_NUMINT;
	value_.i = integer;
}

void TValue::setfvalue(lua_CFunction f)
{
	tt_ = LUA_TLCF;
	value_.f = f;
}

void TValue::setfltvalue(float number)
{
	tt_ = LUA_NUMFLT;
	value_.n = number;
}

void TValue::setbvalue(bool b)
{
	tt_ = LUA_TBOOLEAN;
	value_.b = b ? 1 : 0;
}

void TValue::setpvalue(void* p)
{
	tt_ = LUA_TLIGHTUSERDATA;
	value_.p = p;
}

void TValue::setobj(TValue* value)
{
	this->tt_ = value->tt_;
	this->value_.AssignTo(value->value_, value->tt_);
}
