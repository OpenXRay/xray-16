#include "stdafx.h"

#include "xrCore/Threading/TaskManager.hpp"

#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/CustomHUD.h"
#include "xrEngine/xr_object.h"

#include "Layers/xrRender/FBasicVisual.h"

#if defined(USE_DX11) && RENDER == R_R4
#include "Layers/xrRender/NVRHI/NVRHIDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/xrRender_console.h"
#endif

namespace xray::render::RENDER_NAMESPACE
{
void CRender::RenderMenu()
{
#if defined(USE_DX11)
    TracyD3D11Zone(HW.profiler_ctx, "render_menu");
#endif
    PIX_EVENT(render_menu);
    //	Globals
    RCache.set_CullMode(CULL_CCW);
    RCache.set_Stencil(FALSE);
    RCache.set_ColorWriteEnable();

    // Main Render
    {
        Target->u_setrt(RCache, Target->rt_Generic_0, nullptr, nullptr, Target->rt_Base_Depth); // LDR RT
        g_pGamePersistent->OnRenderPPUI_main(); // PP-UI
    }
    // Distort
    {
        Target->u_setrt(RCache, Target->rt_Generic_1, nullptr, nullptr, Target->rt_Base_Depth); // Now RT is a distortion mask
        RCache.ClearRT(Target->rt_Generic_1, color_rgba(127, 127, 0, 127));
        g_pGamePersistent->OnRenderPPUI_PP(); // PP-UI
    }

    // Actual Display
    Target->u_setrt(RCache, Device.dwWidth, Device.dwHeight, Target->get_base_rt(), 0, 0, Target->get_base_zb());
    RCache.set_Shader(Target->s_menu);
    RCache.set_Geometry(Target->g_menu);

    Fvector2 p0, p1;
    u32 Offset;
    u32 C = color_rgba(255, 255, 255, 255);
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    float d_Z = EPS_S;
    float d_W = 1.f;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);

