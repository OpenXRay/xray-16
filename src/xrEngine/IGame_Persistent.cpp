#include "stdafx.h"
#pragma hdrstop

#include "IGame_Persistent.h"
#include "GameFont.h"
#include "ILoadingScreen.h"
#include "PerformanceAlert.hpp"
#include "StringTable/StringTable.h"
#include "xrScriptEngine/script_engine.hpp"

#ifndef _EDITOR
#include "Environment.h"
#include "IGame_Level.h"
#include "XR_IOConsole.h"
#include "Render.h"
#include "PS_instance.h"
#include "CustomHUD.h"
#include "perlin.h"
#endif

ENGINE_API IGame_Persistent* g_pGamePersistent = nullptr;

IGame_Persistent::IGame_Persistent()
{
    ZoneScoped;

    eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
    eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
    eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);
    eStartMPDemo = Engine.Event.Handler_Attach("KERNEL:start_mp_demo", this);

    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1);
    Device.seqAppActivate.Add(this);
    Device.seqAppDeactivate.Add(this);

    PerlinNoise1D = xr_new<CPerlinNoise1D>(Random.randI(0, 0xFFFF));
    PerlinNoise1D->SetOctaves(2);
    PerlinNoise1D->SetAmplitude(0.66666f);

    pEnvironment = xr_new<CEnvironment>();

    m_pGShaderConstants = xr_new<ShadersExternalData>(); //--#SM+#--

    m_pSound = GEnv.Sound->create_scene();
    DefaultSoundScene = m_pSound;
}

IGame_Persistent::~IGame_Persistent()
{
    ZoneScoped;

    GEnv.Sound->destroy_scene(m_pSound);
    DefaultSoundScene = nullptr;

    Device.seqFrame.Remove(this);
    Device.seqAppActivate.Remove(this);
    Device.seqAppDeactivate.Remove(this);

    Engine.Event.Handler_Detach(eDisconnect, this);
    Engine.Event.Handler_Detach(eStartLoad, this);
    Engine.Event.Handler_Detach(eStart, this);
    Engine.Event.Handler_Detach(eStartMPDemo, this);

    xr_delete(PerlinNoise1D);
#ifndef _EDITOR
    xr_delete(pEnvironment);
#endif
    xr_delete(m_pGShaderConstants); //--#SM+#--
}

void IGame_Persistent::OnAppActivate() {}
void IGame_Persistent::OnAppDeactivate() {}

void IGame_Persistent::OnAppStart()
{
    ZoneScoped;

#ifndef _EDITOR
    Environment().load();
#endif

    Level_Scan();
}

void IGame_Persistent::OnAppEnd()
{
    ZoneScoped;

#ifndef _EDITOR
    Environment().unload();
#endif
    OnGameEnd();

    for (auto& level : Levels)
    {
        xr_free(level.folder);
        xr_free(level.name);
    }
    Levels.clear();
}

void IGame_Persistent::Level_Append(pcstr folder)
{
    ZoneScoped;
    string_path N1, N2, N3, N4;

    strconcat(N1, folder, "level");
    strconcat(N2, folder, "level.ltx");
    strconcat(N3, folder, "level.geom");
    strconcat(N4, folder, "level.cform");

    if (FS.exist("$game_levels$", N1) &&
        FS.exist("$game_levels$", N2) &&
        FS.exist("$game_levels$", N3) &&
        FS.exist("$game_levels$", N4))
    {
        Levels.emplace_back(sLevelInfo{ xr_strdup(folder), nullptr });
    }
}

void IGame_Persistent::Level_Scan()
{
    ZoneScoped;

    for (auto& level : Levels)
    {
        xr_free(level.folder);
        xr_free(level.name);
    }
    Levels.clear();

    xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
    if (!folder)
    {
        Log("! No levels found in game data");
        return;
    }

    for (cpcstr i : *folder)
        Level_Append(i);

    FS.file_list_close(folder);
}

