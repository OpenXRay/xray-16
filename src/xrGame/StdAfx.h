#pragma once

#pragma warning(disable:4995)
#include "xrEngine/stdafx.h"
#include "DPlay/dplay8.h"
#pragma warning(default:4995)
#pragma warning( 4 : 4018 )
#pragma warning( 4 : 4244 )
#pragma warning(disable:4505)

#if XRAY_EXCEPTIONS
#	define	THROW(expr)				do {if (!(expr)) {string4096	assertion_info; xrDebug::GatherInfo(assertion_info, DEBUG_INFO, #expr,   0,   0,0); throw assertion_info;}} while(0)
#	define	THROW2(expr,msg0)		do {if (!(expr)) {string4096	assertion_info; xrDebug::GatherInfo(assertion_info, DEBUG_INFO, #expr,msg0,   0,0); throw assertion_info;}} while(0)
#	define	THROW3(expr,msg0,msg1)	do {if (!(expr)) {string4096	assertion_info; xrDebug::GatherInfo(assertion_info, DEBUG_INFO, #expr,msg0,msg1,0); throw assertion_info;}} while(0)
#else
#	define	THROW					VERIFY
#	define	THROW2					VERIFY2
#	define	THROW3					VERIFY3
#endif

#include "xrEngine/gamefont.h"
#include "xrEngine/xr_object.h"
#include "xrEngine/IGame_Level.h"
#include "xrPhysics/xrphysics.h"
#include "xrServerEntities/smart_cast.h"