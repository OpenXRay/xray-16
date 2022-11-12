#pragma once

#include "Include/xrRender/FactoryPtr.h"
#include "Include/xrRender/UIRender.h"
#include "Include/xrRender/UIShader.h"
#include "xrCore/_plane2.h"
#include "xrCore/_vector2.h"
#include "xrCore/_rect.h"
#include "xrCore/FixedVector.h"
#include "xrCDB/Frustum.h"

#ifdef XRAY_STATIC_BUILD
#   define XRUICORE_API
#else
#   ifdef XRUICORE_EXPORTS
#      define XRUICORE_API XR_EXPORT
#   else
#      define XRUICORE_API XR_IMPORT
#   endif
#endif

typedef FactoryPtr<IUIShader> ui_shader;

constexpr float UI_BASE_WIDTH = 1024.0f;
constexpr float UI_BASE_HEIGHT = 768.0f;

enum EUIItemAlign
{
    alNone = 0x0000,
    alLeft = 0x0001,
    alRight = 0x0002,
    alTop = 0x0004,
    alBottom = 0x0008,
    alCenter = 0x0010
};

struct S2DVert
{
    Fvector2 pt;
    Fvector2 uv;
    S2DVert() {}
    S2DVert(float pX, float pY, float tU, float tV)
    {
        pt.set(pX, pY);
        uv.set(tU, tV);
    }
    void set(float pt_x, float pt_y, float uv_x, float uv_y)
    {
        pt.set(pt_x, pt_y);
        uv.set(uv_x, uv_y);
    }
    void set(const Fvector2& _pt, const Fvector2& _uv)
    {
        pt.set(_pt);
        uv.set(_uv);
    }
    void rotate_pt(const Fvector2& pivot, const float cosA, const float sinA, const float kx);
};

constexpr u32 UI_FRUSTUM_MAXPLANES = 12;
constexpr u32 UI_FRUSTUM_SAFE = (UI_FRUSTUM_MAXPLANES * 4);
typedef svector<S2DVert, UI_FRUSTUM_SAFE> sPoly2D;

class XRUICORE_API C2DFrustum
{
    svector<Fplane2, FRUSTUM_MAXPLANES> planes;
    Frect m_rect;

public:
    void CreateFromRect(const Frect& rect);
    sPoly2D* ClipPoly(sPoly2D& S, sPoly2D& D) const;
};

extern ENGINE_API bool g_bRendering;