void gen_logo_name(string_path& dest, pcstr level_name, int num = -1)
{
    strconcat(sizeof(dest), dest, "intro" DELIMITER "intro_", level_name);

    const auto len = xr_strlen(dest);
    if (dest[len - 1] == _DELIMITER)
        dest[len - 1] = 0;

    if (num < 0)
        return;

    string16 buff;
    xr_strcat(dest, sizeof(dest), "_");
    xr_strcat(dest, sizeof(dest), xr_itoa(num + 1, buff, 10));
}

// Return true if logo exists
// Always sets the path even if logo doesn't exist
bool set_logo_path(string_path& path, pcstr levelName, int count = -1)
{
    gen_logo_name(path, levelName, count);
    string_path temp2;
    return FS.exist(temp2, "$game_textures$", path, ".dds") || FS.exist(temp2, "$level$", path, ".dds");
}

void IGame_Persistent::Level_Set(u32 id)
{
    ZoneScoped;

    if (id >= Levels.size())
        return;
    FS.get_path("$level$")->_set(Levels[id].folder);
    Level_Current = id;

    static string_path path;
    path[0] = 0;

    int count = 0;
    while (true)
    {
        if (set_logo_path(path, Levels[id].folder, count))
            count++;
        else
            break;
    }

    if (count)
    {
        const int num = ::Random.randI(count);
        gen_logo_name(path, Levels[id].folder, num);
    }
    else if (!set_logo_path(path, Levels[id].folder))
    {
        if (!set_logo_path(path, "no_start_picture"))
            path[0] = 0;
    }

    if (path[0])
        m_pLoadingScreen->SetLevelLogo(path);
}

int IGame_Persistent::Level_ID(pcstr name, pcstr ver, bool bSet)
{
    ZoneScoped;

    int result = -1;
    bool arch_res = false;

    for (CLocatorAPI::archive& A : FS.m_archives)
    {
        if (!A.hSrcFile)
        {
            cpcstr ln = A.header->r_string("header", "level_name");
            cpcstr lv = A.header->r_string("header", "level_ver");
            if (0 == xr_stricmp(ln, name) && 0 == xr_stricmp(lv, ver))
            {
                FS.LoadArchive(A);
                arch_res = true;
            }
        }
    }

    if (arch_res)
        Level_Scan();

    string256 buffer;
    strconcat(sizeof(buffer), buffer, name, DELIMITER);
    for (u32 I = 0; I < Levels.size(); ++I)
    {
        if (0 == xr_stricmp(buffer, Levels[I].folder))
        {
            result = int(I);
            break;
        }
    }

    if (bSet && result != -1)
        Level_Set(result);

    if (arch_res)
        g_pGamePersistent->OnAssetsChanged();
    return result;
}

CInifile* IGame_Persistent::GetArchiveHeader(pcstr name, pcstr ver)
{
    for (const CLocatorAPI::archive& A : FS.m_archives)
    {
        if (!A.header)
            continue;

        cpcstr ln = A.header->r_string("header", "level_name");
        cpcstr lv = A.header->r_string("header", "level_ver");
        if (0 == xr_stricmp(ln, name) && 0 == xr_stricmp(lv, ver))
        {
            return A.header;
        }
    }
    return nullptr;
}

