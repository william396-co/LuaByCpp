#pragma once

#include <functional>

#include "luaobject.h"

constexpr auto LUA_EXTRASPACE = sizeof(void*);
#define G(L) ((L)->l_G) // TODO change to c++ mode

using  StkId = TValue*;

struct CallInfo {
	StkId func{};                        // �����ú�����ջ�е�λ��
	StkId top{};						 // ջ��
	int nresults{};						 // �ж��ٸ�����ֵ
	int callstatus{ ErrorCode::Lua_Ok }; // ����״̬
	CallInfo* next{};					 // ��һ������
	CallInfo* previous{};                // ��һ������
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
	/*  VM��غ��� */
	void Seterrobj(int error);
	void luaD_checkstack(int need);
	void luaD_growstack(int size);
	void luaD_throw(int error);

	int luaD_rawunprotected(Pfunc const& f, void* ud);
	int luaD_precall(StkId func, int nresult);
	int luaD_poscall(StkId first_result, int nresult);
	int luaD_call(StkId func, int nresult);
	int luaD_pcall(Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef);

	StkId GetStackTop()const { return top_; }
	//StkId Restorestack(ptrdiff_t diff) { top += diff; return top; }
	//ptrdiff_t Savestack(StkId t)const { return stack - t; }
private:
	// �����µ�CallInfo
	CallInfo* Next_ci(StkId func, int nresult);
	// �ͷ�δʹ�õ�ջ
	void Reset_unuse_stack(ptrdiff_t old_top);
public:
	/* Stack�غ��� */
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

	StkId RestoreStack(ptrdiff_t top_diff);
	ptrdiff_t SaveStack(StkId top)const;

	int StackSize()const;
	void Pop();
private:
	TValue* index2addr(int idx)const;
	void FreeStack();
private:
	void StackInit();
	void Close();

	// ջ��ز�������
	void Increase_top();// ջ����
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
	StkId stack_{};// ջ
	StkId stack_last_{};// �����￪ʼ��ջ���ܱ�ʹ��
	StkId top_{};// ջ��
	int stack_size_{};// ջ�Ĵ�С
	LuaLongJmp* errorjmp_{};// ����ģʽ�У�Ҫ�õĽṹ�����쳣�׳�ʱ�������߼�
	int status_{ ErrorCode::Lua_Ok };// LuaState��״̬
	LuaState* next_{};// ��һ��LuaState��ͨ������Э��ʱ������
	LuaState* previous_{};
	CallInfo base_ci_{};//��LuaState��������һ�µĺ���������Ϣ
	CallInfo* ci_{};//��ǰ���е�CallInfo
	//GlobalState* l_G {};// GlobalStateָ��
	ptrdiff_t errorfunc_{};// ������λ��ջ���ĸ�λ��
	int ncalls_{};//���ж��ٴκ�������
};

struct GlobalState {
	LuaState* mainthread{};  // lua_State��ʵ��lua_thread, ĳ�̶ֳ���˵����Ҳ��Э��
	Lua_Alloc freealloc;           // �Զ�����ڴ���亯��
	void* ud;                      // �������Զ����ڴ������ʱ������Ҫ�õ�����ṹ�����������ùٷ�Ĭ�ϵİ汾
	// �����ʼ����NULL
	Lua_CFunction panic;           // ������LUA_THROW�ӿ�ʱ�������ǰ�����ڱ���ģʽ����ô��ֱ�ӵ���panic����
	// panic����ͨ�������һЩ�ؼ���־
};
