#include "stdafx.h"

#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/Environment.h"
#include "xrCore/FMesh.hpp"
#include "FTreeVisual.h"
#include "Layers/xrRender/r__buffer_pool.h"
#include "Common/OGF_GContainer_Vertices.hpp"

namespace xray::render::fg
{
shared_str m_xform;
shared_str m_xform_v;
shared_str c_consts;
shared_str c_wave;
shared_str c_wind;
shared_str c_c_bias;
shared_str c_c_scale;
shared_str c_c_sun;

FTreeVisual::FTreeVisual(void) {}
FTreeVisual::~FTreeVisual(void) {}
void FTreeVisual::Release() { dxRender_Visual::Release(); }
void FTreeVisual::Load(const char* N, IReader* data, u32 dwFlags)
{
    dxRender_Visual::Load(N, data, dwFlags);

    const VertexElement* vFormat = nullptr;

    // read vertices
    R_ASSERT(data->find_chunk(OGF_GCONTAINER));
    {
        // verts
        u32 vbID = data->r_u32();
        vBase = data->r_u32();
        vCount = data->r_u32();
        vFormat = BufferPool.getVB_Format(vbID);

        VERIFY(nullptr == p_rm_Vertices);
        p_rm_Vertices = BufferPool.getVB(vbID);
        p_rm_Vertices->AddRef();

        // GPU-driven: Store VB pool ID for mega-buffer lookup
        vbPoolID = vbID;

        // indices
        dwPrimitives = 0;
        u32 ibID = data->r_u32();
        iBase = data->r_u32();
        iCount = data->r_u32();
        dwPrimitives = iCount / 3;

        VERIFY(nullptr == p_rm_Indices);
        p_rm_Indices = BufferPool.getIB(ibID);
        p_rm_Indices->AddRef();

        // GPU-driven: Store IB pool ID for mega-buffer lookup
        ibPoolID = ibID;
        useAlternativeGeom = false;
    }

    // load tree-def
    R_ASSERT(data->find_chunk(OGF_TREEDEF2));
    {
        data->r(&xform, sizeof(xform));
        data->r(&c_scale, sizeof(c_scale));
        c_scale.rgb.mul(.5f);
        c_scale.hemi *= .5f;
        c_scale.sun *= .5f;
        data->r(&c_bias, sizeof(c_bias));
        c_bias.rgb.mul(.5f);
        c_bias.hemi *= .5f;
        c_bias.sun *= .5f;
        // Msg				("hemi[%f / %f], sun[%f / %f]",c_scale.hemi,c_bias.hemi,c_scale.sun,c_bias.sun);
    }

    /*if (RImplementation.o.ffp && dcl_equal(vFormat, mu_model_decl_unpacked))
    {
        const size_t vertices_size = vCount * sizeof(mu_model_vert_unpacked);

        const auto new_buffer = xr_new<VertexStagingBuffer>();
        new_buffer->Create(vertices_size);

        auto vert_new = static_cast<mu_model_vert_unpacked*>(new_buffer->Map());
        const auto vert_orig = static_cast<mu_model_vert_unpacked*>(p_rm_Vertices->Map(vBase, vertices_size, true)); // read-back
        CopyMemory(vert_new, vert_orig, vertices_size);

        for (size_t i = 0; i < vCount; ++i)
        {
            //vert_new->P.mul(xform.j);
            ++vert_new;
        }

        new_buffer->Unmap(true);
        p_rm_Vertices->Unmap(false);
        _RELEASE(p_rm_Vertices);
        p_rm_Vertices = new_buffer;
        vBase = 0;
    }*/

    // Geom
    rm_geom.create(vFormat, *p_rm_Vertices, *p_rm_Indices);

    // Get constants
    m_xform = "m_xform";
    m_xform_v = "m_xform_v";
    c_consts = "consts";
    c_wave = "wave";
    c_wind = "wind";
    c_c_bias = "c_bias";
    c_c_scale = "c_scale";
    c_c_sun = "c_sun";
}

struct FTreeVisual_setup
{
    u32 dwFrame;
    float scale;
    Fvector4 wave;
    Fvector4 wind;

    FTreeVisual_setup(): dwFrame(0), scale(0) {}

    void calculate()
    {
        dwFrame = Device.dwFrame;
        CEnvDescriptor& desc = g_pGamePersistent->Environment().CurrentEnv;

        // Calc wind-vector3, scale
        float tm_rot = PI_MUL_2 * Device.fTimeGlobal / desc.m_fTreeRotation;

        wind.set(_sin(tm_rot), 0, _cos(tm_rot), 0);
        wind.normalize();
        wind.mul(desc.m_fTreeAmplitude); // dir1*amplitude

        scale = 1.f / float(FTreeVisual_quant);

        // setup constants
        wave.set(desc.m_fTreeWave.x, desc.m_fTreeWave.y, desc.m_fTreeWave.z, Device.fTimeGlobal * desc.m_fTreeSpeed); // wave
        wave.div(PI_MUL_2);
    }
};

void FTreeVisual::Render(CBackend&, float, bool) {}

#define PCOPY(a) a = pFrom->a
void FTreeVisual::Copy(dxRender_Visual* pSrc)
{
    dxRender_Visual::Copy(pSrc);

    FTreeVisual* pFrom = dynamic_cast<FTreeVisual*>(pSrc);

    PCOPY(rm_geom);
    PCOPY(p_rm_Vertices);
    if (p_rm_Vertices)
        p_rm_Vertices->AddRef();
    PCOPY(vBase);
    PCOPY(vCount);
    PCOPY(vStride);
    PCOPY(p_rm_Indices);
    if (p_rm_Indices)
        p_rm_Indices->AddRef();
    PCOPY(iBase);
    PCOPY(iCount);
    PCOPY(dwPrimitives);

    PCOPY(xform);
    PCOPY(c_scale);
    PCOPY(c_bias);
}

//-----------------------------------------------------------------------------------
// Stripified Tree
//-----------------------------------------------------------------------------------
FTreeVisual_ST::FTreeVisual_ST(void) {}
FTreeVisual_ST::~FTreeVisual_ST(void) {}
void FTreeVisual_ST::Release() { inherited::Release(); }
void FTreeVisual_ST::Load(const char* N, IReader* data, u32 dwFlags) { inherited::Load(N, data, dwFlags); }
void FTreeVisual_ST::Render(CBackend&, float, bool) {}
void FTreeVisual_ST::Copy(dxRender_Visual* pSrc) { inherited::Copy(pSrc); }
//-----------------------------------------------------------------------------------
// Progressive Tree
//-----------------------------------------------------------------------------------
FTreeVisual_PM::FTreeVisual_PM(void)
{
    pSWI = nullptr;
    last_lod = 0;
}
FTreeVisual_PM::~FTreeVisual_PM(void) {}
void FTreeVisual_PM::Release() { inherited::Release(); }
void FTreeVisual_PM::Load(const char* N, IReader* data, u32 dwFlags)
{
    inherited::Load(N, data, dwFlags);
    R_ASSERT(data->find_chunk(OGF_SWICONTAINER));
    {
        u32 ID = data->r_u32();
        pSWI = BufferPool.getSWI(ID);
    }
}
void FTreeVisual_PM::Render(CBackend&, float, bool) {}
void FTreeVisual_PM::Copy(dxRender_Visual* pSrc)
{
    inherited::Copy(pSrc);
    FTreeVisual_PM* pFrom = dynamic_cast<FTreeVisual_PM*>(pSrc);
    PCOPY(pSWI);
}
} // namespace xray::render::fg
