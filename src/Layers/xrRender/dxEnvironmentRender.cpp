#include "stdafx.h"

#include "dxEnvironmentRender.h"

#include "Blender.h"
#include "ResourceManager.h"

#include "xrEngine/Environment.h"
#include "xrEngine/xr_efflensflare.h"

//////////////////////////////////////////////////////////////////////////
// half box def
static Fvector3 hbox_verts[24] =
{
    {-1.f,   -1.f, -1.f}, { -1.f, -1.01f, -1.f}, // down
    { 1.f,   -1.f, -1.f}, {  1.f, -1.01f, -1.f}, // down
    {-1.f,   -1.f,  1.f}, { -1.f, -1.01f,  1.f}, // down
    { 1.f,   -1.f,  1.f}, {  1.f, -1.01f,  1.f}, // down
    {-1.f,    1.f, -1.f}, { -1.f,  1.f,   -1.f},
    { 1.f,    1.f, -1.f}, {  1.f,  1.f,   -1.f},
    {-1.f,    1.f,  1.f}, { -1.f,  1.f,    1.f},
    { 1.f,    1.f,  1.f}, {  1.f,  1.f,    1.f},
    {-1.f, -0.01f, -1.f}, { -1.f, -1.f,   -1.f}, // half
    { 1.f, -0.01f, -1.f}, {  1.f, -1.f,   -1.f}, // half
    { 1.f, -0.01f,  1.f}, {  1.f, -1.f,    1.f}, // half
    {-1.f, -0.01f,  1.f}, { -1.f, -1.f,    1.f}  // half
};

static u16 hbox_faces[20 * 3] =
{
    0,   2,  3,
    3,   1,  0,
    4,   5,  7,
    7,   6,  4,
    0,   1,  9,
    9,   8,  0,
    8,   9,  5,
    5,   4,  8,
    1,   3, 10,
    10,  9,  1,
    9,  10,  7,
    7,   5,  9,
    3,   2, 11,
    11, 10,  3,
    10, 11,  6,
    6,   7, 10,
    2,   0,  8,
    8,  11,  2,
    11,  8,  4,
    4,   6, 11
};

#pragma pack(push, 1)
struct v_skybox
{
    Fvector3 p;
    u32 color;
    Fvector3 uv[2];

    void set(Fvector3& _p, u32 _c, Fvector3& _tc)
    {
        p = _p;
        color = _c;
        uv[0] = _tc;
        uv[1] = _tc;
    }
};
constexpr u32 v_skybox_fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE3(1);
struct v_clouds
{
    Fvector3 p;
    u32 color;
    u32 intensity;
    void set(Fvector3& _p, u32 _c, u32 _i)
    {
        p = _p;
        color = _c;
        intensity = _i;
    }
};
const u32 v_clouds_fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
#pragma pack(pop)

