#pragma once

struct StackTrace{
	char szDesc[255];
	char szFile[255];
	int nLine;
	StackTrace(){szDesc[0]=0;szFile[0]=0;nLine=0;};
} ;

struct Variable
{
	char szName[255];
	char szType[50];
	char szValue[255];
	Variable(){szName[0]=0;szType[0]=0;szValue[0]=0;};
};

struct lua_State;

struct SScriptThread{
//	void*			pScript;
	lua_State		*lua;
	int				scriptID;
	bool			active;
	char			name[255];
	char			process[255];
	SScriptThread():/**pScript(0),/**/lua(0), scriptID(-1),active(false){name[0]=0;process[0]=0;};
	SScriptThread(const SScriptThread& other)
	{
		operator = (other);
	};
	SScriptThread& operator = (const SScriptThread& other){
//		pScript			= other.pScript;
		lua				= other.lua;
		scriptID		= other.scriptID;
		active			= other.active;
		name[0]			=0;
		process[0]	=0;
		xr_strcat(name,other.name);
		xr_strcat(process,other.process);

		return *this;
	}
};


#define DEBUGGER_MAIL_SLOT		"\\\\.\\mailslot\\script_debugger_mailslot"
#define IDE_MAIL_SLOT			"\\\\.\\mailslot\\script_ide_mailslot"

enum dbg_messages{
	_DMSG_FIRST_MSG						=WM_USER+1,
	DMSG_WRITE_DEBUG					,
	DMSG_HAS_BREAKPOINT					,		
	DMSG_GOTO_FILELINE					,
	DMSG_DEBUG_START					,
	DMSG_DEBUG_BREAK					,
	DMSG_DEBUG_END						,
	DMSG_CLEAR_STACKTRACE				,
	DMSG_ADD_STACKTRACE					,
	DMSG_GOTO_STACKTRACE_LEVEL			,
	DMSG_GET_STACKTRACE_LEVEL			,
	DMSG_CLEAR_LOCALVARIABLES			,
	DMSG_ADD_LOCALVARIABLE				,
	DMSG_CLEAR_GLOBALVARIABLES			,
	DMSG_ADD_GLOBALVARIABLE				,
	DMSG_EVAL_WATCH						,
	DMSG_ACTIVATE_IDE					,
	DMSG_DEBUG_STEP_INTO				,
	DMSG_DEBUG_STEP_OVER				,
	DMSG_DEBUG_STEP_OUT					,
	DMSG_DEBUG_RUN_TO_CURSOR			,
	DMSG_STOP_DEBUGGING					,
	DMSG_GOTO_IDE_STACKTRACE_LEVEL		,
	DMSG_NEW_CONNECTION					,
	DMSG_DEBUG_GO						,
	DMSG_GET_BREAKPOINTS				,
	DMSG_CLEAR_THREADS					,
	DMSG_ADD_THREAD						,
	DMSG_THREAD_CHANGED					,
	DMSG_GET_VAR_TABLE					,
	DMSG_CLOSE_CONNECTION				,
										
	_DMSG_LAST_MSG						,
};
