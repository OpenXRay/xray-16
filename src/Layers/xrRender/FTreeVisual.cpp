#include "stdafx.h"

#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/Environment.h"
#include "xrCore/FMesh.hpp"
#include "FTreeVisual.h"
#include "Common/OGF_GContainer_Vertices.hpp"

shared_str m_xform;
shared_str m_xform_v;
shared_str c_consts;
shared_str c_wave;
shared_str c_wind;
shared_str c_c_bias;
shared_str c_c_scale;
shared_str c_c_sun;
shared_str c_c_BendersPos;
shared_str c_c_BendersSetup;

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
        u32 ID = data->r_u32();
        vBase = data->r_u32();
        vCount = data->r_u32();
        vFormat = RImplementation.getVB_Format(ID);

        VERIFY(nullptr == p_rm_Vertices);
        p_rm_Vertices = RImplementation.getVB(ID);
        p_rm_Vertices->AddRef();

        // indices
        dwPrimitives = 0;
        ID = data->r_u32();
        iBase = data->r_u32();
        iCount = data->r_u32();
        dwPrimitives = iCount / 3;

        VERIFY(nullptr == p_rm_Indices);
        p_rm_Indices = RImplementation.getIB(ID);
        p_rm_Indices->AddRef();
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
    c_c_BendersPos = "benders_pos";
    c_c_BendersSetup = "benders_setup";
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

        const float tm_rot = PI_MUL_2 * Device.fTimeGlobal / ps_r__Tree_w_rot;

        // Calc wind-vector3, scale

        wind.set(_sin(tm_rot), 0, _cos(tm_rot), 0);
        wind.normalize();

#if RENDER!=R_R1
        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
        const float fValue = env.m_fTreeAmplitudeIntensity;
        wind.mul(fValue); // dir1*amplitude
#else
        wind.mul(ps_r__Tree_w_amp); // dir1*amplitude
#endif

        scale = 1.f / float(FTreeVisual_quant);

        // setup constants
        wave.set(
            ps_r__Tree_Wave.x, ps_r__Tree_Wave.y, ps_r__Tree_Wave.z, Device.fTimeGlobal * ps_r__Tree_w_speed); // wave
        wave.div(PI_MUL_2);
    }
};

void FTreeVisual::Render(CBackend& cmd_list, float /*LOD*/, bool use_fast_geo)
{
    static FTreeVisual_setup tvs;
    if (tvs.dwFrame != Device.dwFrame)
        tvs.calculate();
// setup constants
#if RENDER != R_R1
    Fmatrix xform_v;
    xform_v.mul_43(cmd_list.get_xform_view(), xform);
    cmd_list.tree.set_m_xform_v(xform_v); // matrix
#endif
    float s = ps_r__Tree_SBC;
    cmd_list.tree.set_m_xform(xform); // matrix
    cmd_list.tree.set_consts(tvs.scale, tvs.scale, 0, 0); // consts/scale
    cmd_list.tree.set_wave(tvs.wave); // wave
    cmd_list.tree.set_wind(tvs.wind); // wind
#if RENDER != R_R1
    s *= 1.3333f;
    cmd_list.tree.set_c_scale(s * c_scale.rgb.x, s * c_scale.rgb.y, s * c_scale.rgb.z, s * c_scale.hemi); // scale
    cmd_list.tree.set_c_bias(s * c_bias.rgb.x, s * c_bias.rgb.y, s * c_bias.rgb.z, s * c_bias.hemi); // bias
#else
    const auto& desc = g_pGamePersistent->Environment().CurrentEnv;
    cmd_list.tree.set_c_scale(s * c_scale.rgb.x, s * c_scale.rgb.y, s * c_scale.rgb.z, s * c_scale.hemi); // scale
    cmd_list.tree.set_c_bias(s * c_bias.rgb.x + desc.ambient.x, s * c_bias.rgb.y + desc.ambient.y,
        s * c_bias.rgb.z + desc.ambient.z, s * c_bias.hemi); // bias
#endif
    cmd_list.tree.set_c_sun(s * c_scale.sun, s * c_bias.sun, 0, 0); // sun

#if RENDER == R_R4
    if (ps_ssfx_grass_interactive.y > 0)
    {
        // Inter grass Settings
        cmd_list.set_c(c_c_BendersSetup, ps_ssfx_int_grass_params_1);

        // Grass benders data ( Player + Characters )
        IGame_Persistent::grass_data& GData = g_pGamePersistent->grass_shader_data;
        Fvector4 player_pos = { 0, 0, 0, 0 };
        int BendersQty = _min(16, (int)(ps_ssfx_grass_interactive.y + 1));

        // Add Player?
        if (ps_ssfx_grass_interactive.x > 0)
        {
            player_pos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, -1);
        }

        Fvector4* c_grass{};
        {
            void* GrassData;
            cmd_list.get_ConstantDirect(c_c_BendersPos, BendersQty * sizeof(Fvector4) * 2, &GrassData, 0, 0);

            c_grass = (Fvector4*)GrassData;
        }

        if (c_grass)
        {
            c_grass[0].set(player_pos);
            c_grass[16].set(0.0f, -99.0f, 0.0f, 1.0f);

            for (int Bend = 1; Bend < BendersQty; Bend++)
            {
                c_grass[Bend].set(GData.pos[Bend].x, GData.pos[Bend].y, GData.pos[Bend].z, GData.radius_curr[Bend]);
                c_grass[Bend + 16].set(GData.dir[Bend].x, GData.dir[Bend].y, GData.dir[Bend].z, GData.str[Bend]);
            }
        }
    }
#endif
}

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
void FTreeVisual_ST::Render(CBackend& cmd_list, float LOD, bool use_fast_geo)
{
    inherited::Render(cmd_list, LOD, use_fast_geo);
    cmd_list.set_Geometry(rm_geom);
    cmd_list.Render(D3DPT_TRIANGLELIST, vBase, 0, vCount, iBase, dwPrimitives);
    cmd_list.stat.r.s_flora.add(vCount);
}
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
        pSWI = RImplementation.getSWI(ID);
    }
}
void FTreeVisual_PM::Render(CBackend& cmd_list, float LOD, bool use_fast_geo)
{
    inherited::Render(cmd_list, LOD, use_fast_geo);
    int lod_id = last_lod;
    if (LOD >= 0.f)
    {
        lod_id = iFloor((1.f - LOD) * float(pSWI->count - 1) + 0.5f);
        last_lod = lod_id;
    }
    VERIFY(lod_id >= 0 && lod_id < int(pSWI->count));
    FSlideWindow& SW = pSWI->sw[lod_id];
    cmd_list.set_Geometry(rm_geom);
    cmd_list.Render(D3DPT_TRIANGLELIST, vBase, 0, SW.num_verts, iBase + SW.offset, SW.num_tris);
    cmd_list.stat.r.s_flora.add(SW.num_verts);
}
void FTreeVisual_PM::Copy(dxRender_Visual* pSrc)
{
    inherited::Copy(pSrc);
    FTreeVisual_PM* pFrom = dynamic_cast<FTreeVisual_PM*>(pSrc);
    PCOPY(pSWI);
}
