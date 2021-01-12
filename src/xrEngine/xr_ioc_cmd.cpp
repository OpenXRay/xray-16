#include "stdafx.h"
#include "IGame_Level.h"

#include "x_ray.h"
#include "XR_IOConsole.h"
#include "xr_ioc_cmd.h"
#if !defined(XR_PLATFORM_LINUX)
#include "xrSASH.h"
#endif

#include "MonitorManager.hpp"

#include "CameraManager.h"
#include "Environment.h"
#include "xr_input.h"
#include "CustomHUD.h"

#include "xr_object.h"
#include "xr_object_list.h"

extern u32 Vid_SelectedMonitor;
extern u32 Vid_SelectedRefreshRate;
xr_vector<xr_token> VidQualityToken;

const xr_token vid_bpp_token[] = {{"16", 16}, {"32", 32}, {0, 0}};

const xr_token snd_precache_all_token[] = {{"off", 0}, {"on", 1}, {nullptr, 0}};

void IConsole_Command::InvalidSyntax()
{
    TInfo I;
    Info(I);
    Msg("~ Invalid syntax in call to '%s'", cName);
    Msg("~ Valid arguments: %s", I);

#if !defined(XR_PLATFORM_LINUX)
    g_SASH.OnConsoleInvalidSyntax(false, "~ Invalid syntax in call to '%s'", cName);
    g_SASH.OnConsoleInvalidSyntax(true, "~ Valid arguments: %s", I);
#endif
}

//-----------------------------------------------------------------------

void IConsole_Command::add_to_LRU(shared_str const& arg)
{
    if (arg.size() == 0 || bEmptyArgsHandled)
    {
        return;
    }

    bool dup = (std::find(m_LRU.begin(), m_LRU.end(), arg) != m_LRU.end());
    if (!dup)
    {
        m_LRU.push_back(arg);
        if (m_LRU.size() > LRU_MAX_COUNT)
        {
            m_LRU.erase(m_LRU.begin());
        }
    }
}

void IConsole_Command::add_LRU_to_tips(vecTips& tips)
{
    vecLRU::reverse_iterator it_rb = m_LRU.rbegin();
    vecLRU::reverse_iterator it_re = m_LRU.rend();
    for (; it_rb != it_re; ++it_rb)
    {
        tips.push_back((*it_rb));
    }
}

// =======================================================

class CCC_Quit : public IConsole_Command
{
public:
    CCC_Quit(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args)
    {
        // TerminateProcess(GetCurrentProcess(),0);
        Console->Hide();
        Engine.Event.Defer("KERNEL:disconnect");
        Engine.Event.Defer("KERNEL:quit");
    }
};
//-----------------------------------------------------------------------
class CCC_DbgStrCheck : public IConsole_Command
{
public:
    CCC_DbgStrCheck(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args) { g_pStringContainer->verify(); }
};

class CCC_DbgStrDump : public IConsole_Command
{
public:
    CCC_DbgStrDump(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args) { g_pStringContainer->dump(); }
};
//-----------------------------------------------------------------------
class CCC_MotionsStat : public IConsole_Command
{
public:
    CCC_MotionsStat(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args)
    {
        // g_pMotionsContainer->dump();
        // TODO: move this console commant into renderer
        VERIFY(0);
    }
};
class CCC_TexturesStat : public IConsole_Command
{
public:
    CCC_TexturesStat(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args)
    {
        Device.DumpResourcesMemoryUsage();
        // Device.Resources->_DumpMemoryUsage();
        // TODO: move this console commant into renderer
        // VERIFY(0);
    }
};
//-----------------------------------------------------------------------
class CCC_E_Dump : public IConsole_Command
{
public:
    CCC_E_Dump(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args) { Engine.Event.Dump(); }
};
class CCC_E_Signal : public IConsole_Command
{
public:
    CCC_E_Signal(pcstr N) : IConsole_Command(N){};
    virtual void Execute(pcstr args)
    {
        char Event[128], Param[128];
        Event[0] = 0;
        Param[0] = 0;
        sscanf(args, "%[^,],%s", Event, Param);
        Engine.Event.Signal(Event, (u64)Param);
    }
};

