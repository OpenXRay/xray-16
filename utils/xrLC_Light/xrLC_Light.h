#ifndef	_XRLC_LIGHT_H_
#define	_XRLC_LIGHT_H_
#pragma once

#include "../../xrCore/xrCore.h"
#ifdef XRLC_LIGHT_EXPORTS
#	define XRLC_LIGHT_API __declspec(dllexport)
#else
#	define XRLC_LIGHT_API __declspec(dllimport)
#endif


#pragma warning(disable:4995)
#include <commctrl.h>
#include <d3dx9.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#pragma warning(default:4995)

//#include "cl_log.h"
#include "_d3d_extensions.h"
#include "../../editors/LevelEditor/Engine/communicate.h"
//#include "Etextureparams.h"

static const int	edge2idx3	[3][3]	= { {0,1,2},	{1,2,0},	{2,0,1}	};
static const int	edge2idx	[3][2]	= { {0,1},		{1,2},		{2,0}	};
static const int	idx2edge	[3][3]  = {
	{-1,  0,  2},
	{ 0, -1,  1},
	{ 2,  1, -1}
};
extern XRLC_LIGHT_API bool g_using_smooth_groups;
extern XRLC_LIGHT_API bool g_smooth_groups_by_faces;

extern XRLC_LIGHT_API xr_pure_interface  XRLC_LIGHT_API i_lc_log 
{
	virtual void clMsg		( LPCSTR msg )			=0;
	virtual void clLog		( LPCSTR msg )			=0;
	virtual void Status		( LPCSTR msg )			=0;
	virtual	void Progress	( const float F )		=0;
	virtual	void Phase		( LPCSTR phase_name )	=0;
} *lc_log;

			XRLC_LIGHT_API void	xrCompileDO		 ( bool net );
extern "C"	XRLC_LIGHT_API  b_params	&g_params();

IC	u8	u8_clr				(float a)	{ s32 _a = iFloor(a*255.f); clamp(_a,0,255); return u8(_a);		};

#ifndef	NDEBUG
#define X_TRY 
#define X_CATCH if (0)
#else
#define X_TRY try
#define X_CATCH catch(...)
#endif

#endif