class CBlender_skybox : public IBlender
{
public:
    pcstr getComment() override { return "INTERNAL: combiner"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override
    {
        C.r_Pass("sky2", "sky2", FALSE, TRUE, FALSE);
#if defined(USE_DX11)
        // C.r_Sampler_clf		("s_sky0",		"$null"			);
        // C.r_Sampler_clf		("s_sky1",		"$null"			);
        C.r_dx11Texture("s_sky0", "$null");
        C.r_dx11Texture("s_sky1", "$null");
        C.r_dx11Sampler("smp_rtlinear");
        // C.r_Sampler_rtf		("s_tonemap",	"$user$tonemap"	);	//. hack
        C.r_dx11Texture("s_tonemap", "$user$tonemap"); //. hack
        C.PassSET_ZB(FALSE, FALSE);
#elif defined(USE_OGL)
        C.r_Sampler_clf("s_sky0", "$null");
        C.r_Sampler_clf("s_sky1", "$null");
        C.r_Sampler_rtf("s_tonemap", "$user$tonemap"); //. hack
        C.PassSET_ZB(FALSE, FALSE);
#else
#   error No graphics API selected or enabled!
#endif
        C.r_End();
    }
};

void dxEnvDescriptorRender::Copy(IEnvDescriptorRender& _in) { *this = *(dxEnvDescriptorRender*)&_in; }
void dxEnvironmentRender::Copy(IEnvironmentRender& _in) { *this = *(dxEnvironmentRender*)&_in; }

particles_systems::library_interface const& dxEnvironmentRender::particles_systems_library()
{
    return (RImplementation.PSLibrary);
}

void dxEnvDescriptorRender::OnDeviceCreate(CEnvDescriptor& owner)
{
    if (owner.sky_texture_name.size())
        sky_texture.create(owner.sky_texture_name.c_str());

    if (owner.sky_texture_env_name.size())
        sky_texture_env.create(owner.sky_texture_env_name.c_str());

    if (owner.clouds_texture_name.size())
        clouds_texture.create(owner.clouds_texture_name.c_str());
}

void dxEnvDescriptorRender::OnDeviceDestroy()
{
    sky_texture.destroy();
    sky_texture_env.destroy();
    clouds_texture.destroy();
}

dxEnvironmentRender::dxEnvironmentRender()
{
    tsky0.create(r2_T_sky0);
    tsky1.create(r2_T_sky1);
    t_envmap_0.create(r2_T_envs0);
    t_envmap_1.create(r2_T_envs1);
    tonemap.create(r2_RT_luminance_cur);
}

void dxEnvironmentRender::Clear()
{
    std::pair<u32, ref_texture> zero = std::make_pair(u32(0), ref_texture(nullptr));
    sky_r_textures.clear();
    sky_r_textures.push_back(zero);
    sky_r_textures.push_back(zero);
    sky_r_textures.push_back(zero);

    clouds_r_textures.clear();
    clouds_r_textures.push_back(zero);
    clouds_r_textures.push_back(zero);
    clouds_r_textures.push_back(zero);
}

void dxEnvironmentRender::lerp(CEnvDescriptorMixer& currentEnv, IEnvDescriptorRender* inA, IEnvDescriptorRender* inB)
{
    dxEnvDescriptorRender* pA = static_cast<dxEnvDescriptorRender*>(inA);
    dxEnvDescriptorRender* pB = static_cast<dxEnvDescriptorRender*>(inB);

    sky_r_textures.clear();
    sky_r_textures.emplace_back(tsky0_tstage, pA->sky_texture);
    sky_r_textures.emplace_back(tsky1_tstage, pB->sky_texture);
    if (tonemap_tstage_2sky != u32(-1))
        sky_r_textures.emplace_back(tonemap_tstage_2sky, tonemap);

    clouds_r_textures.clear();
    clouds_r_textures.emplace_back(tclouds0_tstage, pA->clouds_texture);
    clouds_r_textures.emplace_back(tclouds1_tstage, pB->clouds_texture);
    if (tonemap_tstage_clouds != u32(-1))
        clouds_r_textures.emplace_back(tonemap_tstage_clouds, tonemap);

    //. Setup skybox textures, somewhat ugly
    auto e0 = sky_r_textures[0].second->surface_get();
    auto e1 = sky_r_textures[1].second->surface_get();
#ifdef USE_OGL
    tsky0->surface_set(GL_TEXTURE_CUBE_MAP, e0);
    tsky1->surface_set(GL_TEXTURE_CUBE_MAP, e1);
#else // USE_OGL
    tsky0->surface_set(e0);
    _RELEASE(e0);
    tsky1->surface_set(e1);
    _RELEASE(e1);
#endif // USE_OGL

    const bool menu_pp = g_pGamePersistent->OnRenderPPUI_query();
    e0 = menu_pp ? 0 : pA->sky_texture_env->surface_get();
    e1 = menu_pp ? 0 : pB->sky_texture_env->surface_get();
#   ifdef USE_OGL
    t_envmap_0->surface_set(GL_TEXTURE_CUBE_MAP, e0);
    t_envmap_1->surface_set(GL_TEXTURE_CUBE_MAP, e1);
#   else // USE_OGL
    t_envmap_0->surface_set(e0);
    _RELEASE(e0);
    t_envmap_1->surface_set(e1);
    _RELEASE(e1);
#   endif // USE_OGL

    // ******************** Environment params (setting)
#if defined(USE_DX9)
#   if RENDER == R_R1
    Fvector3 fog_color = currentEnv.fog_color;
    fog_color.mul(ps_r1_fog_luminance);
#   else
    Fvector3& fog_color = currentEnv.fog_color;
#   endif
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGCOLOR, color_rgba_f(fog_color.x, fog_color.y, fog_color.z, 0)));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGSTART, *(u32*)(&currentEnv.fog_near)));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGEND, *(u32*)(&currentEnv.fog_far)));
#else
    //	TODO: DX11: Implement environment parameters setting for DX11 (if necessary)
#endif
}

