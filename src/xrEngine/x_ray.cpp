//-----------------------------------------------------------------------------
// File: x_ray.cpp
//
// Programmers:
// Oles - Oles Shishkovtsov
// AlexMX - Alexander Maksimchuk
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"

#include "ILoadingScreen.h"
#include "XR_IOConsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "xrCDB/ISpatial.h"
#if !defined(XR_PLATFORM_LINUX)
#include "xrSASH.h"
#endif
#include "xrServerEntities/smart_cast.h"
#include "xr_input.h"

//---------------------------------------------------------------------

ENGINE_API CApplication* pApp = nullptr;
extern CRenderDevice Device;

ENGINE_API int ps_rs_loading_stages = 0;

#ifdef MASTER_GOLD
#define NO_MULTI_INSTANCES
#endif // #ifdef MASTER_GOLD

//////////////////////////////////////////////////////////////////////////
struct _SoundProcessor : public pureFrame
{
    virtual void OnFrame()
    {
        // Msg ("------------- sound: %d [%3.2f,%3.2f,%3.2f]",u32(Device.dwFrame),VPUSH(Device.vCameraPosition));
        GEnv.Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
    }
} SoundProcessor;

pcstr _GetFontTexName(pcstr section)
{
    static const char* tex_names[] = {"texture800", "texture", "texture1600"};
    int def_idx = 1; // default 1024x768
    int idx = def_idx;

#if 0
    u32 w = Device.dwWidth;

    if (w <= 800) idx = 0;
    else if (w <= 1280)idx = 1;
    else idx = 2;
#else
    u32 h = Device.dwHeight;

    if (h <= 600)
        idx = 0;
    else if (h < 1024)
        idx = 1;
    else
        idx = 2;
#endif

    while (idx >= 0)
    {
        if (pSettings->line_exist(section, tex_names[idx]))
            return pSettings->r_string(section, tex_names[idx]);
        --idx;
    }
    return pSettings->r_string(section, tex_names[def_idx]);
}

void _InitializeFont(CGameFont*& F, pcstr section, u32 flags)
{
    pcstr font_tex_name = _GetFontTexName(section);
    R_ASSERT(font_tex_name);

    pcstr sh_name = pSettings->r_string(section, "shader");
    if (!F)
    {
        F = xr_new<CGameFont>(sh_name, font_tex_name, flags);
    }
    else
        F->Initialize(sh_name, font_tex_name);

    if (pSettings->line_exist(section, "size"))
    {
        float sz = pSettings->r_float(section, "size");
        if (flags & CGameFont::fsDeviceIndependent)
            F->SetHeightI(sz);
        else
            F->SetHeight(sz);
    }
    if (pSettings->line_exist(section, "interval"))
        F->SetInterval(pSettings->r_fvector2(section, "interval"));
}

CApplication::CApplication()
{
    loaded = false;
    ll_dwReference = 0;

    max_load_stage = 0;

    // events
    eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
    eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
    eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
    eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);
    eConsole = Engine.Event.Handler_Attach("KERNEL:console", this);
    eStartMPDemo = Engine.Event.Handler_Attach("KERNEL:start_mp_demo", this);

    // levels
    Level_Current = u32(-1);
    Level_Scan();

    // Font
    pFontSystem = nullptr;

    // Register us
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

    if (psDeviceFlags.test(mtSound))
        Device.seqFrameMT.Add(&SoundProcessor);
    else
        Device.seqFrame.Add(&SoundProcessor);

    Console->Show();

    // App Title
    loadingScreen = nullptr;
}

extern CInput* pInput;

