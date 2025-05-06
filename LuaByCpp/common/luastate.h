#pragma once

#include <functional>

#include "luaobject.h"

constexpr auto LUA_EXTRASPACE = sizeof(void*);
#define G(L) ((L)->l_G) // TODO change to c++ mode

using  StkId = TValue*;

struct CallInfo {
	StkId func{};                        // 被调用函数在栈中的位置
	StkId top{};						 // 栈顶
	int nresults{};						 // 有多少个返回值
	int callstatus{ ErrorCode::Lua_Ok }; // 调用状态
	CallInfo* next{};					 // 下一个调用
	CallInfo* previous{};                // 上一个调用
};

struct GlobalState;
class LuaState;
struct LuaLongJmp;

using Pfunc = std::function<int(void* ud)>;

class LuaState {
public:
	LuaState();
	~LuaState() { Close(); }	


public:
	/*  VM相关函数 */
	void Seterrobj(int error);
	void luaD_checkstack(int need);
	void luaD_growstack(int size);
	void luaD_throw(int error);

	int luaD_rawunprotected(Pfunc const& f, void* ud);
	int luaD_precall(StkId func, int nresult);
	int luaD_poscall(StkId first_result, int nresult);
	int luaD_call(StkId func, int nresult);
	int luaD_pcall(Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef);

	StkId GetStackTop()const { return top; }
	StkId Restorestack(ptrdiff_t diff) { top += diff; return top; }
	ptrdiff_t Savestack(StkId t)const { return stack - t; }
private:
	// 产生新的CallInfo
	CallInfo* Next_ci(StkId func, int nresult);
	// 释放未使用的栈
	void Reset_unuse_stack(ptrdiff_t old_top);
public:
	/* Stack关函数 */
	void Pushcfunction(Lua_CFunction f);
	void Pushinteger(int integer);
	void Pushnumber(float number);
	void Pushboolean(bool b);
	void Pushnil();
	void Pushlightuserdata(void* p);

	Lua_Integer ToIntegerx(int idx,bool& isOk)const;
	Lua_Number ToNumberx(int idxm,bool& isOk)const;
	bool ToBoolean(int idx)const;
	bool IsNil(int idx)const;

	int Gettop();
	void Pop();
private:
	TValue* index2addr(int idx)const;
	void FreeStack();
private:
	void StackInit();
	void Close();

	// 栈相关操作函数
	void Increase_top();// 栈增长
	void Settop(int idx);
private:
	void Setobj(StkId target, StkId value);
	void Setivalue(StkId target, int integer);
	void Setfvalue(StkId target, Lua_CFunction f);
	void Setfltvalue(StkId target, float number);
	void Setbvalue(StkId target, bool b);
	void Setnilvalue(StkId target);
	void Setpvalue(StkId target, void* p);
private:
	StkId stack{};// 栈
	StkId stack_last{};// 从这里开始，栈不能被使用
	StkId top{};// 栈顶
	int stack_size{};// 栈的大小
	LuaLongJmp* errorjmp{};// 保护模式中，要用的结构，当异常抛出时，跳出逻辑
	int status{ ErrorCode::Lua_Ok };// LuaState的状态
	LuaState* next{};// 下一个LuaState，通常创建协程时候会产生
	LuaState* previous{};
	CallInfo base_ci{};//和LuaState生命周期一致的函数调用信息
	CallInfo* ci{};//当前运行的CallInfo
	//GlobalState* l_G {};// GlobalState指针
	ptrdiff_t errorfunc{};// 错误函数位于栈的哪个位置
	int ncalls{};//进行多少次函数调用
};

struct GlobalState {
	LuaState* mainthread{};  // lua_State其实是lua_thread, 某种程度来说，它也是协程
	Lua_Alloc freealloc;           // 自定义的内存分配函数
	void* ud;                      // 当我们自定义内存分配器时，可能要用到这个结构，但是我们用官方默认的版本
	// 因此它始终是NULL
	Lua_CFunction panic;           // 当调用LUA_THROW接口时，如果当前不处于保护模式，那么会直接调用panic函数
	// panic函数通常是输出一些关键日志
};
