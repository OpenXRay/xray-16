////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine_script.cpp
//	Created 	: 25.12.2002
//  Modified 	: 13.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator script engine export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_engine.h"
#include "ai_space.h"
#include "script_debugger.h"

using namespace luabind;

void LuaLog(LPCSTR caMessage)
{
#ifndef MASTER_GOLD
	ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeMessage,"%s",caMessage);
#endif // #ifndef MASTER_GOLD

#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
		if( ai().script_engine().debugger() )
			ai().script_engine().debugger()->Write(caMessage);
#	endif // #ifndef USE_LUA_STUDIO
#endif // #ifdef USE_DEBUGGER
}

void ErrorLog(LPCSTR caMessage)
{
	ai().script_engine().error_log("%s",caMessage);
#ifdef PRINT_CALL_STACK
	ai().script_engine().print_stack();
#endif // #ifdef PRINT_CALL_STACK
	
#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
		if( ai().script_engine().debugger() ){
			ai().script_engine().debugger()->Write(caMessage);
		}
#	endif // #ifndef USE_LUA_STUDIO
#endif // #ifdef USE_DEBUGGER

#ifdef DEBUG
		bool lua_studio_connected = !!ai().script_engine().debugger();
		if (!lua_studio_connected)
#endif //#ifdef DEBUG
	R_ASSERT2(0, caMessage);
}

void FlushLogs()
{
#ifdef DEBUG
	FlushLog();
	ai().script_engine().flush_log();
#endif // DEBUG
}

void verify_if_thread_is_running()
{
	THROW2	(ai().script_engine().current_thread(),"coroutine.yield() is called outside the LUA thread!");
}

bool is_editor()
{
#ifdef XRGAME_EXPORTS
	return		(false);
#else
	return		(true);
#endif
}

#ifdef XRGAME_EXPORTS
CRenderDevice *get_device()
{
	return		(&Device);
}
#endif

int bit_and(int i, int j)
{
	return			(i & j);
}

int bit_or(int i, int j)
{
	return			(i | j);
}

int bit_xor(int i, int j)
{
	return			(i ^ j);
}

int bit_not(int i)
{
	return			(~i);
}

LPCSTR user_name()
{
	return			(Core.UserName);
}

void prefetch_module(LPCSTR file_name)
{
	ai().script_engine().process_file(file_name);
}

struct profile_timer_script {
	u64							m_start_cpu_tick_count;
	u64							m_accumulator;
	u64							m_count;
	int							m_recurse_mark;
	
	IC								profile_timer_script	()
	{
		m_start_cpu_tick_count	= 0;
		m_accumulator			= 0;
		m_count					= 0;
		m_recurse_mark			= 0;
	}

	IC								profile_timer_script	(const profile_timer_script &profile_timer)
	{
		*this					= profile_timer;
	}

	IC		profile_timer_script&	operator=				(const profile_timer_script &profile_timer)
	{
		m_start_cpu_tick_count	= profile_timer.m_start_cpu_tick_count;
		m_accumulator			= profile_timer.m_accumulator;
		m_count					= profile_timer.m_count;
		m_recurse_mark			= profile_timer.m_recurse_mark;
		return					(*this);
	}

	IC		bool					operator<				(const profile_timer_script &profile_timer) const
	{
		return					(m_accumulator < profile_timer.m_accumulator);
	}

	IC		void					start					()
	{
		if (m_recurse_mark) {
			++m_recurse_mark;
			return;
		}

		++m_recurse_mark;
		++m_count;
		m_start_cpu_tick_count	= CPU::GetCLK();
	}

	IC		void					stop					()
	{
		THROW					(m_recurse_mark);
		--m_recurse_mark;
		
		if (m_recurse_mark)
			return;
		
		u64						finish = CPU::GetCLK();
		if (finish > m_start_cpu_tick_count)
			m_accumulator		+= finish - m_start_cpu_tick_count;
	}

	IC		float					time					() const
	{
		FPU::m64r				();
		float					result = (float(double(m_accumulator)/double(CPU::clk_per_second))*1000000.f);
		FPU::m24r				();
		return					(result);
	}
};

IC	profile_timer_script	operator+	(const profile_timer_script &portion0, const profile_timer_script &portion1)
{
	profile_timer_script	result;
	result.m_accumulator	= portion0.m_accumulator + portion1.m_accumulator;
	result.m_count			= portion0.m_count + portion1.m_count;
	return					(result);
}

// IC	std::ostream& operator<<(std::ostream &stream, profile_timer_script &timer)
// {
// 	stream					<< timer.time();
// 	return					(stream);
// }

#ifdef XRGAME_EXPORTS
ICF	u32	script_time_global	()	{ return Device.dwTimeGlobal; }
ICF	u32	script_time_global_async	()	{ return Device.TimerAsync_MMT(); }
#else
ICF	u32	script_time_global	()	{ return 0; }
ICF	u32	script_time_global_async	()	{ return 0; }
#endif

#ifdef XRGAME_EXPORTS
static bool is_enough_address_space_available_impl()
{
	ENGINE_API bool is_enough_address_space_available();
	return is_enough_address_space_available( );
}
#endif // #ifdef XRGAME_EXPORTS

#pragma optimize("s",on)
void CScriptEngine::script_register(lua_State *L)
{
	module(L)[
		class_<profile_timer_script>("profile_timer")
			.def(constructor<>())
			.def(constructor<profile_timer_script&>())
			.def(const_self + profile_timer_script())
			.def(const_self < profile_timer_script())
			.def(tostring(self))
			.def("start",&profile_timer_script::start)
			.def("stop",&profile_timer_script::stop)
			.def("time",&profile_timer_script::time)
	];

	function	(L,	"log",								LuaLog);
	function	(L,	"error_log",						ErrorLog);
	function	(L,	"flush",							FlushLogs);
	function	(L,	"prefetch",							prefetch_module);
	function	(L,	"verify_if_thread_is_running",		verify_if_thread_is_running);
	function	(L,	"editor",							is_editor);
	function	(L,	"bit_and",							bit_and);
	function	(L,	"bit_or",							bit_or);
	function	(L,	"bit_xor",							bit_xor);
	function	(L,	"bit_not",							bit_not);
	function	(L, "user_name",						user_name);
	function	(L, "time_global",						script_time_global);
	function	(L, "time_global_async",				script_time_global_async);
#ifdef XRGAME_EXPORTS
	function	(L,	"device",							get_device);
	function	(L,	"is_enough_address_space_available",is_enough_address_space_available_impl);
#endif // #ifdef XRGAME_EXPORTS
}