    FVF::TL* pv = (FVF::TL*)RImplementation.Vertex.Lock(4, Target->g_menu->vb_stride, Offset);
#if defined(USE_DX11)
    pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
    pv++;
    pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
    pv++;
    pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
    pv++;
    pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
    pv++;
#elif defined(USE_OGL)
    pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
    pv++;
    pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
    pv++;
    pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
    pv++;
    pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
    pv++;
#else
#   error No graphics API selected or enabled!
#endif
    RImplementation.Vertex.Unlock(4, Target->g_menu->vb_stride);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

extern u32 g_r;
void CRender::Render()
{
    ZoneScoped;
#if defined(USE_DX11)
    TracyD3D11Zone(HW.profiler_ctx, "Render");
#endif
    PIX_EVENT(CRender_Render);

    g_r = 1;

    rmNormal(RCache);

    IMainMenu* pMainMenu = g_pGamePersistent ? g_pGamePersistent->m_pMainMenu : 0;
    bool bMenu = pMainMenu ? pMainMenu->CanSkipSceneRendering() : false;

    // XXX: do we need to handle case when there is level, but HUD isn't loaded yet?
    // if (!(g_pGameLevel && g_hud) || bMenu)
    if (!g_pGameLevel || bMenu)
    {
        Target->u_setrt(RCache, Device.dwWidth, Device.dwHeight, Target->get_base_rt(), 0, 0, Target->get_base_zb());
        return;
    }

#if defined(USE_DX11) && RENDER == R_R4
    // FrameGraph renderer - use new deferred rendering pipeline
    if (ps_r4_use_framegraph && m_framegraphRenderer)
    {
        m_framegraphRenderer->SetEnabled(true);
        m_framegraphRenderer->Render();
        return;
    }
    else if (m_framegraphRenderer)
    {
        // Ensure it's disabled when not using FrameGraph path
        m_framegraphRenderer->SetEnabled(false);
    }

    // NVRHI test mode - render blue screen instead of normal scene
    if (m_nvrhiTestMode && m_nvrhiDevice && m_nvrhiDevice->IsInitialized())
    {
        TestNVRHI_Render();
        return;
    }

    // RenderContext test mode - render colored triangle
    if (m_renderContextTestMode && m_nvrhiDevice && m_nvrhiDevice->IsInitialized())
    {
        TestRenderContext_Triangle();
        return;
    }
#endif

    if (m_bFirstFrameAfterReset)
    {
        m_bFirstFrameAfterReset = false;
        return;
    }

    //.	VERIFY					(g_pGameLevel && g_pGameLevel->pHUD);
    auto& dsgraph = get_imm_context();

    //******* Z-prefill calc - DEFERRER RENDERER
    if (ps_r2_ls_flags.test(R2FLAG_ZFILL))
    {
        PIX_EVENT(DEFER_Z_FILL);
        BasicStats.Culling.Begin();
        float z_distance = ps_r2_zfill;
        Fmatrix m_zfill, m_project;
        m_project.build_projection(deg2rad(Device.fFOV /* *Device.fASPECT*/), Device.fASPECT, VIEWPORT_NEAR,
            z_distance * g_pGamePersistent->Environment().CurrentEnv.far_plane);
        m_zfill.mul(m_project, Device.mView);

        if (last_sector_id != IRender_Sector::INVALID_SECTOR_ID)
        {
            dsgraph.o.phase = PHASE_SMAP;
            dsgraph.r_pmask(true, false); // enable priority "0"
            dsgraph.set_Recorder(nullptr);
            dsgraph.o.use_hom = true;
            dsgraph.o.is_main_pass = true;
            dsgraph.o.sector_id = last_sector_id;
            dsgraph.o.portal_traverse_flags = CPortalTraverser::VQ_HOM | CPortalTraverser::VQ_SSA | CPortalTraverser::VQ_FADE;
            dsgraph.o.spatial_traverse_flags = ISpatial_DB::O_ORDERED;
            dsgraph.o.spatial_types = STYPE_RENDERABLE | STYPE_LIGHTSOURCE;
            dsgraph.o.view_pos = Device.vCameraPosition;
            dsgraph.o.xform = m_zfill;
            dsgraph.o.view_frustum = ViewBase;
            dsgraph.o.query_box_side = VIEWPORT_NEAR + EPS_L;
            dsgraph.o.precise_portals = true;

            dsgraph.build_subspace();
        }
        BasicStats.Culling.End();
    }

    //*******
    // Sync point
    BasicStats.WaitS.Begin();
    {
        q_sync_point.Wait(ps_r2_wait_sleep, ps_r2_wait_timeout);
    }
    BasicStats.WaitS.End();
    q_sync_point.End();

    r_main.sync();

    if (ps_r2_ls_flags.test(R2FLAG_ZFILL))
    {
        // flush
        Target->phase_scene_prepare();
        dsgraph.cmd_list.set_ColorWriteEnable(FALSE);
        dsgraph.render_graph(0);
        dsgraph.cmd_list.set_ColorWriteEnable();
    }
    else
    {
        Target->phase_scene_prepare();
    }

    BOOL split_the_scene_to_minimize_wait = FALSE;
    if (ps_r2_ls_flags.test(R2FLAG_EXP_SPLIT_SCENE))
        split_the_scene_to_minimize_wait = TRUE;

    //******* Main render :: PART-0	-- first
#ifdef USE_OGL
    if (psDeviceFlags.test(rsWireframe))
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
    if (!split_the_scene_to_minimize_wait)
    {
        PIX_EVENT(DEFER_PART0_NO_SPLIT);
        // level, DO NOT SPLIT
        Target->phase_scene_begin();
        dsgraph.render_hud();
        dsgraph.render_graph(0);
        dsgraph.render_lods(true, true);
        if (Details)
            Details->Render(dsgraph.cmd_list);
        Target->phase_scene_end();
    }
    else
    {
        PIX_EVENT(DEFER_PART0_SPLIT);
        // level, SPLIT
        Target->phase_scene_begin();
        dsgraph.render_graph(0);
        Target->disable_aniso();
    }
#ifdef USE_OGL
    if (psDeviceFlags.test(rsWireframe))
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    //******* Occlusion testing of volume-limited light-sources
    Target->phase_occq();
    LP_normal.clear();
    LP_pending.clear();
    if (o.msaa)
    {
#if defined(USE_DX11)
        dsgraph.cmd_list.set_ZB(Target->rt_MSAADepth->pZRT[dsgraph.cmd_list.context_id]);
#elif defined(USE_OGL)
        dsgraph.cmd_list.set_ZB(Target->rt_MSAADepth->pZRT);
#endif
    }
    {
        PIX_EVENT(DEFER_TEST_LIGHT_VIS);
        light_Package& LP = Lights.package;

        // stats
        Stats.l_shadowed = LP.v_shadowed.size();
        Stats.l_unshadowed = LP.v_point.size() + LP.v_spot.size();
        Stats.l_total = Stats.l_shadowed + Stats.l_unshadowed;

        // perform tests
        size_t count = 0;
        count = _max(count, LP.v_point.size());
        count = _max(count, LP.v_spot.size());
        count = _max(count, LP.v_shadowed.size());
        for (size_t it = 0; it < count; it++)
        {
            if (it < LP.v_point.size())
            {
                light* L = LP.v_point[it];
                L->vis_prepare(dsgraph.cmd_list);
                if (L->vis.pending)
                    LP_pending.v_point.push_back(L);
                else
                    LP_normal.v_point.push_back(L);
            }
            if (it < LP.v_spot.size())
            {
                light* L = LP.v_spot[it];
                L->vis_prepare(dsgraph.cmd_list);
                if (L->vis.pending)
                    LP_pending.v_spot.push_back(L);
                else
                    LP_normal.v_spot.push_back(L);
            }
            if (it < LP.v_shadowed.size())
            {
                light* L = LP.v_shadowed[it];
                L->vis_prepare(dsgraph.cmd_list);
                if (L->vis.pending)
                    LP_pending.v_shadowed.push_back(L);
                else
                    LP_normal.v_shadowed.push_back(L);
            }
        }
    }
    LP_normal.sort();
    LP_pending.sort();

    //******* Main render :: PART-1 (second)
    if (split_the_scene_to_minimize_wait)
    {
        PIX_EVENT(DEFER_PART1_SPLIT);
        // skybox can be drawn here
        if (false)
        {
            Target->u_setrt(dsgraph.cmd_list, Target->rt_Generic_0_r, Target->rt_Generic_1_r, nullptr, Target->rt_MSAADepth);
            dsgraph.cmd_list.set_CullMode(CULL_NONE);
            dsgraph.cmd_list.set_Stencil(FALSE);

            // draw skybox
            dsgraph.cmd_list.set_ColorWriteEnable();
            dsgraph.cmd_list.set_Z(false);
            g_pGamePersistent->Environment().RenderSky();
            dsgraph.cmd_list.set_Z(true);
        }

        // level
        Target->phase_scene_begin();
        dsgraph.render_hud();
        dsgraph.render_lods(true, true);
        if (Details)
            Details->Render(dsgraph.cmd_list);
        Target->phase_scene_end();
    }

    if (g_pGameLevel->pHUD && g_pGameLevel->pHUD->RenderActiveItemUIQuery())
    {
        Target->phase_wallmarks();
        dsgraph.render_hud_ui();
    }

    // Wall marks
    if (Wallmarks)
    {
        PIX_EVENT(DEFER_WALLMARKS);
        Target->phase_wallmarks();
        g_r = 0;
        Wallmarks->Render(); // wallmarks has priority as normal geometry
    }

    // Update incremental shadowmap-visibility solver
    {
        PIX_EVENT(DEFER_FLUSH_OCCLUSION);
        u32 it = 0;
        for (it = 0; it < Lights_LastFrame.size(); it++)
        {
            if (0 == Lights_LastFrame[it])
                continue;
            try
            {
                for (int id = 0; id < 3; ++id)
                    Lights_LastFrame[it]->svis[id].flushoccq();
            }
            catch (...)
            {
                Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
            }
        }
        Lights_LastFrame.clear();
    }

    // full screen pass to mark msaa-edge pixels in highest stencil bit
    if (o.msaa)
    {
        PIX_EVENT(MARK_MSAA_EDGES);
        Target->mark_msaa_edges();
    }

    r_rain.sync();

    // Directional light - fucking sun
    {
        PIX_EVENT(DEFER_SUN);
        Stats.l_visible++;
        if (!RImplementation.o.oldshadowcascades)
            r_sun.sync();
        else
            r_sun_old.sync();
        Target->accum_direct_blend(dsgraph.cmd_list);
    }

    {
        PIX_EVENT(DEFER_SELF_ILLUM);
        Target->phase_accumulator(dsgraph.cmd_list);
        // Render emissive geometry, stencil - write 0x0 at pixel pos
        dsgraph.cmd_list.set_xform_project(Device.mProject);
        dsgraph.cmd_list.set_xform_view(Device.mView);
        // Stencil - write 0x1 at pixel pos -
        if (!o.msaa)
        {
            dsgraph.cmd_list.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff,
                D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        }
        else
        {
            dsgraph.cmd_list.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0x7f,
                D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
        }
        dsgraph.cmd_list.set_CullMode(CULL_CCW);
        dsgraph.cmd_list.set_ColorWriteEnable();
        dsgraph.render_emissive();
    }

    // Lighting, non dependant on OCCQ
    {
        PIX_EVENT(DEFER_LIGHT_NO_OCCQ);
        render_lights(LP_normal);
    }

    // Lighting, dependant on OCCQ
    {
        PIX_EVENT(DEFER_LIGHT_OCCQ);
        render_lights(LP_pending);
    }

    // Postprocess
    {
        PIX_EVENT(DEFER_LIGHT_COMBINE);
        Target->phase_combine();
    }

    VERIFY(dsgraph.mapDistort.empty());
}

void CRender::render_forward()
{
    ZoneScoped;
    auto& dsgraph = get_imm_context();

    //******* Main render - second order geometry (the one, that doesn't support deffering)
    //.todo: should be done inside "combine" with estimation of of luminance, tone-mapping, etc.
    {
        //	Igor: we don't want to render old lods on next frame.
        dsgraph.mapLOD.clear();
        dsgraph.render_graph(1); // normal level, secondary priority
        dsgraph.PortalTraverser.fade_render(); // faded-portals
        dsgraph.render_sorted(); // strict-sorted geoms
        g_pGamePersistent->Environment().RenderLast(); // rain/thunder-bolts
    }
}

// Перед началом рендера мира --#SM+#--
void CRender::BeforeWorldRender() {}

// После рендера мира и пост-эффектов --#SM+#--
void CRender::AfterWorldRender() {}

#if defined(USE_DX11) && RENDER == R_R4
void CRender::TestNVRHI_Render()
{
    VERIFY(m_nvrhiDevice && m_nvrhiDevice->IsInitialized());

    try
    {
        // Get NVRHI device and command list
        nvrhi::IDevice* device = m_nvrhiDevice->GetDevice();
        nvrhi::ICommandList* cmd = m_nvrhiDevice->GetCommandList();

        // Get current backbuffer from CHW
        ID3D11RenderTargetView* backbufferRTV = Target->get_base_rt();
        if (!backbufferRTV)
        {
            Msg("! [NVRHI Test] No backbuffer RTV");
            return;
        }

        ID3D11Resource* backbufferRes = nullptr;
        backbufferRTV->GetResource(&backbufferRes);

        if (!backbufferRes)
        {
            Msg("! [NVRHI Test] Failed to get backbuffer resource");
            return;
        }

        // Wrap backbuffer in NVRHI texture handle
        nvrhi::TextureDesc backbufferDesc;
        backbufferDesc.width = Device.dwWidth;
        backbufferDesc.height = Device.dwHeight;
        backbufferDesc.format = nvrhi::Format::RGBA8_UNORM;
        backbufferDesc.isRenderTarget = true;
        backbufferDesc.isUAV = false;
        backbufferDesc.debugName = "Backbuffer";
        backbufferDesc.dimension = nvrhi::TextureDimension::Texture2D;
        backbufferDesc.keepInitialState = true;
        backbufferDesc.initialState = nvrhi::ResourceStates::RenderTarget;

        nvrhi::TextureHandle backbuffer = device->createHandleForNativeTexture(
            nvrhi::ObjectTypes::D3D11_Resource,
            nvrhi::Object(backbufferRes),
            backbufferDesc
        );

        // Release the resource reference
        backbufferRes->Release();

        if (!backbuffer)
        {
            Msg("! [NVRHI Test] Failed to wrap backbuffer");
            return;
        }

        // Open command list
        cmd->open();

        // Clear to blue (R=0.1, G=0.2, B=0.4, A=1.0)
        nvrhi::Color clearColor(0.1f, 0.2f, 0.4f, 1.0f);
        cmd->clearTextureFloat(backbuffer, nvrhi::AllSubresources, clearColor);

        // Close command list
        cmd->close();

        // Execute
        m_nvrhiDevice->ExecuteCommandList(cmd);

        // Present the frame ourselves since we're bypassing normal rendering
        HW.Present();

    }
    catch (const std::exception& e)
    {
        Msg("! [NVRHI Test] Exception: %s", e.what());
    }
}

// Helper function to compile shader from file
static xr_vector<u8> CompileShaderFromFile(LPCSTR path, LPCSTR entryPoint, LPCSTR target)
{
    xr_vector<u8> result;

    // Read shader source file
    IReader* file = FS.r_open("$game_shaders$", path);
    if (!file)
    {
        Msg("! [ShaderCompiler] Failed to open shader: %s", path);
        return result;
    }

    // Prepare source
    xr_string source;
    source.assign((const char*)file->pointer(), file->length());
    FS.r_close(file);

    // Compile shader
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        source.c_str(),
        source.length(),
        path,
        nullptr,  // Defines
        D3D_COMPILE_STANDARD_FILE_INCLUDE,  // Include handler
        entryPoint,
        target,
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            Msg("! [ShaderCompiler] Compilation failed for %s:", path);
            Msg("  %s", (const char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        else
        {
            Msg("! [ShaderCompiler] Compilation failed for %s with HRESULT 0x%08X", path, hr);
        }

        if (shaderBlob)
            shaderBlob->Release();

        return result;
    }

    if (errorBlob)
        errorBlob->Release();

    // Copy to result vector
    result.resize(shaderBlob->GetBufferSize());
    memcpy(result.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
    shaderBlob->Release();

    Msg("~ [ShaderCompiler] Successfully compiled: %s", path);

    return result;
}

void CRender::TestRenderContext_Triangle()
{
    VERIFY(m_nvrhiDevice && m_nvrhiDevice->IsInitialized());

    using namespace xray::render::ng;

    nvrhi::IDevice* device = m_nvrhiDevice->GetDevice();
    nvrhi::ICommandList* cmd = m_nvrhiDevice->GetCommandList();

    // Create resources once
    if (!m_testVertexBuffer)
    {
        // Define triangle vertices (colored + textured)
        struct Vertex {
            float pos[3];
            float color[4];
            float uv[2];
        };

        Vertex vertices[] = {
            // Position (x, y, z)     // Color (r, g, b, a)        // UV (u, v)
            {{ 0.0f,  0.5f, 0.0f},   {1.0f, 1.0f, 1.0f, 1.0f},   {0.5f, 0.0f}}, // Top (white)
            {{ 0.5f, -0.5f, 0.0f},   {1.0f, 1.0f, 1.0f, 1.0f},   {1.0f, 1.0f}}, // Right (white)
            {{-0.5f, -0.5f, 0.0f},   {1.0f, 1.0f, 1.0f, 1.0f},   {0.0f, 1.0f}}  // Left (white)
        };

        // Create vertex buffer
        nvrhi::BufferDesc vbDesc;
        vbDesc.byteSize = sizeof(vertices);
        vbDesc.isVertexBuffer = true;
        vbDesc.debugName = "TestTriangle_VB";
        vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;

        m_testVertexBuffer = device->createBuffer(vbDesc);

        // Upload data
        cmd->open();
        cmd->beginMarker("UploadTriangleVB");
        cmd->writeBuffer(m_testVertexBuffer, vertices, sizeof(vertices));
        cmd->endMarker();
        cmd->close();
        m_nvrhiDevice->ExecuteCommandList(cmd);

        Msg("~ [TestRenderContext] Created vertex buffer");
    }

    if (!m_testIndexBuffer)
    {
        // Define triangle indices
        u16 indices[] = {0, 1, 2};

        // Create index buffer
        nvrhi::BufferDesc ibDesc;
        ibDesc.byteSize = sizeof(indices);
        ibDesc.isIndexBuffer = true;
        ibDesc.debugName = "TestTriangle_IB";
        ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

        m_testIndexBuffer = device->createBuffer(ibDesc);

        // Upload data
        cmd->open();
        cmd->beginMarker("UploadTriangleIB");
        cmd->writeBuffer(m_testIndexBuffer, indices, sizeof(indices));
        cmd->endMarker();
        cmd->close();
        m_nvrhiDevice->ExecuteCommandList(cmd);

        Msg("~ [TestRenderContext] Created index buffer");
    }

    if (!m_testVS || !m_testPS)
    {
        // Compile shaders
        xr_vector<u8> vsBlob = CompileShaderFromFile(
            "r3\\test_triangle.vs", "main", "vs_5_0"
        );

        xr_vector<u8> psBlob = CompileShaderFromFile(
            "r3\\test_triangle.ps", "main", "ps_5_0"
        );

        if (vsBlob.empty() || psBlob.empty())
        {
            Msg("! [TestRenderContext] Failed to compile shaders");
            return;
        }

        // Create shader objects
        nvrhi::ShaderDesc vsDesc;
        vsDesc.shaderType = nvrhi::ShaderType::Vertex;
        vsDesc.debugName = "TestTriangle_VS";

        m_testVS = device->createShader(vsDesc, vsBlob.data(), vsBlob.size());

        nvrhi::ShaderDesc psDesc;
        psDesc.shaderType = nvrhi::ShaderType::Pixel;
        psDesc.debugName = "TestTriangle_PS";

        m_testPS = device->createShader(psDesc, psBlob.data(), psBlob.size());

        Msg("~ [TestRenderContext] Compiled shaders");
    }

    if (!m_testTexture)
    {
        // Create 64x64 checkerboard test texture
        const u32 size = 64;
        u32 pixels[size * size];

        for (u32 y = 0; y < size; y++)
        {
            for (u32 x = 0; x < size; x++)
            {
                bool checker = ((x / 8) + (y / 8)) % 2 == 0;
                pixels[y * size + x] = checker ? 0xFFFFFFFF : 0xFFFF0000;  // White or red
            }
        }

        nvrhi::TextureDesc texDesc;
        texDesc.width = size;
        texDesc.height = size;
        texDesc.mipLevels = 1;
        texDesc.format = nvrhi::Format::RGBA8_UNORM;
        texDesc.debugName = "TestCheckerboard";
        texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
        texDesc.keepInitialState = true;
        texDesc.isShaderResource = true;

        m_testTexture = device->createTexture(texDesc);

        // Upload texture data
        cmd->open();
        cmd->beginMarker("UploadCheckerboard");
        cmd->writeTexture(m_testTexture, 0, 0, pixels, size * 4);
        cmd->endMarker();
        cmd->close();
        m_nvrhiDevice->ExecuteCommandList(cmd);

        Msg("~ [TestRenderContext] Created checkerboard texture");
    }

    if (!m_testSampler)
    {
        // Create sampler state
        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllFilters(true);  // Linear filtering
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);

        m_testSampler = device->createSampler(samplerDesc);

        Msg("~ [TestRenderContext] Created sampler");
    }

    // Create binding layout (before pipeline) - using NVRHI directly
    if (!m_testBindingLayout)
    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Pixel;  // Used in pixel shader
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem{}.setSlot(0).setType(nvrhi::ResourceType::Texture_SRV),  // t0
            nvrhi::BindingLayoutItem{}.setSlot(0).setType(nvrhi::ResourceType::Sampler)       // s0
        };

        m_testBindingLayout = device->createBindingLayout(layoutDesc);

        if (!m_testBindingLayout)
        {
            Msg("! [TestRenderContext] Failed to create binding layout");
            return;
        }

        Msg("~ [TestRenderContext] Created binding layout");
    }