CApplication::~CApplication()
{
    Console->Hide();

    // font
    xr_delete(pFontSystem);

    Device.seqFrameMT.Remove(&SoundProcessor);
    Device.seqFrame.Remove(&SoundProcessor);
    Device.seqFrame.Remove(this);

    // events
    Engine.Event.Handler_Detach(eConsole, this);
    Engine.Event.Handler_Detach(eDisconnect, this);
    Engine.Event.Handler_Detach(eStartLoad, this);
    Engine.Event.Handler_Detach(eStart, this);
    Engine.Event.Handler_Detach(eQuit, this);
    Engine.Event.Handler_Detach(eStartMPDemo, this);
}

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
    if (E == eQuit)
    {
        if (pInput != nullptr)
            pInput->GrabInput(false);
#if !defined(XR_PLATFORM_LINUX)
        g_SASH.EndBenchmark();
#endif
        SDL_Event quit = { SDL_QUIT };
        SDL_PushEvent(&quit);

        for (auto& level : Levels)
        {
            xr_free(level.folder);
            xr_free(level.name);
        }
        Levels.clear();
    }
    else if (E == eStart)
    {
        pstr op_server = pstr(P1);
        pstr op_client = pstr(P2);
        Level_Current = u32(-1);
        R_ASSERT(nullptr == g_pGameLevel);
        R_ASSERT(nullptr != g_pGamePersistent);
        Console->Execute("main_menu off");
        Console->Hide();
        //! this line is commented by Dima
        //! because I don't see any reason to reset device here
        //! Device.Reset (false);
        //-----------------------------------------------------------
        g_pGamePersistent->PreStart(op_server);
        //-----------------------------------------------------------
        g_pGameLevel = dynamic_cast<IGame_Level*>(NEW_INSTANCE(CLSID_GAME_LEVEL));
        R_ASSERT(g_pGameLevel);
        LoadBegin();
        g_pGamePersistent->Start(op_server);
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
            Console->Hide();
            g_pGameLevel->net_Stop();
            DEL_INSTANCE(g_pGameLevel);
            Console->Show();

            if ((false == Engine.Event.Peek("KERNEL:quit")) && (false == Engine.Event.Peek("KERNEL:start")))
            {
                Console->Execute("main_menu off");
                Console->Execute("main_menu on");
            }
        }
        if (g_pGamePersistent)
        {
            g_pGamePersistent->Disconnect();
        }
    }
    else if (E == eConsole)
    {
        pstr command = (pstr)P1;
        Console->ExecuteCommand(command, false);
        xr_free(command);
    }
    else if (E == eStartMPDemo)
    {
        pstr demo_file = pstr(P1);

        R_ASSERT(nullptr == g_pGameLevel);
        R_ASSERT(nullptr != g_pGamePersistent);

        Console->Execute("main_menu off");
        Console->Hide();
        Device.Reset(false);

        g_pGameLevel = smart_cast<IGame_Level*>(NEW_INSTANCE(CLSID_GAME_LEVEL));
        VERIFY(g_pGameLevel);
        shared_str server_options = g_pGameLevel->OpenDemoFile(demo_file);

        //-----------------------------------------------------------
        g_pGamePersistent->PreStart(server_options.c_str());
        //-----------------------------------------------------------

        LoadBegin();
        g_pGamePersistent->Start(""); // server_options.c_str()); - no prefetch !
        g_pGameLevel->net_StartPlayDemo();
        LoadEnd();

        xr_free(demo_file);
    }
}

static CTimer phase_timer;

void CApplication::LoadBegin()
{
    ll_dwReference++;
    if (1 == ll_dwReference)
    {
        loaded = false;

        if (!GEnv.isDedicatedServer)
            _InitializeFont(pFontSystem, "ui_font_letterica18_russian", 0);

        phase_timer.Start();
        load_stage = 0;
    }
}

void CApplication::LoadEnd()
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

void CApplication::SetLoadingScreen(ILoadingScreen* newScreen)
{
    R_ASSERT(!loadingScreen);
    loadingScreen = newScreen;
}

void CApplication::DestroyLoadingScreen()
{
    xr_delete(loadingScreen);
}

void CApplication::ShowLoadingScreen(bool show)
{
    loadingScreen->Show(show);
}

void CApplication::LoadDraw()
{
    if (loaded)
        return;

    Device.dwFrame += 1;

    if (!Device.RenderBegin())
        return;

    if (GEnv.isDedicatedServer)
        Console->OnRender();
    else
        load_draw_internal();

    Device.RenderEnd();
}

void CApplication::LoadForceDrop()
{
    loadingScreen->ForceDrop();
}

void CApplication::LoadForceFinish()
{
    loadingScreen->ForceFinish();
}

void CApplication::SetLoadStageTitle(pcstr _ls_title)
{
    loadingScreen->SetStageTitle(_ls_title);
}

void CApplication::LoadTitleInt(pcstr str1, pcstr str2, pcstr str3)
{
    loadingScreen->SetStageTip(str1, str2, str3);
}