void IGame_Persistent::OnEvent(EVENT E, u64 P1, u64 P2)
{
    ZoneScoped;

    if (E == eStart)
    {
        pstr op_server = pstr(P1);
        pstr op_client = pstr(P2);
        Level_Current = u32(-1);
        R_ASSERT(nullptr == g_pGameLevel);
        Console->Execute("main_menu off");
        Console->Hide();
        //-----------------------------------------------------------
        PreStart(op_server);
        //-----------------------------------------------------------
        g_pGameLevel = CreateLevel();
        R_ASSERT(g_pGameLevel);
        LoadBegin();
        Start(op_server);
        g_pGameLevel->net_Start(op_server, op_client);
        LoadEnd();
        xr_free(op_server);
        xr_free(op_client);
    }
    else if (E == eDisconnect)
    {
        if (pInput != nullptr && true == Engine.Event.Peek("KERNEL:quit"))
            pInput->GrabInput(false);

        if (g_pGameLevel)
        {
            const bool show = Console->bVisible;
            Console->Hide();
            g_pGameLevel->net_Stop();
            DestroyLevel(g_pGameLevel);
            if (show)
                Console->Show();

            if ((false == Engine.Event.Peek("KERNEL:quit")) && (false == Engine.Event.Peek("KERNEL:start")))
            {
                Console->Execute("main_menu off");
                Console->Execute("main_menu on");
            }
        }
        Disconnect();
    }
    else if (E == eStartMPDemo)
    {
        pstr demo_file = pstr(P1);

        R_ASSERT(nullptr == g_pGameLevel);

        Console->Execute("main_menu off");
        Console->Hide();
        Device.Reset(false);

        g_pGameLevel = CreateLevel();
        VERIFY(g_pGameLevel);
        const shared_str server_options = g_pGameLevel->OpenDemoFile(demo_file);

        //-----------------------------------------------------------
        PreStart(server_options.c_str());
        //-----------------------------------------------------------

        LoadBegin();
        Start(""); // server_options.c_str()); - no prefetch !
        g_pGameLevel->net_StartPlayDemo();
        LoadEnd();

        xr_free(demo_file);
    }
}

void IGame_Persistent::PreStart(pcstr op)
{
    ZoneScoped;

    string256 prev_type;
    params new_game_params;
    xr_strcpy(prev_type, m_game_params.m_game_type);
    new_game_params.parse_cmd_line(op);

    // change game type
    if (0 != xr_strcmp(prev_type, new_game_params.m_game_type))
    {
        OnGameEnd();
    }
}
void IGame_Persistent::Start(pcstr op)
{
    ZoneScoped;

    string256 prev_type;
    xr_strcpy(prev_type, m_game_params.m_game_type);
    m_game_params.parse_cmd_line(op);
    // change game type
    if ((0 != xr_strcmp(prev_type, m_game_params.m_game_type)))
    {
        if (*m_game_params.m_game_type)
            OnGameStart();
    }
    else
        UpdateGameType();

    VERIFY(ps_destroy.empty());
}

void IGame_Persistent::Disconnect()
{
    ZoneScoped;
#ifndef _EDITOR
    // clear "need to play" particles
    destroy_particles(true);
#endif
}

void IGame_Persistent::OnGameStart()
{
    ZoneScoped;
#ifndef _EDITOR
    LoadTitle("st_prefetching_objects");
    if (!strstr(Core.Params, "-noprefetch"))
        Prefetch();
#endif
}

#ifndef _EDITOR
void IGame_Persistent::Prefetch()
{
    ZoneScoped;

    // prefetch game objects & models
    CTimer timer;
    timer.Start();
    const auto memoryBefore = Memory.mem_usage();

    Log("Loading objects...");
    ObjectPool.prefetch();

    Log("Loading models...");
    GEnv.Render->models_Prefetch();

    Log("Loading textures...");
    GEnv.Render->ResourcesDeferredUpload();

    const auto memoryAfter = Memory.mem_usage() - memoryBefore;

    Msg("* [prefetch] time:   %d ms", timer.GetElapsed_ms());
    Msg("* [prefetch] memory: %d Kb", memoryAfter / 1024);
}
#endif

void IGame_Persistent::OnGameEnd()
{
    ZoneScoped;

#ifndef _EDITOR
    ObjectPool.clear();
    GEnv.Render->models_Clear(true);
#endif
}

void IGame_Persistent::LoadBegin()
{
    ll_dwReference++;
    if (1 == ll_dwReference)
    {
        loaded = false;
        phase_timer.Start();
        load_stage = 0;
    }
}

void IGame_Persistent::LoadEnd()
{
    ll_dwReference--;
    if (0 == ll_dwReference)
    {
        Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
        Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
        Console->Execute("stat_memory");
        loaded = true;
    }
}