    if (!m_testPipeline)
    {
        // Create graphics pipeline
        nvrhi::GraphicsPipelineDesc pipelineDesc;

        // Shaders
        pipelineDesc.VS = m_testVS;
        pipelineDesc.PS = m_testPS;

        // Create input layout
        nvrhi::VertexAttributeDesc attributes[] = {
            nvrhi::VertexAttributeDesc()
                .setName("POSITION")
                .setFormat(nvrhi::Format::RGB32_FLOAT)
                .setOffset(0)
                .setBufferIndex(0)
                .setElementStride(sizeof(float) * 9),  // 3 pos + 4 color + 2 uv
            nvrhi::VertexAttributeDesc()
                .setName("COLOR")
                .setFormat(nvrhi::Format::RGBA32_FLOAT)
                .setOffset(sizeof(float) * 3)
                .setBufferIndex(0)
                .setElementStride(sizeof(float) * 9),
            nvrhi::VertexAttributeDesc()
                .setName("TEXCOORD")
                .setFormat(nvrhi::Format::RG32_FLOAT)
                .setOffset(sizeof(float) * 7)  // After position and color
                .setBufferIndex(0)
                .setElementStride(sizeof(float) * 9)
        };

        nvrhi::InputLayoutHandle inputLayout = device->createInputLayout(
            attributes,
            3,  // attribute count (position, color, texcoord)
            m_testVS.Get()  // vertex shader
        );

        pipelineDesc.inputLayout = inputLayout;

        // Render state
        pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

        pipelineDesc.renderState.depthStencilState.depthTestEnable = false;
        pipelineDesc.renderState.depthStencilState.depthWriteEnable = false;

        pipelineDesc.renderState.rasterState.fillMode = nvrhi::RasterFillMode::Solid;
        pipelineDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;

        pipelineDesc.renderState.blendState.targets[0].enableBlend();

        // Attach binding layout for texture and sampler
        pipelineDesc.bindingLayouts = { m_testBindingLayout };

        // Note: We'll set the actual framebuffer when we begin the render pass
        // For now, create a temporary dummy texture and framebuffer for pipeline creation
        nvrhi::TextureDesc dummyTexDesc;
        dummyTexDesc.width = Device.dwWidth;
        dummyTexDesc.height = Device.dwHeight;
        dummyTexDesc.format = nvrhi::Format::RGBA8_UNORM;
        dummyTexDesc.isRenderTarget = true;
        dummyTexDesc.debugName = "DummyRT_ForPipelineCreation";
        dummyTexDesc.initialState = nvrhi::ResourceStates::RenderTarget;
        dummyTexDesc.keepInitialState = true;

        nvrhi::TextureHandle dummyTexture = device->createTexture(dummyTexDesc);

        nvrhi::FramebufferDesc tempFbDesc;
        tempFbDesc.addColorAttachment(dummyTexture);
        nvrhi::FramebufferHandle tempFramebuffer = device->createFramebuffer(tempFbDesc);

        m_testPipeline = device->createGraphicsPipeline(pipelineDesc, tempFramebuffer);

        if (!m_testPipeline)
        {
            Msg("! [TestRenderContext] Failed to create pipeline");
            return;
        }

        Msg("~ [TestRenderContext] Created pipeline");
    }