//-----------------------------------------------------------------------
class CCC_Help : public IConsole_Command
{
public:
    CCC_Help(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args)
    {
        Log("- --- Command listing: start ---");
        CConsole::vecCMD_IT it;
        for (it = Console->Commands.begin(); it != Console->Commands.end(); ++it)
        {
            IConsole_Command& C = *(it->second);
            TStatus _S;
            C.GetStatus(_S);
            TInfo _I;
            C.Info(_I);

            Msg("%-20s (%-10s) --- %s", C.Name(), _S, _I);
        }
        Log("Key: Ctrl + A         === Select all ");
        Log("Key: Ctrl + C         === Copy to clipboard ");
        Log("Key: Ctrl + V         === Paste from clipboard ");
        Log("Key: Ctrl + X         === Cut to clipboard ");
        Log("Key: Ctrl + Z         === Undo ");
        Log("Key: Ctrl + Insert    === Copy to clipboard ");
        Log("Key: Shift + Insert   === Paste from clipboard ");
        Log("Key: Shift + Delete   === Cut to clipboard ");
        Log("Key: Insert           === Toggle mode <Insert> ");
        Log("Key: Back / Delete          === Delete symbol left / right ");

        Log("Key: Up   / Down            === Prev / Next command in tips list ");
        Log("Key: Ctrl + Up / Ctrl + Down === Prev / Next executing command ");
        Log("Key: Left, Right, Home, End {+Shift/+Ctrl}       === Navigation in text ");
        Log("Key: PageUp / PageDown      === Scrolling history ");
        Log("Key: Tab  / Shift + Tab     === Next / Prev possible command from list");
        Log("Key: Enter  / NumEnter      === Execute current command ");

        Log("- --- Command listing: end ----");
    }
};

XRCORE_API void _dump_open_files(int mode);
class CCC_DumpOpenFiles : public IConsole_Command
{
public:
    CCC_DumpOpenFiles(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(pcstr args)
    {
        int _mode = atoi(args);
        _dump_open_files(_mode);
    }
};

//-----------------------------------------------------------------------
class CCC_SaveCFG : public IConsole_Command
{
public:
    CCC_SaveCFG(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args)
    {
        string_path cfg_full_name;
        xr_strcpy(cfg_full_name, (xr_strlen(args) > 0) ? args : Console->ConfigFile);

        bool b_abs_name = xr_strlen(cfg_full_name) > 2 && cfg_full_name[1] == ':';

        if (!b_abs_name)
            FS.update_path(cfg_full_name, "$app_data_root$", cfg_full_name);

        if (strext(cfg_full_name))
            *strext(cfg_full_name) = 0;
        xr_strcat(cfg_full_name, ".ltx");

        bool b_allow = true;
#if defined(XR_PLATFORM_WINDOWS)
        if (FS.exist(cfg_full_name))
            b_allow = SetFileAttributes(cfg_full_name, FILE_ATTRIBUTE_NORMAL);
#endif
        if (b_allow)
        {
            IWriter* F = FS.w_open(cfg_full_name);
            CConsole::vecCMD_IT it;
            for (it = Console->Commands.begin(); it != Console->Commands.end(); ++it)
                it->second->Save(F);
            FS.w_close(F);
            Msg("Config-file [%s] saved successfully", cfg_full_name);
        }
        else
            Msg("!Cannot store config file [%s]", cfg_full_name);
    }
};
CCC_LoadCFG::CCC_LoadCFG(pcstr N) : IConsole_Command(N){};

void CCC_LoadCFG::Execute(pcstr args)
{
    Msg("Executing config-script \"%s\"...", args);
    string_path cfg_name;

    xr_strcpy(cfg_name, args);
    if (strext(cfg_name))
        *strext(cfg_name) = 0;
    xr_strcat(cfg_name, ".ltx");

    string_path cfg_full_name;

    FS.update_path(cfg_full_name, "$app_data_root$", cfg_name);

    if (!FS.exist(cfg_full_name))
        FS.update_path(cfg_full_name, "$fs_root$", cfg_name);

    if (!FS.exist(cfg_full_name))
        xr_strcpy(cfg_full_name, cfg_name);

    IReader* F = FS.r_open(cfg_full_name);

    string1024 str;
    if (F != nullptr)
    {
        while (!F->eof())
        {
            F->r_string(str, sizeof(str));
            if (allow(str))
                Console->Execute(str);
        }
        FS.r_close(F);
        Msg("[%s] successfully loaded.", cfg_full_name);
    }
    else
    {
        Msg("! Cannot open script file [%s]", cfg_full_name);
    }
}

CCC_LoadCFG_custom::CCC_LoadCFG_custom(pcstr cmd) : CCC_LoadCFG(cmd) { xr_strcpy(m_cmd, cmd); };
bool CCC_LoadCFG_custom::allow(pcstr cmd) { return (cmd == strstr(cmd, m_cmd)); };
//-----------------------------------------------------------------------
class CCC_Start : public IConsole_Command
{
    void parse(pstr dest, pcstr args, pcstr name)
    {
        dest[0] = 0;
        if (strstr(args, name))
            sscanf(strstr(args, name) + xr_strlen(name), "(%[^)])", dest);
    }