void IGame_Persistent::LoadTitle(pcstr ls_title, bool change_tip, shared_str map_name)
{
    ZoneScoped;

    if (ls_title)
    {
        string256 buff;
        xr_sprintf(buff, "%s%s", StringTable().translate(ls_title).c_str(), "...");
        m_pLoadingScreen->SetStageTitle(buff);
    }
    else if (!change_tip)
        m_pLoadingScreen->SetStageTitle("");

    if (change_tip)
    {
        ZoneScopedN("Change tip");

        bool noTips = false;
        string512 buff;
        u8 tip_num;
        luabind::functor<u8> m_functor;
        const bool is_single = !xr_strcmp(m_game_params.m_game_type, "single");
        if (is_single)
        {
            if (GEnv.ScriptEngine->functor("loadscreen.get_tip_number", m_functor))
                tip_num = m_functor(map_name.c_str());
            else
                noTips = true;
        }
        else
        {
            if (GEnv.ScriptEngine->functor("loadscreen.get_mp_tip_number", m_functor))
                tip_num = m_functor(map_name.c_str());
            else
                noTips = true;
        }
        if (noTips)
            return;

        xr_sprintf(buff, "%s%d:", StringTable().translate("ls_tip_number").c_str(), tip_num);
        const shared_str tmp = buff;

        if (is_single)
            xr_sprintf(buff, "ls_tip_%d", tip_num);
        else
            xr_sprintf(buff, "ls_mp_tip_%d", tip_num);

        m_pLoadingScreen->SetStageTip(StringTable().translate("ls_header").c_str(),
            tmp.c_str(),
            StringTable().translate(buff).c_str());
    }

    LoadStage();
}

void IGame_Persistent::LoadStage(bool draw /*= true*/)
{
    VERIFY(ll_dwReference);
    if (!load_screen_renderer.IsActive())
    {
        Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
        Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);
        phase_timer.Start();
    }

    if (GameType() == 1 && !xr_strcmp(m_game_params.m_alife, "alife"))
        max_load_stage = 18;
    else
        max_load_stage = 14;

    m_pLoadingScreen->Show(true);
    m_pLoadingScreen->Update(load_stage, max_load_stage);

    if (draw)
        LoadDraw();
    ++load_stage;
}

void IGame_Persistent::LoadDraw() const
{
    if (loaded)
        return;

    Device.dwFrame += 1;

    if (!Device.RenderBegin())
        return;

    // XXX: fix dedicated server
    //if (GEnv.isDedicatedServer)
    //    Console->OnRender();
    //else
        load_draw_internal();

    Device.RenderEnd();
}

void IGame_Persistent::load_draw_internal() const
{
    m_pLoadingScreen->Draw();
}

void IGame_Persistent::ShowLoadingScreen(bool show) const
{
    m_pLoadingScreen->Show(show);
}

void IGame_Persistent::OnFrame()
{
    ZoneScoped;

    SpatialSpace.update();
    SpatialSpacePhysic.update();

#ifndef _EDITOR
    if (!Device.Paused() || Device.dwPrecacheFrame)
        Environment().OnFrame();

    stats.Starting = ps_needtoplay.size();
    stats.Active = ps_active.size();
    stats.Destroying = ps_destroy.size();
    // Play req particle systems
    while (ps_needtoplay.size())
    {
        CPS_Instance* psi = ps_needtoplay.back();
        ps_needtoplay.pop_back();
        psi->Play(false);
    }
    // Destroy inactive particle systems
    while (ps_destroy.size())
    {
        // u32 cnt = ps_destroy.size();
        CPS_Instance* psi = ps_destroy.back();
        VERIFY(psi);
        if (psi->Locked())
        {
            Log("--locked");
            break;
        }
        ps_destroy.pop_back();
        psi->PSI_internal_delete();
    }
#endif
}

void IGame_Persistent::destroy_particles(const bool& all_particles)
{
    ZoneScoped;

#ifndef _EDITOR
    ps_needtoplay.clear();

    while (ps_destroy.size())
    {
        CPS_Instance* psi = ps_destroy.back();
        VERIFY(psi);
        VERIFY(!psi->Locked());
        ps_destroy.pop_back();
        psi->PSI_internal_delete();
    }

    // delete active particles
    if (all_particles)
    {
        for (; !ps_active.empty();)
            (*ps_active.begin())->PSI_internal_delete();
    }
    else
    {
        u32 active_size = ps_active.size();
        CPS_Instance** I = (CPS_Instance**)xr_alloca(active_size * sizeof(CPS_Instance*));
        std::copy(ps_active.begin(), ps_active.end(), I);

        CPS_Instance** E = std::remove_if(I, I + active_size, [](CPS_Instance* const& object)
        {
            return (!object->destroy_on_game_load());
        });
        for (; I != E; ++I)
            (*I)->PSI_internal_delete();
    }

    VERIFY(ps_needtoplay.empty() && ps_destroy.empty() && (!all_particles || ps_active.empty()));
#endif
}

