#pragma once
#include "lobject.h"

#define LUA_EXTRASPACE sizeof(void*)
#define G(L) ((L)->l_G)


struct CallInfo {
	StkId func;               // 被调用函数在栈中的位置
	StkId top;                // 栈顶位置
	int nresult;              // 有多少个返回值
	int callstatus;           // 调用状态
	struct CallInfo* next;    // 下一个调用
	struct CallInfo* previous;// 上一个调用
};

class lua_State;
using Pfunc = int(*)(lua_State*, void*);

class lua_State 
{
public:
	StkId stack{};                 // 栈
	StkId stack_last{};            // 从这里开始,栈不能被使用
	StkId top{};                   // 栈顶
	int stack_size{};              // 栈的大小
	struct lua_longjmp* errorjmp;// 保护模式中，要用的结构，当异常抛出时，跳出逻辑
	int status;                  // lua_state的状态
	lua_State* next{};      // 下一个lua_state,通常创建协程是会产生
	lua_State* previous{};
	struct CallInfo base_ci;     // 和lua_state生命周期一致的函数调用信息
	struct CallInfo* ci{};         // 当前运作的CallInfo
	struct global_State* l_G{};    // global_State指针
	ptrdiff_t errorfunc{};         // 错误函数位置栈的哪个位置
	int ncalls{};                  // 进行多少次函数调用
public:
	lua_State();
	~lua_State();

	lua_State(lua_State const&) = delete;
	lua_State& operator=(lua_State const&) = delete;

	lua_State(lua_State&&) = delete;
	lua_State& operator=(lua_State&&) = delete;
public:
	// Lua stack operation
	void increase_top();
	void lua_pushcfunction(lua_CFunction f);
	void lua_pushinteger(int integer);
	void lua_pushnumber(float number);
	void lua_pushboolean(bool b);
	void lua_pushnil();
	void lua_pushlightuserdata(void* p);


	lua_Integer lua_tointegerx(int idx, int* isnum);
	lua_Number lua_tonumberx(int idx, int* isnum);
	bool lua_toboolean(int idx);
	int lua_isnil(int idx);

	void lua_settop(int idx);
	int lua_gettop();
	void lua_pop();

	StkId get_top() { return top; }
public:
	// Lua vm operation
	void seterrobj(int error);
	void luaD_checkstack(int need);
	void luaD_growstack(int size);
	void luaD_throw(int error);

	int luaD_rawunprotected(Pfunc f, void* ud);
	int luaD_precall(StkId func, int nresult);
	int luaD_poscall(StkId first_result, int nresult);
	int luaD_call(StkId func, int nresult);
	int luaD_pcall(Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef);
private:
	void reset_unuse_stack(ptrdiff_t old_top);
	CallInfo* next_ci(StkId func, int nresult);
private:
	TValue* index2addr(int idx);

public:
	void free_stack();
	void stack_init();
};

typedef struct global_State {
	lua_State* mainthread;  // lua_State其实是lua_thread, 某种程度来说，它也是协程
	lua_Alloc freealloc;           // 自定义的内存分配函数
	void* ud;                      // 当我们自定义内存分配器时，可能要用到这个结构，但是我们用官方默认的版本
	// 因此它始终是NULL
	lua_CFunction panic;           // 当调用LUA_THROW接口时，如果当前不处于保护模式，那么会直接调用panic函数
	// panic函数通常是输出一些关键日志
}global_State;

lua_State* lua_newstate(lua_Alloc alloc, void* ud);
void lua_close(lua_State* L);