    void protect_Name_strlwr(pstr str)
    {
        string4096 out;
        xr_strcpy(out, sizeof(out), str);
        xr_strlwr(str);

        pcstr name_str = "name=";
        pcstr name1 = strstr(str, name_str);
        if (!name1 || !xr_strlen(name1))
        {
            return;
        }
        int begin_p = xr_strlen(str) - xr_strlen(name1) + xr_strlen(name_str);
        if (begin_p < 1)
        {
            return;
        }

        pcstr name2 = strchr(name1, '/');
        int end_p = xr_strlen(str) - ((name2) ? xr_strlen(name2) : 0);
        if (begin_p >= end_p)
        {
            return;
        }
        for (int i = begin_p; i < end_p; ++i)
        {
            str[i] = out[i];
        }
    }

public:
    CCC_Start(pcstr N) : IConsole_Command(N) { bLowerCaseArgs = false; };
    virtual void Execute(pcstr args)
    {
        /* if (g_pGameLevel) {
         Log ("! Please disconnect/unload first");
         return;
         }
         */
        string4096 op_server, op_client, op_demo;
        op_server[0] = 0;
        op_client[0] = 0;

        parse(op_server, args, "server"); // 1. server
        parse(op_client, args, "client"); // 2. client
        parse(op_demo, args, "demo"); // 3. demo

        xr_strlwr(op_server);
        protect_Name_strlwr(op_client);

        if (!op_client[0] && strstr(op_server, "single"))
            xr_strcpy(op_client, "localhost");

        if ((0 == xr_strlen(op_client)) && (0 == xr_strlen(op_demo)))
        {
            Log("! Can't start game without client. Arguments: '%s'.", args);
            return;
        }
        if (g_pGameLevel)
            Engine.Event.Defer("KERNEL:disconnect");

        if (xr_strlen(op_demo))
        {
            Engine.Event.Defer("KERNEL:start_mp_demo", u64(xr_strdup(op_demo)), 0);
        }
        else
        {
            Engine.Event.Defer(
                "KERNEL:start", u64(xr_strlen(op_server) ? xr_strdup(op_server) : 0), u64(xr_strdup(op_client)));
        }
    }
};

class CCC_Disconnect : public IConsole_Command
{
public:
    CCC_Disconnect(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args) { Engine.Event.Defer("KERNEL:disconnect"); }
};
//-----------------------------------------------------------------------
class CCC_VID_Reset : public IConsole_Command
{
public:
    CCC_VID_Reset(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args)
    {
        if (Device.b_is_Ready)
        {
            Device.Reset();
        }
    }
};
//-----------------------------------------------------------------------
class CCC_VidMode : public CCC_Token
{
    u32 _dummy = 0;

public:
    CCC_VidMode(pcstr name) : CCC_Token(name, &_dummy, nullptr)
    {
        bEmptyArgsHandled = false;
    }

