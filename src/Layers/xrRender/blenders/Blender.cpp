// Blender.cpp: implementation of the IBlender class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#define _USE_32BIT_TIME_T
#include <time.h>

#include "Blender.h"

void CBlender_DESC::Setup(LPCSTR N)
{
    // Name
    VERIFY(xr_strlen(N) < 128);
    VERIFY(nullptr == strchr(N, '.'));
    xr_strcpy(cName, N);
    xr_strlwr(cName);

    xr_strcpy(cComputer, Core.CompName); // Computer
#ifndef _EDITOR
#if defined(WINDOWS) // TODO Implement for Linux
    _tzset();
    _time32((__time32_t*)&cTime); // Time
#endif
#else
    _tzset();
    time((long*)&cTime); // Time
#endif
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IBlender::IBlender()
{
    oPriority.min = 0;
    oPriority.max = 3;
    oPriority.value = 1;
    xr_strcpy(oT_Name, "$base0");
    xr_strcpy(oT_xform, "$null");
}

IBlender::~IBlender() {}
void IBlender::Save(IWriter& fs)
{
    fs.w(&description, sizeof(description));
    xrPWRITE_MARKER(fs, "General");
    xrPWRITE_PROP(fs, "Priority", xrPID_INTEGER, oPriority);
    xrPWRITE_PROP(fs, "Strict sorting", xrPID_BOOL, oStrictSorting);
    xrPWRITE_MARKER(fs, "Base Texture");
    xrPWRITE_PROP(fs, "Name", xrPID_TEXTURE, oT_Name);
    xrPWRITE_PROP(fs, "Transform", xrPID_MATRIX, oT_xform);
}

void IBlender::Load(IReader& fs, u16)
{
    // Read desc and doesn't change version
    u16 V = description.version;
    fs.r(&description, sizeof(description));
    description.version = V;

    // Properties
    xrPREAD_MARKER(fs);
    xrPREAD_PROP(fs, xrPID_INTEGER, oPriority);
    xrPREAD_PROP(fs, xrPID_BOOL, oStrictSorting);
    xrPREAD_MARKER(fs);
    xrPREAD_PROP(fs, xrPID_TEXTURE, oT_Name);
    xrPREAD_PROP(fs, xrPID_MATRIX, oT_xform);
}

void IBlender::Compile(CBlender_Compile& C)
{
    if (C.bEditor)
        C.SetParams(oPriority.value, oStrictSorting.value ? true : false);
    else
        C.SetParams(oPriority.value, oStrictSorting.value ? true : false);
}