void dxEnvironmentRender::RenderSky(CEnvironment& env)
{
    RImplementation.rmFar(RCache);

    // draw sky box
    Fmatrix mSky;
    mSky.rotateY(env.CurrentEnv.sky_rotation);
    mSky.translate_over(Device.vCameraPosition);

    u32 i_offset, v_offset;
    u32 C = color_rgba(iFloor(env.CurrentEnv.sky_color.x * 255.f), iFloor(env.CurrentEnv.sky_color.y * 255.f),
        iFloor(env.CurrentEnv.sky_color.z * 255.f), iFloor(env.CurrentEnv.weight * 255.f));

    // Fill index buffer
    u16* pib = RImplementation.Index.Lock(20 * 3, i_offset);
    CopyMemory(pib, hbox_faces, 20 * 3 * 2);
    RImplementation.Index.Unlock(20 * 3);

    // Fill vertex buffer
    v_skybox* pv = (v_skybox*)RImplementation.Vertex.Lock(12, sh_2geom.stride(), v_offset);
    for (u32 v = 0; v < 12; v++)
        pv[v].set(hbox_verts[v * 2], C, hbox_verts[v * 2 + 1]);
    RImplementation.Vertex.Unlock(12, sh_2geom.stride());

    // Render
    RCache.set_xform_world(mSky);
    RCache.set_Geometry(sh_2geom);
    RCache.set_Shader(sh_2sky);
#if defined(USE_DX11)
    RCache.set_Textures(&sky_r_textures);
#elif defined(USE_OGL)
    if (HW.Caps.geometry.bVTF)
        RCache.set_Textures(&sky_r_textures);
#else
#   error No graphics API selected or enabled!
#endif
    RCache.Render(D3DPT_TRIANGLELIST, v_offset, 0, 12, i_offset, 20);

#ifdef USE_OGL
    // Sun must be rendered to generic0 only as it is done in DX
    if (!RImplementation.o.msaa)
        RImplementation.Target->u_setrt(RCache, RImplementation.Target->rt_Generic_0, nullptr, nullptr, RImplementation.Target->rt_Base_Depth);
    else
        RImplementation.Target->u_setrt(RCache, RImplementation.Target->rt_Generic_0_r, nullptr, nullptr, RImplementation.Target->rt_MSAADepth);
#endif // USE_OGL

    // Sun
    RImplementation.rmNormal(RCache);
#if RENDER != R_R1
    //
    // This hack is done to make sure that the state is set for sure:
    // The state may be not set by RCache if the state is changed using API SetRenderState() function before
    // and the RCache flag will remain unchanged to it's old value.
    //
    RCache.set_Z(FALSE);
    RCache.set_Z(TRUE);
    env.eff_LensFlare->Render(TRUE, FALSE, FALSE);
    RCache.set_Z(FALSE);
#else // RENDER != R_R1
    env.eff_LensFlare->Render(TRUE, FALSE, FALSE);
#endif // RENDER != R_R1

#ifdef USE_OGL
    // set low/hi RTs for clouds
    if (!RImplementation.o.msaa)
        RImplementation.Target->u_setrt(RCache, RImplementation.Target->rt_Generic_0, RImplementation.Target->rt_Generic_1, nullptr, RImplementation.Target->rt_Base_Depth);
    else
        RImplementation.Target->u_setrt(RCache, RImplementation.Target->rt_Generic_0_r, RImplementation.Target->rt_Generic_1_r, nullptr, RImplementation.Target->rt_MSAADepth);
#endif // USE_OGL
}