    void Execute(pcstr args) override
    {
        u32 w, h;
        const int cnt = sscanf(args, "%dx%d", &w, &h);
        if (cnt == 2)
        {
            psCurrentVidMode[0] = w;
            psCurrentVidMode[1] = h;
        }
        else
        {
            Msg("! Wrong video mode [%s]", args);
        }
    }

    const xr_token* GetToken() noexcept override
    {
        return g_monitors.GetTokensForCurrentMonitor().data();
    }

    void GetStatus(TStatus& S) override
    {
        xr_sprintf(S, sizeof(S), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);
    }

    void Info(TInfo& I) override
    {
        xr_strcpy(I, sizeof(I), "change screen resolution WxH");
    }

    void fill_tips(vecTips& tips, u32 /*mode*/) override
    {
        g_monitors.FillResolutionsTips(tips);
    }
};
//-----------------------------------------------------------------------
class CCC_VidMonitor : public IConsole_Command
{
public:
    CCC_VidMonitor(pcstr name) : IConsole_Command(name)
    {
        bEmptyArgsHandled = false;
    }

    void Execute(pcstr args) override
    {
        u32 id = 0;

        const auto result = sscanf(args, "%u*", &id);
        const auto count = g_monitors.GetMonitorsCount();

        if (result != 1 || id < 1 || id > count)
            InvalidSyntax();
        else
            Vid_SelectedMonitor = id - 1;
    }

    void GetStatus(TStatus& S) override
    {
        const u32 id = Vid_SelectedMonitor; // readability
        xr_sprintf(S, sizeof(S), "%d. %s", id + 1, SDL_GetDisplayName(id));
    }

    void Info(TInfo& I) override
    {
        xr_strcpy(I, sizeof(I), "change monitor");
    }

    void fill_tips(vecTips& tips, u32 /*mode*/) override
    {
        g_monitors.FillMonitorsTips(tips);
    }
};
//-----------------------------------------------------------------------
class CCC_VidRefresh : public IConsole_Command
{
public:
    CCC_VidRefresh(pcstr name) : IConsole_Command(name)
    {
        bEmptyArgsHandled = false;
    }

    void Execute(pcstr args) override
    {
        if (!g_monitors.SelectedResolutionIsSafe())
        {
            Log("~ It's unsafe to set refresh rate for your resolution");
            return;
        }

        auto rates = g_monitors.GetRefreshRates();

        if (!rates)
        {
            Log("! No refresh rates for current resolution?!");
            return;
        }

        u32 value = static_cast<u32>(std::atoi(args));

        const auto it = std::find(rates->begin(), rates->end(), value);

        if (it == rates->end())
            InvalidSyntax();
        else
            Vid_SelectedRefreshRate = value;
    }

    void GetStatus(TStatus& S) override
    {
        xr_sprintf(S, sizeof(S), "%d", Vid_SelectedRefreshRate);
    }

    void Info(TInfo& I) override
    {
        xr_strcpy(I, sizeof(I), "change screen refresh rate");
    }