    // Create RenderContext if needed
    // NOTE: This test code is being phased out in favor of FrameGraph renderer
    // For now, we skip RenderContext creation since it requires RenderDevice, not nvrhi::IDevice
    // TODO: Remove this test code once FrameGraph is fully operational
    if (!m_renderContext)
    {
        Msg("! [TestRenderContext] Skipping - test code needs migration to use ng::RenderDevice");
        return;  // Skip this test for now
    }

    // Create binding set (binds actual texture and sampler resources)
    if (!m_testBindingSet)
    {
        using namespace xray::render::ng;

        BindingSetDesc setDesc;
        setDesc.layout = m_testBindingLayout.Get();
        setDesc.AddTexture(0, m_testTexture.Get());    // t0
        setDesc.AddSampler(0, m_testSampler.Get());    // s0

        m_testBindingSet = m_renderContext->CreateBindingSet(setDesc);

        if (!m_testBindingSet)
        {
            Msg("! [TestRenderContext] Failed to create binding set");
            return;
        }

        Msg("~ [TestRenderContext] Created binding set");
    }

    // === RENDER USING RENDERCONTEXT ===

    try
    {
        // Open command list
        cmd->open();

        // Get backbuffer
        ID3D11RenderTargetView* backbufferRTV = Target->get_base_rt();
        if (!backbufferRTV)
        {
            Msg("! [TestRenderContext] No backbuffer RTV");
            cmd->close();
            return;
        }

        ID3D11Resource* backbufferRes = nullptr;
        backbufferRTV->GetResource(&backbufferRes);

        nvrhi::TextureDesc backbufferDesc;
        backbufferDesc.width = Device.dwWidth;
        backbufferDesc.height = Device.dwHeight;
        backbufferDesc.format = nvrhi::Format::RGBA8_UNORM;
        backbufferDesc.isRenderTarget = true;
        backbufferDesc.debugName = "Backbuffer";
        backbufferDesc.dimension = nvrhi::TextureDimension::Texture2D;
        backbufferDesc.keepInitialState = true;
        backbufferDesc.initialState = nvrhi::ResourceStates::RenderTarget;

        nvrhi::TextureHandle backbuffer = device->createHandleForNativeTexture(
            nvrhi::ObjectTypes::D3D11_Resource,
            nvrhi::Object(backbufferRes),
            backbufferDesc
        );

        backbufferRes->Release();

        // Begin render pass
        RenderPassDesc passDesc;
        passDesc.renderTargets[0] = backbuffer;
        passDesc.numRenderTargets = 1;
        passDesc.clearColor = true;
        passDesc.clearValue.color[0] = 0.2f;  // Dark gray background
        passDesc.clearValue.color[1] = 0.2f;
        passDesc.clearValue.color[2] = 0.2f;
        passDesc.clearValue.color[3] = 1.0f;

        m_renderContext->BeginRenderPass(passDesc);

        // Set pipeline FIRST - NVRHI requires valid pipeline for setGraphicsState
        m_renderContext->SetPipeline(m_testPipeline.Get());

        // Set viewport
        m_renderContext->SetViewport(0, 0,
                                    (float)Device.dwWidth,
                                    (float)Device.dwHeight);

        // Set vertex buffer
        m_renderContext->SetVertexBuffer(0, m_testVertexBuffer.Get(), 0);

        // Set index buffer
        m_renderContext->SetIndexBuffer(m_testIndexBuffer.Get(),
                                       nvrhi::Format::R16_UINT, 0);

        // Bind texture and sampler
        m_renderContext->SetBindingSet(0, m_testBindingSet.Get());

        // Draw textured triangle!
        m_renderContext->DrawIndexed(3, 0, 0);

        // End render pass
        m_renderContext->EndRenderPass();

        // Close command list
        cmd->close();

        // Execute
        m_nvrhiDevice->ExecuteCommandList(cmd);

        // Present
        HW.Present();

    }
    catch (const std::exception& e)
    {
        Msg("! [TestRenderContext] Exception: %s", e.what());
    }
}
#endif

} // namespace xray::render::RENDER_NAMESPACE
