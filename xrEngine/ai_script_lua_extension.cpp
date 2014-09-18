////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_script_lua_extension.cpp
//	Created 	: 19.09.2003
//  Modified 	: 19.09.2003
//	Author		: Dmitriy Iassenev
//	Description : XRay Script extensions
////////////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include "stdafx.h"
#include "ai_script_lua_extension.h"
#include "ai_script_space.h"

#ifdef XRRENDER_R4_EXPORTS
#define ENGINE_BUILD
#endif	//	XRRENDER_R4_EXPORTS

#ifdef XRRENDER_R3_EXPORTS
#define ENGINE_BUILD
#endif	//	XRRENDER_R3_EXPORTS

#ifdef XRRENDER_R2_EXPORTS
#define ENGINE_BUILD
#endif	//	XRRENDER_R2_EXPORTS

#ifdef XRRENDER_R1_EXPORTS
#define ENGINE_BUILD
#endif	//	XRRENDER_R1_EXPORTS

#ifndef ENGINE_BUILD
	#include "ai_space.h"
#endif

using namespace Script;

int __cdecl Lua::LuaOut(Lua::ELuaMessageType tLuaMessageType, LPCSTR caFormat, ...)
{
#ifndef ENGINE_BUILD
	if (!psAI_Flags.test(aiLua))
		return(0);
#endif

	LPCSTR		S = "", SS = "";
	LPSTR		S1;
	string4096	S2;
	switch (tLuaMessageType) {
		case Lua::eLuaMessageTypeInfo : {
			S	= "* [LUA] ";
			SS	= "[INFO]        ";
			break;
		}
		case Lua::eLuaMessageTypeError : {
			S	= "! [LUA] ";
			SS	= "[ERROR]       ";
			break;
		}
		case Lua::eLuaMessageTypeMessage : {
			S	= "[LUA] ";
			SS	= "[MESSAGE]     ";
			break;
		}
		case Lua::eLuaMessageTypeHookCall : {
			S	= "[LUA][HOOK_CALL] ";
			SS	= "[CALL]        ";
			break;
		}
		case Lua::eLuaMessageTypeHookReturn : {
			S	= "[LUA][HOOK_RETURN] ";
			SS	= "[RETURN]      ";
			break;
		}
		case Lua::eLuaMessageTypeHookLine : {
			S	= "[LUA][HOOK_LINE] ";
			SS	= "[LINE]        ";
			break;
		}
		case Lua::eLuaMessageTypeHookCount : {
			S	= "[LUA][HOOK_COUNT] ";
			SS	= "[COUNT]       ";
			break;
		}
		case Lua::eLuaMessageTypeHookTailReturn : {
			S	= "[LUA][HOOK_TAIL_RETURN] ";
			SS	= "[TAIL_RETURN] ";
			break;
		}
		default : NODEFAULT;
	}
	
	va_list	l_tMarker;
	
	va_start(l_tMarker,caFormat);

	xr_strcpy	(S2,S);
	S1		= S2 + xr_strlen(S);
	int		l_iResult = vsprintf(S1,caFormat,l_tMarker);
	Msg		("%s",S2);
	
	xr_strcpy	(S2,SS);
	S1		= S2 + xr_strlen(SS);
	vsprintf(S1,caFormat,l_tMarker);

#ifdef ENGINE_BUILD
	// Msg("[LUA Output] : %s",S2);
#else
	ai().lua_output().w_string(S2);
#endif

	va_end	(l_tMarker);

	return	(l_iResult);
}

#ifndef ENGINE_BUILD
void Script::vfLoadStandardScripts(CLuaVirtualMachine *tpLuaVM)
{
	string256		S,S1;
	FS.update_path	(S,"$game_data$","script.ltx");
	CInifile		*l_tpIniFile = xr_new<CInifile>(S);
	R_ASSERT		(l_tpIniFile);
	LPCSTR			caScriptString = l_tpIniFile->r_string("common","script");

	u32				caNamespaceName = _GetItemCount(caScriptString);
	string256		I;
	for (u32 i=0; i<caNamespaceName; ++i) {
		FS.update_path(S,"$game_scripts$",strconcat(S1,_GetItem(caScriptString,i,I),".script"));
		bfLoadFile	(tpLuaVM,S,true);
		if (bfIsObjectPresent(tpLuaVM,"_G",xr_strcat(I,"_initialize"),LUA_TFUNCTION))
			lua_dostring(tpLuaVM,xr_strcat(I,"()"));
	}
	xr_delete		(l_tpIniFile);
}

