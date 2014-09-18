////////////////////////////////////////////////////////////////////////////
//	Module 		: stdafx.h
//	Created 	: 18.06.2004
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Precompiled header creator
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../xrCore/xrCore.h"

#define STRINGIZE(a)			#a
#define UP(a)					STRINGIZE(../../##a)
//#undef	STRINGIZE

#define WIN32_LEAN_AND_MEAN

#define ENGINE_API
#define ECORE_API
#define DLL_API					__declspec(dllexport)
#define TIXML_USE_STL

#include "clsid_game.h"

namespace std { class exception; }
namespace boost { void throw_exception( std::exception const& A ); }

#include "smart_cast.h"

#define READ_IF_EXISTS(ltx,method,section,name,default_value)\
	(ltx->line_exist(section,name)) ? ltx->method(section,name) : default_value

#if XRAY_EXCEPTIONS
IC	xr_string string2xr_string(LPCSTR s) {return s ? s : "";}
#	define	THROW(expr)				do {if (!(expr)) {string4096	assertion_info; ::Debug.gather_info(_TRE(#expr),   0,   0,0,DEBUG_INFO,assertion_info); throw assertion_info;}} while(0)
#	define	THROW2(expr,msg0)		do {if (!(expr)) {string4096	assertion_info; ::Debug.gather_info(_TRE(#expr),msg0,   0,0,DEBUG_INFO,assertion_info); throw assertion_info;}} while(0)
#	define	THROW3(expr,msg0,msg1)	do {if (!(expr)) {string4096	assertion_info; ::Debug.gather_info(_TRE(#expr),msg0,msg1,0,DEBUG_INFO,assertion_info); throw assertion_info;}} while(0)
#else
#	define	THROW					VERIFY
#	define	THROW2					VERIFY2
#	define	THROW3					VERIFY3
#endif
