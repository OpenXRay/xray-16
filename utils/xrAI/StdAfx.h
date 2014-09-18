// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__81632403_DFD8_4A42_A4D3_0AFDD8EA0D25__INCLUDED_)
#define AFX_STDAFX_H__81632403_DFD8_4A42_A4D3_0AFDD8EA0D25__INCLUDED_7

#pragma once

#include "../../xrCore/xrCore.h"

#pragma warning(disable:4995)
#include <d3dx9.h>
#include <commctrl.h>
#pragma warning(default:4995)

#define ENGINE_API
#define ECORE_API
#define XR_EPROPS_API
#include "../../xrcore/clsid.h"
#include "defines.h"
#include "cl_log.h"
#include "../../xrcdb/xrCDB.h"
#include "_d3d_extensions.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

#ifdef AI_COMPILER
#	include "../../xrServerEntities/smart_cast.h"
#endif
// TODO: reference additional headers your program requires here

#define READ_IF_EXISTS(ltx,method,section,name,default_value)\
	(ltx->line_exist(section,name)) ? ltx->method(section,name) : default_value

#undef		THROW

#if XRAY_EXCEPTIONS
IC	xr_string string2xr_string(LPCSTR s) {return s ? s : "";}
#	define	THROW(xpr)				if (!(xpr)) {throw __FILE__LINE__"\""#xpr"\"";}
#	define	THROW2(xpr,msg0)		if (!(xpr)) {throw *shared_str(xr_string(__FILE__LINE__).append(" \"").append(#xpr).append(string2xr_string(msg0)).c_str());}
#	define	THROW3(xpr,msg0,msg1)	if (!(xpr)) {throw *shared_str(xr_string(__FILE__LINE__).append(" \"").append(#xpr).append(string2xr_string(msg0)).append(", ").append(string2xr_string(msg1)).c_str());}
#else
#	define	THROW					VERIFY
#	define	THROW2					VERIFY2
#	define	THROW3					VERIFY3
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__81632403_DFD8_4A42_A4D3_0AFDD8EA0D25__INCLUDED_7)