void LuaError(lua_State* L)
{
	Debug.fatal(DEBUG_INFO,"LUA error: %s",lua_tostring(L,-1));
}

void Script::vfExportToLua(CLuaVirtualMachine *tpLuaVM)
{
	luabind::open					(tpLuaVM);
	luabind::set_error_callback		(LuaError);

	lua_atpanic		(tpLuaVM,Script::LuaPanic);

	vfExportGlobals				(tpLuaVM);
	vfExportFvector				(tpLuaVM);
	vfExportFmatrix				(tpLuaVM);
	vfExportGame				(tpLuaVM);
	vfExportLevel				(tpLuaVM);
	vfExportDevice				(tpLuaVM);
	vfExportParticles			(tpLuaVM);
	vfExportSound				(tpLuaVM);
	vfExportHit					(tpLuaVM);
	vfExportActions				(tpLuaVM);
	vfExportObject				(tpLuaVM);
	vfExportEffector			(tpLuaVM);
	vfExportArtifactMerger		(tpLuaVM);
	vfExportMemoryObjects		(tpLuaVM);
	vfExportActionManagement	(tpLuaVM);
	vfExportMotivationManagement(tpLuaVM);

#ifdef DEBUG
	lua_sethook					(tpLuaVM, LuaHookCall,	LUA_HOOKCALL | LUA_HOOKRET | LUA_HOOKLINE | LUA_HOOKTAILRET,	0);
#endif

	vfLoadStandardScripts		(tpLuaVM);
}

bool Script::bfLoadFile(CLuaVirtualMachine *tpLuaVM, LPCSTR caScriptName, bool bCall)
{
	string256		l_caNamespaceName;
	_splitpath		(caScriptName,0,0,l_caNamespaceName,0);
	if (!xr_strlen(l_caNamespaceName))
		return		(bfLoadFileIntoNamespace(tpLuaVM,caScriptName,"_G",bCall));
	else
		return		(bfLoadFileIntoNamespace(tpLuaVM,caScriptName,l_caNamespaceName,bCall));
}
#endif

bool bfCreateNamespaceTable(CLuaVirtualMachine *tpLuaVM, LPCSTR caNamespaceName)
{
	lua_pushstring	(tpLuaVM,"_G");
	lua_gettable	(tpLuaVM,LUA_GLOBALSINDEX);
	LPSTR			S2	= xr_strdup(caNamespaceName);
	LPSTR			S	= S2;
	for (;;) {
		if (!xr_strlen(S)) {
			lua_pop		(tpLuaVM,1);
			LuaOut		(Lua::eLuaMessageTypeError,"the namespace name %s is incorrect!",caNamespaceName);
			xr_free		(S2);
			return		(false);
		}
		LPSTR			S1 = strchr(S,'.');
		if (S1)
			*S1				= 0;
		lua_pushstring	(tpLuaVM,S);
		lua_gettable	(tpLuaVM,-2);
		if (lua_isnil(tpLuaVM,-1)) {
			lua_pop			(tpLuaVM,1);
			lua_newtable	(tpLuaVM);
			lua_pushstring	(tpLuaVM,S);
			lua_pushvalue	(tpLuaVM,-2);
			lua_settable	(tpLuaVM,-4);
		}
		else
			if (!lua_istable(tpLuaVM,-1)) {
				xr_free			(S2);
				lua_pop			(tpLuaVM,2);
				LuaOut			(Lua::eLuaMessageTypeError,"the namespace name %s is already being used by the non-table object!",caNamespaceName);
				return			(false);
			}
			lua_remove		(tpLuaVM,-2);
			if (S1)
				S			= ++S1;
			else
				break;
	}
	xr_free			(S2);
	return			(true);
}

void vfCopyGlobals(CLuaVirtualMachine *tpLuaVM)
{
	lua_newtable	(tpLuaVM);
	lua_pushstring	(tpLuaVM,"_G");
	lua_gettable	(tpLuaVM,LUA_GLOBALSINDEX);
	lua_pushnil		(tpLuaVM);
	while (lua_next(tpLuaVM, -2)) {
		lua_pushvalue	(tpLuaVM,-2);
		lua_pushvalue	(tpLuaVM,-2);
		lua_settable	(tpLuaVM,-6);
		lua_pop			(tpLuaVM, 1);
	}
}

