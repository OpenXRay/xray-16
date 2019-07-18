#pragma once

#include "xrCore/xrCore.h"
#ifdef XRLC_LIGHT_EXPORTS
#define XRLC_LIGHT_API XR_EXPORT
#else
#define XRLC_LIGHT_API XR_IMPORT
#endif

#pragma warning(push)
#pragma warning(disable : 4995)
#include <commctrl.h>
#include <d3dx9.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#pragma warning(pop)

#include "Common/_d3d_extensions.h"
#include "utils/communicate.h"

static const int edge2idx3[3][3] = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1}};
static const int edge2idx[3][2] = {{0, 1}, {1, 2}, {2, 0}};
static const int idx2edge[3][3] = {{-1, 0, 2}, {0, -1, 1}, {2, 1, -1}};
extern XRLC_LIGHT_API bool g_using_smooth_groups;
extern XRLC_LIGHT_API bool g_smooth_groups_by_faces;

XRLC_LIGHT_API void xrCompileDO(bool net);
extern "C" XRLC_LIGHT_API b_params& g_params();

IC u8 u8_clr(float a)
{
    s32 _a = iFloor(a * 255.f);
    clamp(_a, 0, 255);
    return u8(_a);
};

#ifndef NDEBUG
#define X_TRY
#define X_CATCH if (0)
#else
#define X_TRY try
#define X_CATCH catch (...)
#endif
