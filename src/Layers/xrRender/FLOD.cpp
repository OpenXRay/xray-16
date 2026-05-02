#include "stdafx.h"
#include "xrCore/FMesh.hpp"
#include "FLOD.h"

namespace xray::render::fg
{
namespace flod
{
struct _hw
{
    Fvector p0;
    Fvector p1;
    Fvector n0;
    Fvector n1;
    u32 sun_af;
    Fvector2 t0;
    Fvector2 t1;
    u32 rgbh0;
    u32 rgbh1;
};

static VertexElement dwDecl[] =
{
    {0, 0,  VF_FLOAT3, 0, VS_POSITION, 0},
    {0, 12, VF_FLOAT3, 0, VS_POSITION, 1},
    {0, 24, VF_FLOAT3, 0, VS_NORMAL,   0},
    {0, 36, VF_FLOAT3, 0, VS_NORMAL,   1},
    {0, 48, VF_COLOR,  0, VS_COLOR,    0},
    {0, 52, VF_FLOAT2, 0, VS_TEXCOORD, 0},
    {0, 60, VF_FLOAT2, 0, VS_TEXCOORD, 1},
    {0, 68, VF_COLOR,  0, VS_COLOR,    1},
    {0, 72, VF_COLOR,  0, VS_COLOR,    2},
    XR_VERTEX_ELEMENT_END
};
}

void FLOD::Load(LPCSTR name, IReader* data, u32 dwFlags)
{
    inherited::Load(name, data, dwFlags);

    // LOD-def
    R_ASSERT(data->find_chunk(OGF_LODDEF2));
    for (int f = 0; f < 8; f++)
    {
        data->r(facets[f].v, sizeof(facets[f].v));
        _vertex* v = facets[f].v;

        Fvector N, T;
        N.set(0, 0, 0);
        T.mknormal(v[0].v, v[1].v, v[2].v);
        N.add(T);
        T.mknormal(v[1].v, v[2].v, v[3].v);
        N.add(T);
        T.mknormal(v[2].v, v[3].v, v[0].v);
        N.add(T);
        T.mknormal(v[3].v, v[0].v, v[1].v);
        N.add(T);
        N.div(4.f);
        facets[f].N.normalize(N);
        facets[f].N.invert();
    }

    // lod correction
    Fvector3 S;
    vis.box.getradius(S);
    float r = vis.sphere.R;
    std::sort(&S.x, &S.x + 3);
    float a = S.y;
    float Sf = 4.f * (0.5f * (r * r * asin(a / r) + a * _sqrt(r * r - a * a)));
    float Ss = M_PI * r * r;
    lod_factor = Sf / Ss;
}
void FLOD::Copy(dxRender_Visual* pFrom)
{
    inherited::Copy(pFrom);

    FLOD* F = (FLOD*)pFrom;
    geom = F->geom;
    lod_factor = F->lod_factor;
    CopyMemory(facets, F->facets, sizeof(facets));
}
} // namespace xray::render::fg
