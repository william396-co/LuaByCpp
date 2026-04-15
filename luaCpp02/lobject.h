#pragma once

#include "lua.h"

#include <functional>

class lua_State;

using lua_Integer = LUA_INTEGER;
using lua_Number = LUA_NUMBER;
using lu_byte = unsigned char;
using lua_CFunction = int(*)(lua_State*);
using lua_Alloc = void* (*) (void* ud, void* ptr, size_t osize, size_t nsize);

// lua number type
#define LUA_NUMINT (LUA_TNUMBER | (0<<4))
#define LUA_NUMFLT (LUA_TNUMBER | (1<<4))

// lua function
#define LUA_TLCL (LUA_TFUNCTION | (0<<4))
#define LUA_TLCF (LUA_TFUNCTION | (1<<4))
#define LUA_TCCL (LUA_TFUNCTION | (2<<4))

// string type
#define LUA_LNGSTR (LUA_TSTRING | (0<<4))
#define LUA_SHRSTR (LUA_TSTRING | (1<<4))


// GCObject
#define CommonHeader struct GCObject* next; lu_byte tt_;lu_byte marked
#define LUA_GCSTEPMUL 200

struct GCObject {
	CommonHeader;
};

typedef union lua_Value {
	struct GCObject* gc;
	void* p;// light userdata
	int b; // boolean: 1 = true, 0 = false
	lua_Integer i;
	lua_Number n;
	lua_CFunction f;

	void AssignTo(lua_Value const& other, int tt) {
		switch (tt)
		{
		case LUA_NUMINT:
			i = other.i;
			break;
		case LUA_TLCF:
			f = other.f;
			break;
		case LUA_NUMFLT:
			n = other.n;
			break;
		case LUA_TBOOLEAN:
			b = other.b;
			break;
		case LUA_TLIGHTUSERDATA:
			p = other.p;
			break;
		default:
			break;
		}
	}
}Value;

struct TValue {
	Value value_{};
	int tt_{ LUA_TNIL };
public:
	TValue() {
		setnilvalue();
	}

	explicit TValue(int integer) {
		setivalue(integer);
	}
	explicit TValue(lua_CFunction f) {
		setfvalue(f);
	}
	explicit TValue(float number) {
		setfltvalue(number);
	}
	explicit TValue(bool b) {
		setbvalue(b);
	}
	explicit TValue(void* p) {
		setpvalue(p);
	}
	explicit TValue(TValue* value) {
		setobj(value);
	}

	void setnilvalue() { tt_ = LUA_TNIL; }
	void setivalue(int integer);
	void setfvalue(lua_CFunction f);
	void setfltvalue(float number);
	void setbvalue(bool b);
	void setpvalue(void* p);
	void setobj(TValue* value);
};

using StkId = TValue*;

struct TString {
	int test_field1;
	int test_field2;
};