//ECO_RENDER add
bool IGame_Persistent::IsMainMenuActive() const
{
    return m_pMainMenu && m_pMainMenu->IsActive();
}

bool IGame_Persistent::MainMenuActiveOrLevelNotExist() const
{
    return !g_pGameLevel || IsMainMenuActive();
}

void IGame_Persistent::OnAssetsChanged()
{
#ifndef _EDITOR
    GEnv.Render->OnAssetsChanged();
#endif
}

void IGame_Persistent::GrassBendersUpdate(u16 id, u8& data_idx, u32& data_frame, Fvector& position, float init_radius, float init_str, bool CheckDistance)
{
    // Interactive grass disabled
    if (ps_ssfx_grass_interactive.y < 1)
        return;

    // Just update position if not NULL
    if (data_idx != 0)
    {
        // Explosions can take the mem spot, unassign and try to get a spot later.
        if (grass_shader_data.id[data_idx] != id)
        {
            data_idx = 0;
            data_frame = Device.dwFrame + Random.randI(10, 35);
        }
        else
        {
            grass_shader_data.pos[data_idx] = position;
        }
    }

    if (Device.dwFrame < data_frame)
        return;

    // Wait some random frames to split the checks
    data_frame = Device.dwFrame + Random.randI(10, 35);

    // Check Distance
    if (CheckDistance)
    {
        if (position.distance_to_xz_sqr(Device.vCameraPosition) > ps_ssfx_grass_interactive.z)
        {
            GrassBendersRemoveByIndex(data_idx);
            return;
        }
    }

    CFrustum& view_frust = GEnv.Render->ViewBase;
    u32 mask = 0xff;
    float rad = data_idx == 0 ? 1.0 : std::max(1.0f, grass_shader_data.radius_curr[data_idx] + 0.5f);

    // In view frustum?
    if (!view_frust.testSphere(position, rad, mask))
    {
        GrassBendersRemoveByIndex(data_idx);
        return;
    }

    // Empty slot, let's use this
    if (data_idx == 0)
    {
        u8 idx = grass_shader_data.index + 1;

        // Add to grass blenders array
        if (grass_shader_data.id[idx] == 0)
        {
            data_idx = idx;
            GrassBendersSet(idx, id, position, Fvector3().set(0, -99, 0), 0, 0, 0.0f, init_radius, BENDER_ANIM_DEFAULT, true);

            grass_shader_data.str_target[idx] = init_str;
            grass_shader_data.radius_curr[idx] = init_radius;
        }
        // Back to 0 when the array limit is reached
        grass_shader_data.index = idx < ps_ssfx_grass_interactive.y ? idx : 0;
    }
    else
    {
        // Already in view, let's add more time to re-check
        data_frame += 60;
        grass_shader_data.pos[data_idx] = position;
    }
}

void IGame_Persistent::GrassBendersAddExplosion(u16 id, Fvector position, Fvector3 dir, float fade, float speed, float intensity, float radius)
{
    if (ps_ssfx_grass_interactive.y < 1)
        return;

    for (int idx = 1; idx < ps_ssfx_grass_interactive.y + 1; idx++)
    {
        // Add explosion to any spot not already taken by an explosion.
        if (grass_shader_data.anim[idx] != BENDER_ANIM_EXPLOSION)
        {
            // Add 99 to the ID to avoid conflicts between explosions and basic benders happening at the same time with the same ID.
            GrassBendersSet(idx, id + 99, position, dir, fade, speed, intensity, radius, BENDER_ANIM_EXPLOSION, true);
            grass_shader_data.str_target[idx] = intensity;
            break;
        }
    }
}