    void fill_tips(vecTips& tips, u32 /*mode*/) override
    {
        g_monitors.FillRatesTips(tips);
    }
};
//-----------------------------------------------------------------------
class CCC_SND_Restart : public IConsole_Command
{
public:
    CCC_SND_Restart(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args)
    {
        if (GEnv.Sound)
            GEnv.Sound->_restart();
    }
};

//-----------------------------------------------------------------------
float ps_gamma = 1.f, ps_brightness = 1.f, ps_contrast = 1.f;
class CCC_Gamma : public CCC_Float
{
public:
    CCC_Gamma(pcstr N, float* V) : CCC_Float(N, V, 0.5f, 1.5f) {}
    virtual void Execute(pcstr args)
    {
        CCC_Float::Execute(args);
        GEnv.Render->setGamma(ps_gamma);
        GEnv.Render->setBrightness(ps_brightness);
        GEnv.Render->setContrast(ps_contrast);
        GEnv.Render->updateGamma();
    }
};

//-----------------------------------------------------------------------
/*
#ifdef DEBUG
extern int g_bDR_LM_UsePointsBBox;
extern int g_bDR_LM_4Steps;
extern int g_iDR_LM_Step;
extern Fvector g_DR_LM_Min, g_DR_LM_Max;

class CCC_DR_ClearPoint : public IConsole_Command
{
public:
CCC_DR_ClearPoint(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
virtual void Execute(pcstr args) {
g_DR_LM_Min.x = 1000000.0f;
g_DR_LM_Min.z = 1000000.0f;

g_DR_LM_Max.x = -1000000.0f;
g_DR_LM_Max.z = -1000000.0f;

Msg("Local BBox (%f, %f) - (%f, %f)", g_DR_LM_Min.x, g_DR_LM_Min.z, g_DR_LM_Max.x, g_DR_LM_Max.z);
}
};

class CCC_DR_TakePoint : public IConsole_Command
{
public:
CCC_DR_TakePoint(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
virtual void Execute(pcstr args) {
Fvector CamPos = Device.vCameraPosition;

if (g_DR_LM_Min.x > CamPos.x) g_DR_LM_Min.x = CamPos.x;
if (g_DR_LM_Min.z > CamPos.z) g_DR_LM_Min.z = CamPos.z;

if (g_DR_LM_Max.x < CamPos.x) g_DR_LM_Max.x = CamPos.x;
if (g_DR_LM_Max.z < CamPos.z) g_DR_LM_Max.z = CamPos.z;

Msg("Local BBox (%f, %f) - (%f, %f)", g_DR_LM_Min.x, g_DR_LM_Min.z, g_DR_LM_Max.x, g_DR_LM_Max.z);
}
};

class CCC_DR_UsePoints : public CCC_Integer
{
public:
CCC_DR_UsePoints(pcstr N, int* V, int _min=0, int _max=999) : CCC_Integer(N, V, _min, _max) {};
virtual void Save (IWriter *F) {};
};
#endif
*/

ENGINE_API bool renderer_allow_override = false;

class CCC_renderer : public CCC_Token
{
    typedef CCC_Token inherited;

    u32 renderer_value = 0;
    static bool cmd_lock;

public:
    CCC_renderer(pcstr N) : inherited(N, &renderer_value, NULL) {};
    ~CCC_renderer() override {}
    void Execute(pcstr args) override
    {
        if ((renderer_allow_override == false) && (cmd_lock == true))
        {
            /*
             * It is a case when the renderer type was specified as
             * an application command line argument. This setting should
             * have the highest priority over other command invocations
             * (e.g. user config loading).
             * Since the Engine doesn't support switches between renderers
             * in runtime, it's safe to disable this command until restart.
             */
            Msg("Renderer is overrided by command line argument");
            return;
        }

        tokens = VidQualityToken.data();

        inherited::Execute(args);
    
        cmd_lock = true;
    }

    void Save(IWriter* F) override
    {
        if (renderer_allow_override == false)
        {   // Do not save forced value
            return;
        }

        tokens = VidQualityToken.data();
        inherited::Save(F);
    }

    const xr_token* GetToken() noexcept override
    {
        tokens = VidQualityToken.data();
        return inherited::GetToken();
    }
};
bool CCC_renderer::cmd_lock = false;

class CCC_VSync : public CCC_Mask
{
    using inherited = CCC_Mask;

public:
    CCC_VSync(pcstr name) : CCC_Mask(name, &psDeviceFlags, rsVSync) {}

    void Execute(pcstr args) override
    {
        // `apply` means that renderer asks to apply vsync settings
        // and we don't need to change it
        if (0 != xr_strcmp(args, "apply"))
            inherited::Execute(args);

        if (GEnv.Render->GetBackendAPI() != IRender::BackendAPI::OpenGL)
            return;

        if (psDeviceFlags.test(rsVSync))
        {
            // Try adaptive vsync first
            if (SDL_GL_SetSwapInterval(-1) == -1)
                SDL_GL_SetSwapInterval(1);
        }
        else
            SDL_GL_SetSwapInterval(0);
    }
};

class CCC_soundDevice : public CCC_Token
{
    typedef CCC_Token inherited;

public:
    CCC_soundDevice(pcstr N) : inherited(N, &snd_device_id, NULL){};
    virtual ~CCC_soundDevice() {}
    virtual void Execute(pcstr args)
    {
        GetToken();
        if (!tokens)
            return;
        inherited::Execute(args);
    }

