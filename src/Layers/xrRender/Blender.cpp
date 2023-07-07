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

#if defined(XR_PLATFORM_WINDOWS) // TODO Implement for Linux
    _tzset();
    _time32((__time32_t*)&cTime); // Time
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
    // XXX: there was a bLighting variable
    // which was set to false in 'if' path
    // and set to true in 'else' path
    // but it was ignored anyway in the SetParams ¯\_(ツ)_/¯.
    // Need to research commits from 2003 in xray-soc-history more
    if (!ps_r1_flags.is_any(R1FLAG_FFP_LIGHTMAPS | R1FLAG_DLIGHTS))
        C.SetParams(oPriority.value, oStrictSorting.value ? true : false);
    else
        C.SetParams(oPriority.value, oStrictSorting.value ? true : false);
}

#ifndef _EDITOR
IBlender* IBlender::Create(CLASS_ID cls)
{
    return ::RImplementation.blender_create(cls);
}

void IBlender::Destroy(IBlender*& B)
{
    ::RImplementation.blender_destroy(B);
}
#else

// Editor
#include "blenders/BlenderDefault.h"
#include "blenders/Blender_default_aref.h"
#include "blenders/Blender_Vertex.h"
#include "blenders/Blender_Vertex_aref.h"
#include "blenders/Blender_Screen_SET.h"
#include "blenders/Blender_Screen_GRAY.h"
#include "blenders/Blender_Editor_Wire.h"
#include "blenders/Blender_Editor_Selection.h"
#include "blenders/Blender_LaEmB.h"
#include "blenders/Blender_Lm(EbB).h"
#include "blenders/Blender_BmmD.h"
#include "blenders/Blender_Model.h"
#include "blenders/Blender_Model_EbB.h"
#include "blenders/Blender_detail_still.h"
#include "blenders/Blender_tree.h"
#include "blenders/Blender_Particle.h"

IBlender* IBlender::Create(CLASS_ID cls)
{
    switch (cls)
    {
    case B_DEFAULT: return xr_new<CBlender_default>();
    case B_DEFAULT_AREF: return xr_new<CBlender_default_aref>();
    case B_VERT: return xr_new<CBlender_Vertex>();
    case B_VERT_AREF: return xr_new<CBlender_Vertex_aref>();
    case B_SCREEN_SET: return xr_new<CBlender_Screen_SET>();
    case B_SCREEN_GRAY: return xr_new<CBlender_Screen_GRAY>();
    case B_EDITOR_WIRE: return xr_new<CBlender_Editor_Wire>();
    case B_EDITOR_SEL: return xr_new<CBlender_Editor_Selection>();
    case B_LaEmB: return xr_new<CBlender_LaEmB>();
    case B_LmEbB: return xr_new<CBlender_LmEbB>();
    case B_BmmD: return xr_new<CBlender_BmmD>();
    case B_MODEL: return xr_new<CBlender_Model>();
    case B_MODEL_EbB: return xr_new<CBlender_Model_EbB>();
    case B_DETAIL: return xr_new<CBlender_Detail_Still>();
    case B_TREE: return xr_new<CBlender_Tree>();
    case B_PARTICLE: return xr_new<CBlender_Particle>();
    }
    return 0;
}
void IBlender::Destroy(IBlender*& B) { xr_delete(B); }

#endif