void IGame_Persistent::GrassBendersAddShot(u16 id, Fvector position, Fvector3 dir, float fade, float speed, float intensity, float radius)
{
    // Is disabled?
    if (ps_ssfx_grass_interactive.y < 1 || intensity <= 0.0f)
        return;

    // Check distance
    if (position.distance_to_xz_sqr(Device.vCameraPosition) > ps_ssfx_grass_interactive.z)
        return;

    int AddAt = -1;

    // Look for a spot
    for (int idx = 1; idx < ps_ssfx_grass_interactive.y + 1; idx++)
    {
        // Already exist, just update and increase intensity
        if (grass_shader_data.id[idx] == id)
        {
            float currentSTR = grass_shader_data.str[idx];
            GrassBendersSet(idx, id, position, dir, fade, speed, currentSTR, radius, BENDER_ANIM_EXPLOSION, false);
            grass_shader_data.str_target[idx] += intensity;
            AddAt = -1;
            break;
        }
        else
        {
            // Check all indexes and keep usable index to use later if needed...
            if (AddAt == -1 && fsimilar(grass_shader_data.radius[idx], 0.f))
                AddAt = idx;
        }
    }

    // We got an available index... Add bender at AddAt
    if (AddAt != -1)
    {
        GrassBendersSet(AddAt, id, position, dir, fade, speed, 0.001f, radius, BENDER_ANIM_EXPLOSION, true);
        grass_shader_data.str_target[AddAt] = intensity;
    }
}

void IGame_Persistent::GrassBendersUpdateAnimations()
{
    for (int idx = 1; idx < ps_ssfx_grass_interactive.y + 1; idx++)
    {
        if (grass_shader_data.id[idx] != 0)
        {
            switch (grass_shader_data.anim[idx])
            {
            case BENDER_ANIM_EXPLOSION: // Internal Only ( You can use BENDER_ANIM_PULSE for anomalies )
            {
                // Radius
                grass_shader_data.time[idx] += Device.fTimeDelta * grass_shader_data.speed[idx];
                grass_shader_data.radius_curr[idx] = grass_shader_data.radius[idx] * std::min(1.0f, grass_shader_data.time[idx]);

                grass_shader_data.str_target[idx] = std::min(1.0f, grass_shader_data.str_target[idx]);

                // Easing
                float diff = abs(grass_shader_data.str[idx] - grass_shader_data.str_target[idx]);
                diff = std::max(0.1f, diff);

                // Intensity
                if (grass_shader_data.str_target[idx] <= grass_shader_data.str[idx])
                {
                    grass_shader_data.str[idx] -= Device.fTimeDelta * grass_shader_data.fade[idx] * diff;
                }
                else
                {
                    grass_shader_data.str[idx] += Device.fTimeDelta * grass_shader_data.speed[idx] * diff;

                    if (grass_shader_data.str[idx] >= grass_shader_data.str_target[idx])
                        grass_shader_data.str_target[idx] = 0;
                }

                // Remove Bender
                if (grass_shader_data.str[idx] < 0.0f)
                    GrassBendersReset(idx);
            }
            break;

            case BENDER_ANIM_WAVY:
            {
                // Anim Speed
                grass_shader_data.time[idx] += Device.fTimeDelta * 1.5f * grass_shader_data.speed[idx];

                // Curve
                float curve = sin(grass_shader_data.time[idx]);

                // Intensity using curve
                grass_shader_data.str[idx] = curve * cos(curve * 1.4f) * 1.8f * grass_shader_data.str_target[idx];
            }

            break;

            case BENDER_ANIM_SUCK:
            {
                // Anim Speed
                grass_shader_data.time[idx] += Device.fTimeDelta * grass_shader_data.speed[idx];

                // Perlin Noise
                float curve = clampr(PerlinNoise1D->GetContinious(grass_shader_data.time[idx]) + 0.5f, 0.f, 1.f) * -1.0;

                // Intensity using Perlin
                grass_shader_data.str[idx] = curve * grass_shader_data.str_target[idx];
            }
            break;

            case BENDER_ANIM_BLOW:
            {
                // Anim Speed
                grass_shader_data.time[idx] += Device.fTimeDelta * 1.2f * grass_shader_data.speed[idx];

                // Perlin Noise
                float curve = clampr(PerlinNoise1D->GetContinious(grass_shader_data.time[idx]) + 1.0f, 0.f, 2.0f) * 0.25f;

                // Intensity using Perlin
                grass_shader_data.str[idx] = curve * grass_shader_data.str_target[idx];
            }
            break;

            case BENDER_ANIM_PULSE:
            {
                // Anim Speed
                grass_shader_data.time[idx] += Device.fTimeDelta * grass_shader_data.speed[idx];

                // Radius
                grass_shader_data.radius_curr[idx] = grass_shader_data.radius[idx] * std::min(1.0f, grass_shader_data.time[idx]);

                // Diminish intensity when radius target is reached
                if (grass_shader_data.radius_curr[idx] >= grass_shader_data.radius[idx])
                    grass_shader_data.str[idx] += GrassBenderToValue(grass_shader_data.str[idx], 0.0f, grass_shader_data.speed[idx] * 0.6f, true);

                // Loop when intensity is <= 0
                if (grass_shader_data.str[idx] <= 0.0f)
                {
                    grass_shader_data.str[idx] = grass_shader_data.str_target[idx];
                    grass_shader_data.radius_curr[idx] = 0.0f;
                    grass_shader_data.time[idx] = 0.0f;
                }

            }
            break;

            case BENDER_ANIM_DEFAULT:

                // Just fade to target strength
                grass_shader_data.str[idx] += GrassBenderToValue(grass_shader_data.str[idx], grass_shader_data.str_target[idx], 2.0f, true);

                break;
            }
        }
    }
}