    void GetStatus(TStatus& S) override
    {
        GetToken();
        if (!tokens)
            return;
        inherited::GetStatus(S);
    }

    const xr_token* GetToken() noexcept override
    {
        tokens = snd_devices_token;
        return inherited::GetToken();
    }

    virtual void Save(IWriter* F)
    {
        GetToken();
        if (!tokens)
            return;
        inherited::Save(F);
    }
};

//-----------------------------------------------------------------------

class CCC_ExclusiveMode : public IConsole_Command
{
private:
    typedef IConsole_Command inherited;

public:
    CCC_ExclusiveMode(pcstr N) : inherited(N) {}
    virtual void Execute(pcstr args)
    {
        bool value = false;
        if (!xr_strcmp(args, "on"))
            value = true;
        else if (!xr_strcmp(args, "off"))
            value = false;
        else if (!xr_strcmp(args, "true"))
            value = true;
        else if (!xr_strcmp(args, "false"))
            value = false;
        else if (!xr_strcmp(args, "1"))
            value = true;
        else if (!xr_strcmp(args, "0"))
            value = false;
        else
            InvalidSyntax();

        pInput->ExclusiveMode(value);
    }

    virtual void Save(IWriter* F) {}
};

class ENGINE_API CCC_HideConsole : public IConsole_Command
{
public:
    CCC_HideConsole(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; }
    virtual void Execute(pcstr args) { Console->Hide(); }
    void GetStatus(TStatus& S) override { S[0] = 0; }
    virtual void Info(TInfo& I) { xr_sprintf(I, sizeof(I), "hide console"); }
};

class CCC_CenterScreen : public IConsole_Command
{
public:
    CCC_CenterScreen(pcstr name) : IConsole_Command(name) { bEmptyArgsHandled = true; }
    void Execute(pcstr args) override
    {
        SDL_SetWindowPosition(Device.m_sdlWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
};

ENGINE_API float g_fov = 67.5f;
ENGINE_API float psHUD_FOV = 0.45f;

// extern int psSkeletonUpdate;
extern int rsDVB_Size;
extern int rsDIB_Size;
extern int psNET_ClientUpdate;
extern int psNET_ClientPending;
extern int psNET_ServerUpdate;
extern int psNET_ServerPending;
extern int psNET_DedicatedSleep;
extern char psNET_Name[32];
extern Flags32 psEnvFlags;
// extern float r__dtex_range;

extern int g_ErrorLineCount;
extern int ps_rs_loading_stages;

ENGINE_API int ps_always_active = 0;

ENGINE_API int ps_r__Supersample = 1;
ENGINE_API int ps_r__WallmarksOnSkeleton = 0;

void CCC_Register()
{
#ifdef DEBUG
    const bool isDebugMode = true;
#else // DEBUG
    const bool isDebugMode = !!strstr(Core.Params, "-debug");
#endif // DEBUG

    // General
    CMD1(CCC_Help, "help");
    CMD1(CCC_Quit, "quit");
    CMD1(CCC_Start, "start");
    CMD1(CCC_Disconnect, "disconnect");
    CMD1(CCC_SaveCFG, "cfg_save");
    CMD1(CCC_LoadCFG, "cfg_load");

#ifdef DEBUG
    CMD1(CCC_MotionsStat, "stat_motions");
    CMD1(CCC_TexturesStat, "stat_textures");
#endif // DEBUG

#ifdef DEBUG
    CMD3(CCC_Mask, "mt_particles", &psDeviceFlags, mtParticles);

    CMD1(CCC_DbgStrCheck, "dbg_str_check");
    CMD1(CCC_DbgStrDump, "dbg_str_dump");

    CMD3(CCC_Mask, "mt_sound", &psDeviceFlags, mtSound);
    CMD3(CCC_Mask, "mt_physics", &psDeviceFlags, mtPhysics);
    CMD3(CCC_Mask, "mt_network", &psDeviceFlags, mtNetwork);

    // Events
    CMD1(CCC_E_Dump, "e_list");
    CMD1(CCC_E_Signal, "e_signal");

    CMD3(CCC_Mask, "rs_clear_bb", &psDeviceFlags, rsClearBB);
    CMD3(CCC_Mask, "rs_occlusion", &psDeviceFlags, rsOcclusion);

    // CMD4(CCC_Float, "r__dtex_range", &r__dtex_range, 5, 175 );
    // CMD3(CCC_Mask, "rs_constant_fps", &psDeviceFlags, rsConstantFPS );
#endif // DEBUG

    // Console commands that are available with -debug key on release configuration and always available on mixed configuration
    if (isDebugMode)
    {
        CMD3(CCC_Mask, "rs_detail", &psDeviceFlags, rsDetails);
        CMD3(CCC_Mask, "rs_render_statics", &psDeviceFlags, rsDrawStatic);
        CMD3(CCC_Mask, "rs_render_dynamics", &psDeviceFlags, rsDrawDynamic);
        CMD3(CCC_Mask, "rs_render_particles", &psDeviceFlags, rsDrawParticles);
        CMD3(CCC_Mask, "rs_wireframe", &psDeviceFlags, rsWireframe);
    }

    // Render device states
    CMD4(CCC_Integer, "r__supersample", &ps_r__Supersample, 1, 4);
    CMD4(CCC_Integer, "r__wallmarks_on_skeleton", &ps_r__WallmarksOnSkeleton, 0, 1);

    CMD4(CCC_Integer, "rs_loadingstages", &ps_rs_loading_stages, 0, 1);
    CMD1(CCC_VSync, "rs_v_sync"); // If you change the name, you also should change it in glHW.cpp in the OpenGL renderer
    // CMD3(CCC_Mask, "rs_disable_objects_as_crows",&psDeviceFlags, rsDisableObjectsAsCrows );
    CMD3(CCC_Mask, "rs_fullscreen", &psDeviceFlags, rsFullscreen);
    CMD3(CCC_Mask, "rs_refresh_60hz", &psDeviceFlags, rsRefresh60hz);
    CMD3(CCC_Mask, "rs_stats", &psDeviceFlags, rsStatistic);
    CMD3(CCC_Mask, "rs_fps", &psDeviceFlags, rsShowFPS);
    CMD3(CCC_Mask, "rs_fps_graph", &psDeviceFlags, rsShowFPSGraph);
    CMD4(CCC_Float, "rs_vis_distance", &psVisDistance, 0.4f, 1.5f);

    CMD3(CCC_Mask, "rs_cam_pos", &psDeviceFlags, rsCameraPos);
#ifdef DEBUG
    CMD3(CCC_Mask, "rs_occ_draw", &psDeviceFlags, rsOcclusionDraw);
    CMD3(CCC_Mask, "rs_occ_stats", &psDeviceFlags, rsOcclusionStats);
// CMD4(CCC_Integer, "rs_skeleton_update", &psSkeletonUpdate, 2, 128 );
#endif // DEBUG

    CMD2(CCC_Gamma, "rs_c_gamma", &ps_gamma);
    CMD2(CCC_Gamma, "rs_c_brightness", &ps_brightness);
    CMD2(CCC_Gamma, "rs_c_contrast", &ps_contrast);
    // CMD4(CCC_Integer, "rs_vb_size", &rsDVB_Size, 32, 4096);
    // CMD4(CCC_Integer, "rs_ib_size", &rsDIB_Size, 32, 4096);

    // Texture manager
    CMD4(CCC_Integer, "texture_lod", &psTextureLOD, 0, 4);
    CMD4(CCC_Integer, "net_dedicated_sleep", &psNET_DedicatedSleep, 0, 64);

    // General video control
    CMD1(CCC_VidMode, "vid_mode");
    CMD1(CCC_VidMonitor, "vid_monitor");
    CMD1(CCC_VidRefresh, "vid_refresh")

#ifdef DEBUG
    CMD3(CCC_Token, "vid_bpp", &psCurrentBPP, vid_bpp_token);
#endif // DEBUG

    CMD1(CCC_VID_Reset, "vid_restart");

    // Sound
    CMD2(CCC_Float, "snd_volume_eff", &psSoundVEffects);
    CMD2(CCC_Float, "snd_volume_music", &psSoundVMusic);
    CMD1(CCC_SND_Restart, "snd_restart");
    CMD3(CCC_Mask, "snd_acceleration", &psSoundFlags, ss_Hardware);
    CMD3(CCC_Mask, "snd_efx", &psSoundFlags, ss_EAX);
    CMD4(CCC_Integer, "snd_targets", &psSoundTargets, 4, 32);
    CMD4(CCC_Integer, "snd_cache_size", &psSoundCacheSizeMB, 4, 64);
    CMD3(CCC_Token, "snd_precache_all", &psSoundPrecacheAll, snd_precache_all_token);

#ifdef DEBUG
    CMD3(CCC_Mask, "snd_stats", &g_stats_flags, st_sound);
    CMD3(CCC_Mask, "snd_stats_min_dist", &g_stats_flags, st_sound_min_dist);
    CMD3(CCC_Mask, "snd_stats_max_dist", &g_stats_flags, st_sound_max_dist);
    CMD3(CCC_Mask, "snd_stats_ai_dist", &g_stats_flags, st_sound_ai_dist);
    CMD3(CCC_Mask, "snd_stats_info_name", &g_stats_flags, st_sound_info_name);
    CMD3(CCC_Mask, "snd_stats_info_object", &g_stats_flags, st_sound_info_object);

    CMD4(CCC_Integer, "error_line_count", &g_ErrorLineCount, 6, 1024);
#endif // DEBUG

    // Mouse
    CMD3(CCC_Mask, "mouse_invert", &psMouseInvert, 1);
    psMouseSens = 0.12f;
    CMD4(CCC_Float, "mouse_sens", &psMouseSens, 0.001f, 0.6f);

    // Camera
    CMD2(CCC_Float, "cam_inert", &psCamInert);
    CMD2(CCC_Float, "cam_slide_inert", &psCamSlideInert);

    CMD1(CCC_CenterScreen, "center_screen");
    CMD4(CCC_Integer, "always_active", &ps_always_active, 0, 1);

    CMD1(CCC_renderer, "renderer");

    if (!GEnv.isDedicatedServer)
        CMD1(CCC_soundDevice, "snd_device");

    // psSoundRolloff = pSettings->r_float ("sound","rolloff"); clamp(psSoundRolloff, EPS_S, 2.f);
    psSoundOcclusionScale = pSettings->r_float("sound", "occlusion_scale");
    clamp(psSoundOcclusionScale, 0.1f, .5f);

    extern int g_Dump_Export_Obj;
    extern int g_Dump_Import_Obj;
    CMD4(CCC_Integer, "net_dbg_dump_export_obj", &g_Dump_Export_Obj, 0, 1);
    CMD4(CCC_Integer, "net_dbg_dump_import_obj", &g_Dump_Import_Obj, 0, 1);

#ifdef DEBUG
    CMD1(CCC_DumpOpenFiles, "dump_open_files");
#endif

    CMD1(CCC_ExclusiveMode, "input_exclusive_mode");
#if !defined(XR_PLATFORM_LINUX)
    extern int g_svTextConsoleUpdateRate;
    CMD4(CCC_Integer, "sv_console_update_rate", &g_svTextConsoleUpdateRate, 1, 100);
#endif
    extern int g_svDedicateServerUpdateReate;
    CMD4(CCC_Integer, "sv_dedicated_server_update_rate", &g_svDedicateServerUpdateReate, 1, 1000);

    CMD1(CCC_HideConsole, "hide");

#ifdef DEBUG
    extern BOOL debug_destroy;
    CMD4(CCC_Integer, "debug_destroy", &debug_destroy, 0, 1);
#endif
};