bool Script::bfLoadBuffer(CLuaVirtualMachine *tpLuaVM, LPCSTR caBuffer, size_t tSize, LPCSTR caScriptName, LPCSTR caNameSpaceName)
{
	int				l_iErrorCode;
	if (caNameSpaceName) {
		string256		insert;
		xr_sprintf		(insert,sizeof(insert),"local this = %s\n",caNameSpaceName);
		size_t			str_len = xr_strlen(insert);
		LPSTR			script = xr_alloc<char>(u32(str_len + tSize));
		xr_strcpy		(script, str_len+tSize, insert);
		CopyMemory		(script+str_len, caBuffer, u32(tSize));
		l_iErrorCode	= luaL_loadbuffer(tpLuaVM,script,tSize + str_len,caScriptName);
		xr_free			(script);
	}
	else
		l_iErrorCode	= luaL_loadbuffer(tpLuaVM,caBuffer,tSize,caScriptName);

	if (l_iErrorCode) {
#ifdef DEBUG
		if (!bfPrintOutput	(tpLuaVM,caScriptName,l_iErrorCode))
			vfPrintError(tpLuaVM,l_iErrorCode);
#endif
		return			(false);
	}
	return			(true);
}

bool bfDoFile(CLuaVirtualMachine *tpLuaVM, LPCSTR caScriptName, LPCSTR caNameSpaceName, bool bCall)
{
	string_path		l_caLuaFileName;
	IReader			*l_tpFileReader = FS.r_open(caScriptName);
	R_ASSERT		(l_tpFileReader);
	strconcat		(sizeof(l_caLuaFileName),l_caLuaFileName,"@",caScriptName);
	
	if (!bfLoadBuffer(tpLuaVM,static_cast<LPCSTR>(l_tpFileReader->pointer()),(size_t)l_tpFileReader->length(),l_caLuaFileName,caNameSpaceName)) {
		lua_pop			(tpLuaVM,4);
		FS.r_close		(l_tpFileReader);
		return		(false);
	}
	FS.r_close		(l_tpFileReader);

	if (bCall) {
		lua_call	(tpLuaVM,0,0);
//		int			l_iErrorCode = lua_pcall(tpLuaVM,0,0,0);
//		if (l_iErrorCode) {
//#ifdef DEBUG
//			bfPrintOutput	(tpLuaVM,caScriptName,l_iErrorCode);
//			vfPrintError	(tpLuaVM,l_iErrorCode);
//#endif
//			return	(false);
//		}
	}
	else
		lua_insert		(tpLuaVM,-4);

	return			(true);
}

void vfSetNamespace(CLuaVirtualMachine *tpLuaVM)
{
	lua_pushnil		(tpLuaVM);
	while (lua_next(tpLuaVM, -2)) {
		lua_pushvalue	(tpLuaVM,-2);
		lua_gettable	(tpLuaVM,-5);
		if (lua_isnil(tpLuaVM,-1)) {
			lua_pop			(tpLuaVM,1);
			lua_pushvalue	(tpLuaVM,-2);
			lua_pushvalue	(tpLuaVM,-2);
			lua_pushvalue	(tpLuaVM,-2);
			lua_pushnil		(tpLuaVM);
			lua_settable	(tpLuaVM,-7);
			lua_settable	(tpLuaVM,-7);
		}
		else {
			lua_pop			(tpLuaVM,1);
			lua_pushvalue	(tpLuaVM,-2);
			lua_gettable	(tpLuaVM,-4);
			if (!lua_equal(tpLuaVM,-1,-2)) {
				lua_pushvalue	(tpLuaVM,-3);
				lua_pushvalue	(tpLuaVM,-2);
				lua_pushvalue	(tpLuaVM,-2);
				lua_pushvalue	(tpLuaVM,-5);
				lua_settable	(tpLuaVM,-8);
				lua_settable	(tpLuaVM,-8);
			}
			lua_pop			(tpLuaVM,1);
		}
		lua_pushvalue	(tpLuaVM,-2);
		lua_pushnil		(tpLuaVM);
		lua_settable	(tpLuaVM,-6);
		lua_pop			(tpLuaVM, 1);
	}
	lua_pop			(tpLuaVM,3);
}

