//----------------------------------------------------
// file: DrawUtils.h
//----------------------------------------------------

#pragma once
#ifndef DrawUtilsH
#define DrawUtilsH
#include "xrCore/xr_types.h"
// tamlin WIP
//#include "xrCore/_vector3d.h"

#ifndef ECORE_API
#pragma message("ECORE_API not defined. Invoking ugly hack to enable compilation.")
#define ECORE_API
#endif

template <class T>
struct _vector3;
typedef _vector3<float> Fvector;

template <class T>
struct _obb;
typedef _obb<float> Fobb;

//----------------------------------------------------
// Utilities
//----------------------------------------------------
class ECORE_API CDUInterface
{
public:
    //----------------------------------------------------
    virtual void DrawCross(const Fvector& p, float szx1, float szy1, float szz1, float szx2, float szy2,
        float szz2, u32 clr, BOOL bRot45 = false) = 0;
    virtual void DrawCross(const Fvector& p, float sz, u32 clr, BOOL bRot45 = false) = 0;
    virtual void DrawFlag(
        const Fvector& p, float heading, float height, float sz, float sz_fl, u32 clr, BOOL bDrawEntity) = 0;
    virtual void DrawRomboid(const Fvector& p, float radius, u32 clr) = 0;
    virtual void DrawJoint(const Fvector& p, float radius, u32 clr) = 0;

    virtual void DrawSpotLight(const Fvector& p, const Fvector& d, float range, float phi, u32 clr) = 0;
    virtual void DrawDirectionalLight(
        const Fvector& p, const Fvector& d, float radius, float range, u32 clr) = 0;
    virtual void DrawPointLight(const Fvector& p, float radius, u32 clr) = 0;

    virtual void DrawSound(const Fvector& p, float radius, u32 clr) = 0;
    virtual void DrawLineSphere(const Fvector& p, float radius, u32 clr, BOOL bCross) = 0;

    virtual void dbgDrawPlacement(
        const Fvector& p, int sz, u32 clr, LPCSTR caption = 0, u32 clr_font = 0xffffffff) = 0;
    virtual void dbgDrawVert(const Fvector& p0, u32 clr, LPCSTR caption = 0) = 0;
    virtual void dbgDrawEdge(const Fvector& p0, const Fvector& p1, u32 clr, LPCSTR caption = 0) = 0;
    virtual void dbgDrawFace(
        const Fvector& p0, const Fvector& p1, const Fvector& p2, u32 clr, LPCSTR caption = 0) = 0;

    virtual void DrawFace(
        const Fvector& p0, const Fvector& p1, const Fvector& p2, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawLine(const Fvector& p0, const Fvector& p1, u32 clr) = 0;
    virtual void DrawLink(const Fvector& p0, const Fvector& p1, float sz, u32 clr) = 0;
    virtual void DrawFaceNormal(
        const Fvector& p0, const Fvector& p1, const Fvector& p2, float size, u32 clr) = 0;
    virtual void DrawFaceNormal(const Fvector* p, float size, u32 clr) = 0;
    virtual void DrawFaceNormal(const Fvector& C, const Fvector& N, float size, u32 clr) = 0;
    virtual void DrawSelectionBox(const Fvector& center, const Fvector& size, u32* c = 0) = 0;
    virtual void DrawSelectionBoxB(const Fbox& box, u32* c = 0) = 0;
    virtual void DrawIdentSphere(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
    virtual void DrawIdentSpherePart(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
    virtual void DrawIdentCone(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
    virtual void DrawIdentCylinder(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
    virtual void DrawIdentBox(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;

    virtual void DrawBox(
        const Fvector& offs, const Fvector& Size, BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) = 0;
    virtual void DrawAABB(
        const Fvector& p0, const Fvector& p1, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawAABB(const Fmatrix& parent, const Fvector& center, const Fvector& size, u32 clr_s,
        u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawOBB(const Fmatrix& parent, const Fobb& box, u32 clr_s, u32 clr_w) = 0;
    virtual void DrawSphere(
        const Fmatrix& parent, const Fvector& center, float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawSphere(
        const Fmatrix& parent, const Fsphere& S, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawCylinder(const Fmatrix& parent, const Fvector& center, const Fvector& dir, float height,
        float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawCone(const Fmatrix& parent, const Fvector& apex, const Fvector& dir, float height,
        float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawPlane(const Fvector& center, const Fvector2& scale, const Fvector& rotate, u32 clr_s,
        u32 clr_w, BOOL bCull, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawPlane(const Fvector& p, const Fvector& n, const Fvector2& scale, u32 clr_s, u32 clr_w,
        BOOL bCull, BOOL bSolid, BOOL bWire) = 0;
    virtual void DrawRectangle(
        const Fvector& o, const Fvector& u, const Fvector& v, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) = 0;

    virtual void DrawGrid() = 0;
    virtual void DrawPivot(const Fvector& pos, float sz = 5.f) = 0;
    virtual void DrawAxis(const Fmatrix& T) = 0;
    virtual void DrawObjectAxis(const Fmatrix& T, float sz, BOOL sel) = 0;
    virtual void DrawSelectionRect(const Ivector2& m_SelStart, const Ivector2& m_SelEnd) = 0;

    virtual void DrawIndexedPrimitive(int prim_type, u32 pc, const Fvector& pos, const Fvector* vb,
        const u32& vb_size, const u32* ib, const u32& ib_size, const u32& clr_argb, float scale = 1.0f) = 0;

    virtual void OutText(
        const Fvector& pos, LPCSTR text, u32 color = 0xFF000000, u32 shadow_color = 0xFF909090) = 0;

    virtual void OnDeviceDestroy() = 0;
};
//----------------------------------------------------
#endif
