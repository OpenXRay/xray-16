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

#include "XR_IOConsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "xrCDB/ISpatial.h"
#include "xrSASH.h"
#include "xrServerEntities/smart_cast.h"

//---------------------------------------------------------------------

extern CRenderDevice Device;

#ifdef MASTER_GOLD
# define NO_MULTI_INSTANCES
#endif // #ifdef MASTER_GOLD

//////////////////////////////////////////////////////////////////////////
struct _SoundProcessor : public pureFrame
{
    virtual void  OnFrame()
    {
        //Msg ("------------- sound: %d [%3.2f,%3.2f,%3.2f]",u32(Device.dwFrame),VPUSH(Device.vCameraPosition));
        ::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
    }
} SoundProcessor;

LPCSTR _GetFontTexName(LPCSTR section)
{
    static const char* tex_names[] = {"texture800", "texture", "texture1600"};
    int def_idx = 1;//default 1024x768
    int idx = def_idx;

#if 0
    u32 w = Device.dwWidth;

    if (w <= 800) idx = 0;
    else if (w <= 1280)idx = 1;
    else idx = 2;
#else
    u32 h = Device.dwHeight;

    if (h <= 600) idx = 0;
    else if (h < 1024) idx = 1;
    else idx = 2;
#endif

    while (idx >= 0)
    {
        if (pSettings->line_exist(section, tex_names[idx]))
            return pSettings->r_string(section, tex_names[idx]);
        --idx;
    }
    return pSettings->r_string(section, tex_names[def_idx]);
}

void _InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
    LPCSTR font_tex_name = _GetFontTexName(section);
    R_ASSERT(font_tex_name);

    LPCSTR sh_name = pSettings->r_string(section, "shader");
    if (!F)
    {
        F = new CGameFont(sh_name, font_tex_name, flags);
    }
    else
        F->Initialize(sh_name, font_tex_name);

    if (pSettings->line_exist(section, "size"))
    {
        float sz = pSettings->r_float(section, "size");
        if (flags&CGameFont::fsDeviceIndependent) F->SetHeightI(sz);
        else F->SetHeight(sz);
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
    pFontSystem = NULL;

    // Register us
    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

    if (psDeviceFlags.test(mtSound)) Device.seqFrameMT.Add(&SoundProcessor);
    else Device.seqFrame.Add(&SoundProcessor);

    Console->Show();

    // App Title
    // app_title[ 0 ] = '\0';
    ls_header[0] = '\0';
    ls_tip_number[0] = '\0';
    ls_tip[0] = '\0';
}

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
        g_SASH.EndBenchmark();

        PostQuitMessage(0);

        for (u32 i = 0; i < Levels.size(); i++)
        {
            xr_free(Levels[i].folder);
            xr_free(Levels[i].name);
        }
    }
    else if (E == eStart)
    {
        LPSTR op_server = LPSTR(P1);
        LPSTR op_client = LPSTR(P2);
        Level_Current = u32(-1);
        R_ASSERT(0 == g_pGameLevel);
        R_ASSERT(0 != g_pGamePersistent);
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
        ls_header[0] = '\0';
        ls_tip_number[0] = '\0';
        ls_tip[0] = '\0';

        if (g_pGameLevel)
        {
            Console->Hide();
            g_pGameLevel->net_Stop();
            DEL_INSTANCE(g_pGameLevel);
            Console->Show();

            if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
            {
                Console->Execute("main_menu off");
                Console->Execute("main_menu on");
            }
        }
        R_ASSERT(0 != g_pGamePersistent);
        g_pGamePersistent->Disconnect();
    }
    else if (E == eConsole)
    {
        LPSTR command = (LPSTR)P1;
        Console->ExecuteCommand(command, false);
        xr_free(command);
    }
    else if (E == eStartMPDemo)
    {
        LPSTR demo_file = LPSTR(P1);

        R_ASSERT(0 == g_pGameLevel);
        R_ASSERT(0 != g_pGamePersistent);

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
        g_pGamePersistent->Start("");//server_options.c_str()); - no prefetch !
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

#ifndef DEDICATED_SERVER
        _InitializeFont(pFontSystem, "ui_font_letterica18_russian", 0);

        m_pRender->LoadBegin();
#endif
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
        // DUMP_PHASE;
    }
}

void CApplication::destroy_loading_shaders()
{
    m_pRender->destroy_loading_shaders();
    //hLevelLogo.destroy ();
    //sh_progress.destroy ();
    //. ::Sound->mute (false);
}

void CApplication::LoadDraw()
{
    if (loaded)
        return;

    Device.dwFrame += 1;


    if (!Device.Begin()) return;

    if (g_dedicated_server)
        Console->OnRender();
    else
        load_draw_internal();

    Device.End();
}