bool Script::bfLoadFileIntoNamespace(CLuaVirtualMachine *tpLuaVM, LPCSTR caScriptName, LPCSTR caNamespaceName, bool bCall)
{
	if (!bfCreateNamespaceTable(tpLuaVM,caNamespaceName))
		return		(false);
	vfCopyGlobals	(tpLuaVM);
	if (!bfDoFile(tpLuaVM,caScriptName,caNamespaceName,bCall))
		return		(false);
	vfSetNamespace	(tpLuaVM);
	return			(true);
}

bool Script::bfGetNamespaceTable(CLuaVirtualMachine *tpLuaVM, LPCSTR N)
{
	lua_pushstring 		(tpLuaVM,"_G"); 
	lua_gettable 		(tpLuaVM,LUA_GLOBALSINDEX); 
	string256			S2;	xr_strcpy	(S2,N);
	LPSTR				S	= S2;
	for (;;) { 
		if (!xr_strlen(S)) return	(false); 
		LPSTR S1 		= strchr(S,'.'); 
		if (S1) 	*S1 = 0; 
		lua_pushstring 	(tpLuaVM,S); 
		lua_gettable 	(tpLuaVM,-2); 
		if (lua_isnil(tpLuaVM,-1)) { 
			lua_pop		(tpLuaVM,2); 
			return		(false);	//	there is no namespace!
		}
		else 
			if (!lua_istable(tpLuaVM,-1)) { 
				lua_pop		(tpLuaVM,2); 
				FATAL		(" Error : the namespace name is already being used by the non-table object!\n");
				return		(false); 
			} 
			lua_remove	(tpLuaVM,-2); 
			if (S1)		S = ++S1; 
			else 		break; 
	} 
	return	(true); 
}

CLuaVirtualMachine *Script::get_namespace_table(CLuaVirtualMachine *tpLuaVM, LPCSTR N)
{
	if (!xr_strlen(N))
		return				(tpLuaVM);
	lua_pushstring 			(tpLuaVM,"_G"); 
	lua_gettable 			(tpLuaVM,LUA_GLOBALSINDEX); 
	string256				S2;
	xr_strcpy					(S2,N);
	LPSTR					S	= S2;
	for (;;) { 
		if (!xr_strlen(S))
			return			(0); 
		LPSTR				S1 = strchr(S,'.'); 
		if (S1)
			*S1				= 0; 
		lua_pushstring 		(tpLuaVM,S); 
		lua_gettable 		(tpLuaVM,-2); 
		if (lua_isnil(tpLuaVM,-1)) { 
			lua_pop			(tpLuaVM,2); 
			return			(0);	//	there is no namespace!
		}
		else 
			if (!lua_istable(tpLuaVM,-1)) { 
				lua_pop		(tpLuaVM,2); 
				FATAL		(" Error : the namespace name is already being used by the non-table object!\n");
				return		(0); 
			} 

			lua_remove		(tpLuaVM,-2);

			if (S1)
				S			= ++S1; 
			else
				break; 
	} 
	return					(tpLuaVM); 
}

bool	Script::bfIsObjectPresent	(CLuaVirtualMachine *tpLuaVM, LPCSTR identifier, int type)
{
	lua_pushnil (tpLuaVM); 
	while (lua_next(tpLuaVM, -2)) { 
		if ((lua_type(tpLuaVM, -1) == type) && !xr_strcmp(identifier,lua_tostring(tpLuaVM, -2))) { 
			lua_pop (tpLuaVM, 3); 
			return	(true); 
		} 
		lua_pop (tpLuaVM, 1); 
	} 
	lua_pop (tpLuaVM, 1); 
	return	(false); 
}

bool	Script::bfIsObjectPresent	(CLuaVirtualMachine *tpLuaVM, LPCSTR namespace_name, LPCSTR identifier, int type)
{
	if (xr_strlen(namespace_name) && !bfGetNamespaceTable(tpLuaVM,namespace_name))
		return				(false); 
	return					(bfIsObjectPresent(tpLuaVM,identifier,type)); 
}

luabind::object Script::lua_namespace_table(CLuaVirtualMachine *tpLuaVM, LPCSTR namespace_name)
{
	string256			S1;
	xr_strcpy				(S1,namespace_name);
	LPSTR				S = S1;
	luabind::object		lua_namespace = luabind::get_globals(tpLuaVM);
	for (;;) {
		if (!xr_strlen(S))
			return		(lua_namespace);
		LPSTR			I = strchr(S,'.');
		if (!I)
			return		(lua_namespace[S]);
		*I				= 0;
		lua_namespace	= lua_namespace[S];
		S				= I + 1;
	}
}