void dxEnvironmentRender::RenderClouds(CEnvironment& env)
{
    if (!clouds_sh)
        return;

    RImplementation.rmFar(RCache);

    Fmatrix mXFORM, mScale;
    mScale.scale(10, 0.4f, 10);
    mXFORM.rotateY(env.CurrentEnv.clouds_rotation);
    mXFORM.mulB_43(mScale);
    mXFORM.translate_over(Device.vCameraPosition);

    Fvector wd0, wd1;
    Fvector4 wind_dir;
    wd0.setHP(PI_DIV_4, 0);
    wd1.setHP(PI_DIV_4 + PI_DIV_8, 0);
    wind_dir.set(wd0.x, wd0.z, wd1.x, wd1.z).mul(0.5f).add(0.5f).mul(255.f);
    u32 i_offset, v_offset;
    u32 C0 = color_rgba(iFloor(wind_dir.x), iFloor(wind_dir.y), iFloor(wind_dir.w), iFloor(wind_dir.z));
    u32 C1 = color_rgba(iFloor(env.CurrentEnv.clouds_color.x * 255.f), iFloor(env.CurrentEnv.clouds_color.y * 255.f),
        iFloor(env.CurrentEnv.clouds_color.z * 255.f), iFloor(env.CurrentEnv.clouds_color.w * 255.f));

    // Fill index buffer
    u16* pib = RImplementation.Index.Lock(env.CloudsIndices.size(), i_offset);
    CopyMemory(pib, &env.CloudsIndices.front(), env.CloudsIndices.size() * sizeof(u16));
    RImplementation.Index.Unlock(env.CloudsIndices.size());

    // Fill vertex buffer
    v_clouds* pv = (v_clouds*)RImplementation.Vertex.Lock(env.CloudsVerts.size(), clouds_geom.stride(), v_offset);
    for (auto it = env.CloudsVerts.begin(); it != env.CloudsVerts.end(); ++it, pv++)
        pv->set(*it, C0, C1);
    RImplementation.Vertex.Unlock(env.CloudsVerts.size(), clouds_geom.stride());

    // Render
    RCache.set_xform_world(mXFORM);
    RCache.set_Geometry(clouds_geom);
    RCache.set_Shader(clouds_sh);
    RCache.set_Textures(&clouds_r_textures);
    RCache.Render(D3DPT_TRIANGLELIST, v_offset, 0, env.CloudsVerts.size(), i_offset, env.CloudsIndices.size() / 3);

    RImplementation.rmNormal(RCache);
}

void dxEnvironmentRender::OnDeviceCreate()
{
    if (GEnv.isDedicatedServer)
        return;

    if (RImplementation.o.ffp)
    {
        // XXX: We need better blender with multitexturing
        // to properly blend two textures.
        // Currently, it just suddenly changes.
        sh_2sky.create("sky\\skydome", "skybox_2t");
    }
    else
    {
        CBlender_skybox b_skybox;
        sh_2sky.create(&b_skybox, "skybox_2t");
    }
    sh_2geom.create(v_skybox_fvf, RImplementation.Vertex.Buffer(), RImplementation.Index.Buffer());
    clouds_sh.create("clouds", "null");
    clouds_geom.create(v_clouds_fvf, RImplementation.Vertex.Buffer(), RImplementation.Index.Buffer());

    const auto& sky2_constants = sh_2sky->E[0]->passes[0]->constants;
    const auto& clouds_constants = clouds_sh->E[0]->passes[0]->constants;

    // Just let texture stages be 0 if constants are missing
    if (sky2_constants)
    {
        if (const auto C = sky2_constants->get(c_ssky0)._get())
            tsky0_tstage = C->samp.index;

        if (const auto C = sky2_constants->get(c_ssky1)._get())
            tsky1_tstage = C->samp.index;
    }
    if (clouds_constants)
    {
        if (const auto C = clouds_constants->get(c_sclouds0)._get())
            tclouds0_tstage = C->samp.index;

        if (const auto C = clouds_constants->get(c_sclouds1)._get())
            tclouds1_tstage = C->samp.index;
    }

    const bool r2 = RImplementation.GenerationIsR2OrHigher();
    tonemap_tstage_2sky = sh_2sky->E[0]->passes[0]->T->find_texture_stage(r2_RT_luminance_cur, r2);
    tonemap_tstage_clouds = clouds_sh->E[0]->passes[0]->T->find_texture_stage(r2_RT_luminance_cur, r2);
}

void dxEnvironmentRender::OnDeviceDestroy()
{
    sky_r_textures.clear();
    clouds_r_textures.clear();

#if defined(USE_DX11)
    tsky0->surface_set(nullptr);
    tsky1->surface_set(nullptr);
    t_envmap_0->surface_set(nullptr);
    t_envmap_1->surface_set(nullptr);
    tonemap->surface_set(nullptr);
#elif defined(USE_OGL)
    tsky0->surface_set(GL_TEXTURE_CUBE_MAP, 0);
    tsky1->surface_set(GL_TEXTURE_CUBE_MAP, 0);
    t_envmap_0->surface_set(GL_TEXTURE_CUBE_MAP, 0);
    t_envmap_1->surface_set(GL_TEXTURE_CUBE_MAP, 0);
    tonemap->surface_set(GL_TEXTURE_CUBE_MAP, 0);
#else
#   error No graphics API slected or defined!
#endif

    sh_2sky.destroy();
    sh_2geom.destroy();
    clouds_sh.destroy();
    clouds_geom.destroy();

    tsky0_tstage = 0;
    tsky1_tstage = 0;
    tclouds0_tstage = 0;
    tclouds1_tstage = 0;
    tonemap_tstage_2sky = u32(-1);
    tonemap_tstage_clouds = u32(-1);
}