void CApplication::LoadStage()
{
    VERIFY(ll_dwReference);
    Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
    phase_timer.Start();
    Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);

    if (g_pGamePersistent->GameType() == 1 && !xr_strcmp(g_pGamePersistent->m_game_params.m_alife, "alife"))
        max_load_stage = 18;
    else
        max_load_stage = 14;

    loadingScreen->Show(true);
    LoadDraw();
    ++load_stage;
}

void CApplication::LoadSwitch() {}
// Sequential
void CApplication::OnFrame()
{
    Engine.Event.OnFrame();
    g_SpatialSpace->update();
    g_SpatialSpacePhysic->update();
    if (g_pGameLevel)
        g_pGameLevel->SoundEvent_Dispatch();
}

void CApplication::Level_Append(pcstr folder)
{
    string_path N1, N2, N3, N4;
    strconcat(sizeof(N1), N1, folder, "level");
    strconcat(sizeof(N2), N2, folder, "level.ltx");
    strconcat(sizeof(N3), N3, folder, "level.geom");
    strconcat(sizeof(N4), N4, folder, "level.cform");
    if (FS.exist("$game_levels$", N1) && FS.exist("$game_levels$", N2) && FS.exist("$game_levels$", N3) &&
        FS.exist("$game_levels$", N4))
    {
        sLevelInfo LI;
        LI.folder = xr_strdup(folder);
        LI.name = nullptr;
        Levels.push_back(LI);
    }
}

void CApplication::Level_Scan()
{
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

    for (u32 i = 0; i < folder->size(); ++i)
        Level_Append((*folder)[i]);

    FS.file_list_close(folder);
}

void gen_logo_name(string_path& dest, pcstr level_name, int num = -1)
{
    strconcat(sizeof(dest), dest, "intro" DELIMITER "intro_", level_name);

    u32 len = xr_strlen(dest);
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

void CApplication::Level_Set(u32 L)
{
    if (L >= Levels.size())
        return;
    FS.get_path("$level$")->_set(Levels[L].folder);
    Level_Current = L;

    static string_path path;
    path[0] = 0;

    int count = 0;
    while (true)
    {
        if (set_logo_path(path, Levels[L].folder, count))
            count++;
        else
            break;
    }

    if (count)
    {
        const int num = ::Random.randI(count);
        gen_logo_name(path, Levels[L].folder, num);
    }
    else if (!set_logo_path(path, Levels[L].folder))
    {
        if (!set_logo_path(path, "no_start_picture"))
            path[0] = 0;
    }

    if (path[0])
        loadingScreen->SetLevelLogo(path);
}

int CApplication::Level_ID(pcstr name, pcstr ver, bool bSet)
{
    int result = -1;
    auto it = FS.m_archives.begin();
    auto it_e = FS.m_archives.end();
    bool arch_res = false;

    for (; it != it_e; ++it)
    {
        CLocatorAPI::archive& A = *it;
#if defined(XR_PLATFORM_WINDOWS)
        if (A.hSrcFile == nullptr)
#elif defined(XR_PLATFORM_LINUX)
        if (A.hSrcFile == 0)
#endif
        {
            pcstr ln = A.header->r_string("header", "level_name");
            pcstr lv = A.header->r_string("header", "level_ver");
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

CInifile* CApplication::GetArchiveHeader(pcstr name, pcstr ver)
{
    auto it = FS.m_archives.begin();
    auto it_e = FS.m_archives.end();

    for (; it != it_e; ++it)
    {
        CLocatorAPI::archive& A = *it;
        if (!A.header)
            continue;

        pcstr ln = A.header->r_string("header", "level_name");
        pcstr lv = A.header->r_string("header", "level_ver");
        if (0 == xr_stricmp(ln, name) && 0 == xr_stricmp(lv, ver))
        {
            return A.header;
        }
    }
    return nullptr;
}

void CApplication::LoadAllArchives()
{
    if (FS.load_all_unloaded_archives())
    {
        Level_Scan();
        g_pGamePersistent->OnAssetsChanged();
    }
}

#pragma optimize("g", off)
void CApplication::load_draw_internal(bool precaching /*= false*/)
{
    if (precaching)
    {
        const u32 total = Device.dwPrecacheTotal;
        loadingScreen->Update(total - Device.dwPrecacheFrame, total);
    }

    else if (loadingScreen->IsShown())
        loadingScreen->Update(load_stage, max_load_stage);

    else
        GEnv.Render->ClearTarget();
}