void CApplication::LoadTitleInt(LPCSTR str1, LPCSTR str2, LPCSTR str3)
{
    xr_strcpy(ls_header, str1);
    xr_strcpy(ls_tip_number, str2);
    xr_strcpy(ls_tip, str3);
    // LoadDraw ();
}
void CApplication::LoadStage()
{
    load_stage++;
    VERIFY(ll_dwReference);
    Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
    phase_timer.Start();
    Msg("* phase cmem: %d K", Memory.mem_usage() / 1024);

    if (g_pGamePersistent->GameType() == 1 && strstr(Core.Params, "alife"))
        max_load_stage = 17;
    else
        max_load_stage = 14;
    LoadDraw();
}

void CApplication::LoadSwitch()
{
}

// Sequential
void CApplication::OnFrame()
{
    Engine.Event.OnFrame();
    g_SpatialSpace->update();
    g_SpatialSpacePhysic->update();
    if (g_pGameLevel)
        g_pGameLevel->SoundEvent_Dispatch();
}

void CApplication::Level_Append(LPCSTR folder)
{
    string_path N1, N2, N3, N4;
    strconcat(sizeof(N1), N1, folder, "level");
    strconcat(sizeof(N2), N2, folder, "level.ltx");
    strconcat(sizeof(N3), N3, folder, "level.geom");
    strconcat(sizeof(N4), N4, folder, "level.cform");
    if (
        FS.exist("$game_levels$", N1) &&
        FS.exist("$game_levels$", N2) &&
        FS.exist("$game_levels$", N3) &&
        FS.exist("$game_levels$", N4)
    )
    {
        sLevelInfo LI;
        LI.folder = xr_strdup(folder);
        LI.name = 0;
        Levels.push_back(LI);
    }
}

void CApplication::Level_Scan()
{
    for (u32 i = 0; i < Levels.size(); i++)
    {
        xr_free(Levels[i].folder);
        xr_free(Levels[i].name);
    }
    Levels.clear();


    xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
    //. R_ASSERT (folder&&folder->size());

    for (u32 i = 0; i < folder->size(); ++i)
        Level_Append((*folder)[i]);

    FS.file_list_close(folder);
}

void gen_logo_name(string_path& dest, LPCSTR level_name, int num)
{
    strconcat(sizeof(dest), dest, "intro\\intro_", level_name);

    u32 len = xr_strlen(dest);
    if (dest[len - 1] == '\\')
        dest[len - 1] = 0;

    string16 buff;
    xr_strcat(dest, sizeof(dest), "_");
    xr_strcat(dest, sizeof(dest), itoa(num + 1, buff, 10));
}

void CApplication::Level_Set(u32 L)
{
    if (L >= Levels.size()) return;
    FS.get_path("$level$")->_set(Levels[L].folder);

    static string_path path;

    if (Level_Current != L)
    {
        path[0] = 0;

        Level_Current = L;

        int count = 0;
        while (true)
        {
            string_path temp2;
            gen_logo_name(path, Levels[L].folder, count);
            if (FS.exist(temp2, "$game_textures$", path, ".dds") || FS.exist(temp2, "$level$", path, ".dds"))
                count++;
            else
                break;
        }

        if (count)
        {
            int num = ::Random.randI(count);
            gen_logo_name(path, Levels[L].folder, num);
        }
    }

    if (path[0])
        m_pRender->setLevelLogo(path);
}

int CApplication::Level_ID(LPCSTR name, LPCSTR ver, bool bSet)
{
    int result = -1;
    CLocatorAPI::archives_it it = FS.m_archives.begin();
    CLocatorAPI::archives_it it_e = FS.m_archives.end();
    bool arch_res = false;

    for (; it != it_e; ++it)
    {
        CLocatorAPI::archive& A = *it;
        if (A.hSrcFile == NULL)
        {
            LPCSTR ln = A.header->r_string("header", "level_name");
            LPCSTR lv = A.header->r_string("header", "level_ver");
            if (0 == stricmp(ln, name) && 0 == stricmp(lv, ver))
            {
                FS.LoadArchive(A);
                arch_res = true;
            }
        }
    }

    if (arch_res)
        Level_Scan();

    string256 buffer;
    strconcat(sizeof(buffer), buffer, name, "\\");
    for (u32 I = 0; I < Levels.size(); ++I)
    {
        if (0 == stricmp(buffer, Levels[I].folder))
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

CInifile* CApplication::GetArchiveHeader(LPCSTR name, LPCSTR ver)
{
    CLocatorAPI::archives_it it = FS.m_archives.begin();
    CLocatorAPI::archives_it it_e = FS.m_archives.end();

    for (; it != it_e; ++it)
    {
        CLocatorAPI::archive& A = *it;

        LPCSTR ln = A.header->r_string("header", "level_name");
        LPCSTR lv = A.header->r_string("header", "level_ver");
        if (0 == stricmp(ln, name) && 0 == stricmp(lv, ver))
        {
            return A.header;
        }
    }
    return NULL;
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
void CApplication::load_draw_internal()
{
    m_pRender->load_draw_internal(*this);
}
