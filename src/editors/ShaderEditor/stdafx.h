//----------------------------------------------------
// file: stdafx.h
//----------------------------------------------------
#ifndef stdafxH
#define stdafxH

#pragma once

#pragma warn -pck

#define sqrtf(a) sqrt(a)

#define smart_cast dynamic_cast

#ifndef O_SEQUENTIAL
#define O_SEQUENTIAL 0
#endif

#define DIRECTINPUT_VERSION 0x0800

#define         R_R1    1
#define         R_R2    2
#define         RENDER  R_R1

// Std C++ headers
#include <fastmath.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <process.h>
#include <utime.h>

// iseful macros
// MSC names for functions
#ifdef _eof
#undef _eof
#endif
__inline int _eof	(int _a)   		                        { return ::eof(_a); }
#ifdef _access
#undef _access
#endif
__inline int _access(const char *_a, int _b)                { return ::access(_a,_b); }
#ifdef _lseek
#undef _lseek
#endif
__inline long _lseek(int handle, long offset, int fromwhere){ return ::lseek(handle, offset, fromwhere);}
#ifdef _dup
#undef _dup
#endif
#define fmodf fmod
__inline int _dup    (int handle)                           { return ::dup(handle);}
__inline float modff(float a, float *b){
	double x,y;
    y = modf(double(a),&x);
    *b = x;
    return float(y);
}
__inline float expf	(float val)                           	{ return ::exp(val);}


#ifdef	_ECOREB
    #define ECORE_API		__declspec(dllexport)
    #define ENGINE_API		__declspec(dllexport)
#else
    #define ECORE_API		__declspec(dllimport)
    #define ENGINE_API		__declspec(dllimport)
#endif

#define DLL_API			__declspec(dllimport)
#define PropertyGP(a,b)	__declspec( property( get=a, put=b ) )
#define THROW			FATAL("THROW");
#define THROW2(a)		FATAL(a);
#define NO_XRC_STATS

#define clMsg 			Msg

// core
#include <xrCore.h>

#ifdef _EDITOR
	class PropValue;
	class PropItem;
	DEFINE_VECTOR(PropItem*,PropItemVec,PropItemIt);

	class ListItem;
	DEFINE_VECTOR(ListItem*,ListItemsVec,ListItemsIt);
#endif

#include "../../xrCDB/xrCDB.h"
#include "../../xrSound/Sound.h"
#include "PSystem.h"

// DirectX headers
#include <d3d9.h>
#include <d3dx9.h>
#include "..\..\Layers\xrRender\xrD3dDefs.h"
#include <dinput.h>
#include <dsound.h>

// some user components
#include "fmesh.h"
#include "_d3d_extensions.h"

#include "../ECore/Editor/D3DX_Wrapper.h"

DEFINE_VECTOR		(AnsiString,AStringVec,AStringIt);
DEFINE_VECTOR		(AnsiString*,LPAStringVec,LPAStringIt);

#include "../../xrServerEntities/xrEProps.h"
#include "Log.h"
#include "../ECore/Editor/engine.h"
#include "defines.h"

struct str_pred : public std::binary_function<char*, char*, bool>
{
    IC bool operator()(LPCSTR x, LPCSTR y) const
    {	return strcmp(x,y)<0;	}
};
struct astr_pred : public std::binary_function<const AnsiString&, const AnsiString&, bool>
{
    IC bool operator()(const AnsiString& x, const AnsiString& y) const
    {	return x<y;	}
};

#ifdef _EDITOR
	#include "../ECore/Editor/device.h"
	#include "properties.h"
	#include "../ECore/Editor/render.h"
	DEFINE_VECTOR(FVF::L,FLvertexVec,FLvertexIt);
	DEFINE_VECTOR(FVF::TL,FTLvertexVec,FTLvertexIt);
	DEFINE_VECTOR(FVF::LIT,FLITvertexVec,FLITvertexIt);
	DEFINE_VECTOR(shared_str,RStrVec,RStrVecIt);

	#include "../ECore/Editor/EditorPreferences.h"
#endif

#ifdef _LEVEL_EDITOR
	#include "net_utils.h"
#endif

#define INI_NAME(buf) 		{FS.update_path(buf,"$local_root$",EFS.ChangeFileExt(UI->EditorName(),".ini").c_str());}
//#define INI_NAME(buf) 		{buf = buf+xr_string(Core.WorkingPath)+xr_string("\\")+EFS.ChangeFileExt(UI->EditorName(),".ini");}
#define DEFINE_INI(storage)	{string_path buf; INI_NAME(buf); storage->IniFileName=buf;}
#define NONE_CAPTION "<none>"
#define MULTIPLESEL_CAPTION "<multiple selection>"

// path definition
#define _server_root_		"$server_root$"
#define _server_data_root_	"$server_data_root$"
#define _local_root_		"$local_root$"
#define _import_			"$import$"
#define _sounds_			"$sounds$"
#define _textures_			"$textures$"
#define _objects_			"$objects$"
#define _maps_				"$maps$"
#define _groups_			"$groups$"
#define _temp_				"$temp$"
#define _omotion_			"$omotion$"
#define _omotions_			"$omotions$"
#define _smotion_			"$smotion$"
#define _detail_objects_	"$detail_objects$"
#endif

#define		TEX_POINT_ATT	"internal\\internal_light_attpoint"
#define		TEX_SPOT_ATT	"internal\\internal_light_attclip"

#pragma hdrstop

