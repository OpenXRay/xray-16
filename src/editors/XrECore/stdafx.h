//----------------------------------------------------
// file: stdafx.h
//----------------------------------------------------
#pragma once

#define smart_cast dynamic_cast

#ifndef O_SEQUENTIAL
#define O_SEQUENTIAL 0
#endif

#define DIRECTINPUT_VERSION 0x0800

#define R_R1 1
#define R_R2 2
#define RENDER R_R1

// Std C++ headers

// iseful macros
// MSC names for functions
#ifdef XRECORE_EXPORTS
#define ECORE_API __declspec(dllexport)
#define ENGINE_API __declspec(dllexport)
#else
#define ECORE_API __declspec(dllimport)
#define ENGINE_API __declspec(dllimport)
#endif

#define DLL_API __declspec(dllimport)
#define PropertyGP(a, b) __declspec(property(get = a, put = b))
#define THROW FATAL("THROW");
#define THROW2(a) FATAL(a);
#define NO_XRC_STATS

#define clMsg Msg

// core
#include "..\..\XrCore\xrCore.h"
#define time_t __time32_t
#include "..\XrEProps\stdafx.h"
#ifdef _EDITOR
class PropValue;
class PropItem;
DEFINE_VECTOR(PropItem *, PropItemVec, PropItemIt);

class ListItem;
DEFINE_VECTOR(ListItem *, ListItemsVec, ListItemsIt);
#endif

#include "..\..\xrCDB\xrCDB.h"
#include "..\..\xrSound\Sound.h"
#include "..\..\XrEngine\psystem.h"

// DirectX headers
#include <d3d9.h>
#include "d3dx9.h"
#include "..\..\XrRender\Private\xrD3dDefs.h"

#include <dinput.h>
//#include <dsound.h>

// some user components
#include "..\..\XrEngine\fmesh.h"
#include "..\..\XrEngine\_d3d_extensions.h"

DEFINE_VECTOR(xr_string, AStringVec, AStringIt);
DEFINE_VECTOR(xr_string *, LPAStringVec, LPAStringIt);

#include "..\..\xrServerEntities\xrEProps.h"
#include "..\..\xrCore\Log.h"
#include "editor\engine.h"
#include "..\..\XrEngine\defines.h"

#include "../../xrphysics/xrphysics.h"
#include "..\..\XrRender\Private\FVF.h"

struct str_pred
{
	IC bool operator()(LPCSTR x, LPCSTR y) const
	{
		return strcmp(x, y) < 0;
	}
};
struct astr_pred
{
	IC bool operator()(const xr_string &x, const xr_string &y) const
	{
		return x < y;
	}
};

#ifdef _EDITOR
#include "editor\device.h"
#include "..\..\XrEngine\properties.h"
#include "editor\render.h"
DEFINE_VECTOR(FVF::L, FLvertexVec, FLvertexIt);
DEFINE_VECTOR(FVF::TL, FTLvertexVec, FTLvertexIt);
DEFINE_VECTOR(FVF::LIT, FLITvertexVec, FLITvertexIt);
DEFINE_VECTOR(shared_str, RStrVec, RStrVecIt);

#include "Editor/EditorPreferences.h"
#endif

#ifdef _LEVEL_EDITOR
#include "net_utils.h"
#endif

#define INI_NAME(buf)                                                                             \
	{                                                                                             \
		FS.update_path(buf, "$local_root$", EFS.ChangeFileExt(UI->EditorName(), ".ini").c_str()); \
	}
//#define INI_NAME(buf) 		{buf = buf+xr_string(Core.WorkingPath)+xr_string("\\")+EFS.ChangeFileExt(UI->EditorName(),".ini");}
#define DEFINE_INI(storage)         \
	{                               \
		string_path buf;            \
		INI_NAME(buf);              \
		storage->IniFileName = buf; \
	}
#define NONE_CAPTION "<none>"
#define MULTIPLESEL_CAPTION "<multiple selection>"

// path definition
#define _server_root_ "$server_root$"
#define _server_data_root_ "$server_data_root$"
#define _local_root_ "$local_root$"
#define _import_ "$import$"
#define _sounds_ "$sounds$"
#define _textures_ "$textures$"
#define _objects_ "$objects$"
#define _maps_ "$maps$"
#define _groups_ "$groups$"
#define _temp_ "$temp$"
#define _omotion_ "$omotion$"
#define _omotions_ "$omotions$"
#define _smotion_ "$smotion$"
#define _detail_objects_ "$detail_objects$"

#define TEX_POINT_ATT "internal\\internal_light_attpoint"
#define TEX_SPOT_ATT "internal\\internal_light_attclip"

#include "..\..\XrRender\Private\ETextureParams.h"
#include "..\..\XrRender\Private\ResourceManager.h"
#include "Editor/ImageManager.h"
inline xr_string ChangeFileExt(const char *name, const char *e)
{
	string_path path;
	xr_strcpy(path, name);
	if (strrchr(path, '.'))
	{
		strrchr(path, '.')[0] = 0;
	}
	xr_string str;
	str.append(path);
	str.append(e);
	return str;
}
inline xr_string ChangeFileExt(const xr_string &name, const char *e)
{
	string_path path;
	xr_strcpy(path, name.c_str());
	if (strrchr(path, '.'))
	{
		strrchr(path, '.')[0] = 0;
	}
	xr_string str;
	str.append(path);
	str.append(e);
	return str;
}
inline u32 TColor(u32 r)
{
	return r;
}
#ifdef XRECORE_EXPORTS
inline void not_implemented()
{
	if (IsDebuggerPresent())
		DebugBreak();
	else
	{
		R_ASSERT(0);
	}
}
#endif