void IGame_Persistent::GrassBendersRemoveByIndex(u8& idx)
{
    if (idx != 0)
    {
        GrassBendersReset(idx);
        idx = 0;
    }
}

void IGame_Persistent::GrassBendersRemoveById(u16 id)
{
    // Search by Object ID ( Used when removing benders CPHMovementControl::DestroyCharacter() )
    for (int i = 1; i < ps_ssfx_grass_interactive.y + 1; i++)
        if (grass_shader_data.id[i] == id)
            GrassBendersReset(i);
}

void IGame_Persistent::GrassBendersReset(u8 idx)
{
    // Reset Everything
    GrassBendersSet(idx, 0, Fvector3().set(0, 0, 0), Fvector3().set(0, -99, 0), 0, 0, 0, 0, BENDER_ANIM_DEFAULT, true);
    grass_shader_data.str_target[idx] = 0;
}

void IGame_Persistent::GrassBendersSet(u8 idx, u16 id, Fvector position, Fvector3 dir, float fade, float speed, float intensity, float radius, GrassBenders_Anim anim, bool resetTime)
{
    // Set values
    grass_shader_data.anim[idx] = anim;
    grass_shader_data.pos[idx] = position;
    grass_shader_data.id[idx] = id;
    grass_shader_data.radius[idx] = radius;
    grass_shader_data.str[idx] = intensity;
    grass_shader_data.fade[idx] = fade;
    grass_shader_data.speed[idx] = speed;
    grass_shader_data.dir[idx] = dir;

    if (resetTime)
    {
        grass_shader_data.radius_curr[idx] = 0.01f;
        grass_shader_data.time[idx] = 0;
    }
}

float IGame_Persistent::GrassBenderToValue(float& current, float go_to, float intensity, bool use_easing)
{
    float diff = abs(current - go_to);

    float r_value = Device.fTimeDelta * intensity * (use_easing ? std::min(0.5f, diff) : 1.0f);

    if (diff - r_value <= 0)
    {
        current = go_to;
        return 0;
    }

    return current < go_to ? r_value : -r_value;
}


void IGame_Persistent::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    // XXX: move to particle engine
    stats.FrameEnd();
    font.OutNext("Particles:");
    font.OutNext("- starting:   %u", stats.Starting);
    font.OutNext("- active:     %u", stats.Active);
    font.OutNext("- destroying: %u", stats.Destroying);
    stats.FrameStart();
}
