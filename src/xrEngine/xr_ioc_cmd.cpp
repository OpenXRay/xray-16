#include "stdafx.h"
#include "IGame_Level.h"

#include "XR_IOConsole.h"
#include "xr_ioc_cmd.h"

#include "CameraManager.h"
#include "Environment.h"
#include "xr_input.h"
#include "CustomHUD.h"

#include "xr_object.h"
#include "xr_object_list.h"

xr_vector<xr_token> VidQualityToken;

extern xr_vector<xr_token> vid_monitor_token;
extern xr_map<u32, xr_vector<xr_token>> vid_mode_token;

const xr_token vid_bpp_token[] = {{"16", 16}, {"32", 32}, {0, 0}};

const xr_token snd_precache_all_token[] = {{"off", 0}, {"on", 1}, {nullptr, 0}};

void IConsole_Command::InvalidSyntax()
{
    TInfo I;
    Info(I);
    Msg("~ Invalid syntax in call to '%s'", cName);
    Msg("~ Valid arguments: %s", I);
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
            TStatus status;
            C.GetStatus(status);
            TInfo info;
            C.Info(info);

            Msg("%-20s (%-10s) --- %s", C.Name(), status, info);
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
            Msg("! Cannot store config file [%s]", cfg_full_name);
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
class CCC_VidMonitor : public CCC_Token
{
public:
    CCC_VidMonitor(pcstr name) : CCC_Token(name, &psDeviceMode.Monitor, nullptr) {}

    const xr_token* GetToken() noexcept override
    {
        return vid_monitor_token.data();
    }
};
//-----------------------------------------------------------------------
class CCC_VidMode : public CCC_Token
{
    u32 _dummy = 0;

public:
    CCC_VidMode(pcstr name) : CCC_Token(name, &_dummy, nullptr) {}

    void Execute(pcstr args) override
    {
        u32 w, h, r = 0;
        const int cnt = sscanf(args, "%ux%u (%uHz)", &w, &h, &r);
        if (cnt >= 2)
        {
            psDeviceMode.Width = w;
            psDeviceMode.Height = h;

            if (cnt == 3)
            {
                psDeviceMode.RefreshRate = r;
                m_Refresh60hz.set(fl_Refresh60hz, psDeviceMode.RefreshRate == 60);
            }
        }
        else
        {
            Msg("! Wrong video mode [%s]", args);
        }
    }

    const xr_token* GetToken() noexcept override
    {
        return vid_mode_token[psDeviceMode.Monitor].data();
    }

    void GetStatus(TStatus& S) override
    {
        xr_sprintf(S, "%ux%u (%uHz)", psDeviceMode.Width, psDeviceMode.Height, psDeviceMode.RefreshRate);
    }

    void Info(TInfo& I) override
    {
        xr_strcpy(I, sizeof(I), "change screen resolution WxH (RHz)");
    }

    void fill_tips(vecTips& tips, u32 /*mode*/) override
    {
        TStatus buf;
        xr_sprintf(buf, "%ux%u (%dHz) (current)", psDeviceMode.Width, psDeviceMode.Height, psDeviceMode.RefreshRate);
        tips.push_back(buf);

        const xr_token* tok = GetToken();
        while (tok->name)
        {
            tips.push_back(tok->name);
            tok++;
        }
    }

private:
    enum { fl_Refresh60hz = 1u << 0u };
    inline static Flags32 m_Refresh60hz; // for rs_refresh_60hz backwards compatibility

public:
    class CCC_Refresh60hz final : public CCC_Mask
    {
    public:
        CCC_Refresh60hz(pcstr name) : CCC_Mask(name, &m_Refresh60hz, fl_Refresh60hz)
        {
            m_Refresh60hz.set(fl_Refresh60hz, psDeviceMode.RefreshRate == 60);
        }

        void Execute(pcstr args) override
        {
            CCC_Mask::Execute(args);
            if (GetValue())
                psDeviceMode.RefreshRate = 60;
            else
                psDeviceMode.RefreshRate = 0; // Device will adjust
        }
    };
};
using CCC_Refresh60hz = CCC_VidMode::CCC_Refresh60hz;
//-----------------------------------------------------------------------
class CCC_VidWindowMode final : public CCC_Token
{
    inline static xr_token vid_window_mode_token[] =
    {
        { "st_opt_windowed",                rsWindowed             },
        { "st_opt_windowed_borderless",     rsWindowedBorderless   },
        { "st_opt_fullscreen",              rsFullscreen           },
        { "st_opt_fullscreen_borderless",   rsFullscreenBorderless },
        { nullptr,                          -1                     },
    };

public:
    CCC_VidWindowMode(pcstr name) : CCC_Token(name, &psDeviceMode.WindowStyle, vid_window_mode_token) {}

    void Execute(pcstr args) override
    {
        CCC_Token::Execute(args);
        m_fullscreen.set(fl_fullscreen, psDeviceMode.WindowStyle == rsFullscreen);
    }

private:
    enum { fl_fullscreen = 1u << 0u };
    inline static Flags32 m_fullscreen; // for rs_fullscreen backwards compatibility

public:
    class CCC_Fullscreen final : public CCC_Mask
    {
    public:
        CCC_Fullscreen(pcstr name) : CCC_Mask(name, &m_fullscreen, fl_fullscreen)
        {
            m_fullscreen.set(fl_fullscreen, psDeviceMode.WindowStyle == rsFullscreen);
        }

        void Execute(pcstr args) override
        {
            CCC_Mask::Execute(args);
            if (GetValue())
                psDeviceMode.WindowStyle = rsFullscreen;
            else
                psDeviceMode.WindowStyle = rsWindowedBorderless;
        }
    };
};
using CCC_Fullscreen = CCC_VidWindowMode::CCC_Fullscreen;
//-----------------------------------------------------------------------
class CCC_SND_Restart : public IConsole_Command
{
public:
    CCC_SND_Restart(pcstr N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(pcstr args)
    {
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

// Anomaly
ENGINE_API float ps_r2_img_exposure = 1.0f; // r2-only
ENGINE_API float ps_r2_img_gamma = 1.0f; // r2-only
ENGINE_API float ps_r2_img_saturation = 1.0f; // r2-only
ENGINE_API Fvector ps_r2_img_cg = { .0f, .0f, .0f }; // r2-only
ENGINE_API Fvector4 ps_r2_mask_control = { .0f, .0f, .0f, .0f }; // r2-only, condition, vignette?, visor reflection, null?
ENGINE_API Fvector ps_r2_drops_control = { .0f, 1.15f, .0f }; // r2-only, power, null, speed
ENGINE_API int ps_r2_nightvision = 0; // beef's nvg enable

ENGINE_API Fvector4 ps_dev_param_1 = { .0f, .0f, .0f, .0f };
ENGINE_API Fvector4 ps_dev_param_2 = { .0f, .0f, .0f, .0f };
ENGINE_API Fvector4 ps_dev_param_3 = { .0f, .0f, .0f, .0f };
ENGINE_API Fvector4 ps_dev_param_4 = { .0f, .0f, .0f, .0f };
ENGINE_API Fvector4 ps_dev_param_5 = { .0f, .0f, .0f, .0f };
ENGINE_API Fvector4 ps_dev_param_6 = { .0f, .0f, .0f, .0f };
// beef's nvg
// x = generation (1.0-3.0) . num_tubes (0.10, 0.20, 0.40, 0.11, 0.12)
// y = gain_adjust (0.1-3.0) . washout_threshold (0.0 - 0.9)
// z = vignette power (0.0-1.0) . glitch power (0-0.9)
// w = gain offset (0.5-3.0) . mode (0.0-1.0)
// TODO: put into it's own var, keep dev params for dev work
ENGINE_API Fvector4 ps_dev_param_7 = { .0f, .0f, .0f, .0f };
// beef's nvg
// x = flipdown amount (1.0-100.0) . unused
// y = unused . nvg_radius (0.0, 0.9)
// z = unused
// w = unused
// TODO: put into it's own var, keep dev params for dev work
ENGINE_API Fvector4 ps_dev_param_8 = { .0f, .0f, .0f, .0f };

// Ascii1457's Screen Space Shaders
ENGINE_API Fvector4 ps_ssfx_hud_drops_1 = { 1.0f, 1.0f, 30.f, .05f }; // Anim Speed, Int, Reflection, Refraction
ENGINE_API Fvector4 ps_ssfx_hud_drops_2 = { .0225f, 1.f, 0.0f, 2.0f }; // Density, Size, Extra Gloss, Gloss
ENGINE_API Fvector4 ps_ssfx_hud_drops_1_cfg = { 3.0f, 1.f, 1.f, 50.f }; // Quantity of drops, Refrelction intensity, Refraction intensity, Speed of the drops animation
ENGINE_API Fvector4 ps_ssfx_hud_drops_2_cfg = { 50.f, 50.f, 0.75f, 2.f }; // Drops build up speed, Drying speed, Size of the drops, Raindrops gloss intensity
ENGINE_API Fvector4 ps_ssfx_blood_decals = { 0.6f, 0.6f, 0.f, 0.f };
ENGINE_API Fvector4 ps_ssfx_rain_1 = { 2.0f, 0.1f, 0.6f, 2.f }; // Len, Width, Speed, Quality
ENGINE_API Fvector4 ps_ssfx_rain_2 = { 0.7f, 0.1f, 1.0f, 0.5f }; // Alpha, Brigthness, Refraction, Reflection
ENGINE_API Fvector4 ps_ssfx_rain_3 = { 0.01f, 1.0f, 0.0f, 0.0f }; // Alpha, Refraction ( Splashes ) - Yohji: Alpha was edited (0.5->0.01f) due to a bug with transparency and other particles.
ENGINE_API Fvector3 ps_ssfx_shadow_cascades = { 20.f, 40.f, 160.f };
ENGINE_API Fvector4 ps_ssfx_grass_shadows = { .0f, .35f, 30.0f, .0f };
ENGINE_API Fvector4 ps_ssfx_grass_interactive = { 1.0f, 8.f, 2000.0f, 1.0f };
ENGINE_API Fvector4 ps_ssfx_int_grass_params_1 = { 1.0f, 1.0f, 1.0f, 25.0f };
ENGINE_API Fvector4 ps_ssfx_int_grass_params_2 = { 1.0f, 5.0f, 1.0f, 1.0f };
ENGINE_API Fvector4 ps_ssfx_wpn_dof_1 = { .0f, .0f, .0f, .0f };
ENGINE_API Fvector4 ps_ssfx_wpn_dof_2 = { 0.15f, .0f, .0f, .0f };
ENGINE_API Fvector4 ps_ssfx_florafixes_1 = { 0.3f, 0.21f, 0.3f, 0.21f }; // Flora fixes 1
ENGINE_API Fvector4 ps_ssfx_florafixes_2 = { 2.0f, 1.0f, 0.0f, 0.0f }; // Flora fixes 2
ENGINE_API Fvector4 ps_ssfx_wetsurfaces_1 = { 1.0f, 1.0f, 1.0f, 1.0f }; // Wet surfaces 1
ENGINE_API Fvector4 ps_ssfx_wetsurfaces_2 = { 1.0f, 1.0f, 1.0f, 1.0f }; // Wet surfaces 2
ENGINE_API int ps_ssfx_is_underground = 0;
ENGINE_API int ps_ssfx_gloss_method = 1;
ENGINE_API float ps_ssfx_gloss_factor = 0.5f;
ENGINE_API Fvector3 ps_ssfx_gloss_minmax = { 0.0f,0.92f,0.0f }; // Gloss
ENGINE_API Fvector4 ps_ssfx_lightsetup_1 = { 0.35f, 0.5f, 0.0f, 0.0f }; // Spec intensity

ENGINE_API float ps_r3_dyn_wet_surf_near = 5.f; // 10.0f
ENGINE_API float ps_r3_dyn_wet_surf_far = 20.f; // 30.0f
ENGINE_API int ps_r3_dyn_wet_surf_sm_res = 256; // 256

int ps_disable_lens_flare = 1;

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
        tokens = Engine.Sound.GetDevicesList().data();
        return tokens;
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

class CCC_ControllerSensorEnable final : public CCC_Mask
{
public:
    CCC_ControllerSensorEnable(pcstr name, Flags32* value, u32 mask)
        : CCC_Mask(name, value, mask) {}

    void Execute(pcstr args) override
    {
        CCC_Mask::Execute(args);
        pInput->EnableControllerSensors(GetValue());
    }
};

class CCC_Editor : public IConsole_Command
{
public:
    CCC_Editor(pcstr name) : IConsole_Command(name) { bEmptyArgsHandled = true; }
    void Execute(pcstr args) override
    {
        Device.editor().SetState(xray::editor::ide::visible_state::full);
    }
};

ENGINE_API float g_fov = 67.5f;
ENGINE_API float psHUD_FOV = 0.45f;

// extern int psSkeletonUpdate;
extern int rsDVB_Size;
extern int rsDIB_Size;

extern int psNET_DedicatedSleep;

extern Flags32 psEnvFlags;
// extern float r__dtex_range;

extern int g_ErrorLineCount;

ENGINE_API int ps_r__Supersample = 1;
ENGINE_API int ps_r__WallmarksOnSkeleton = 0;
ENGINE_API shared_str current_player_hud_sect{};

Fvector3 ssfx_wetness_multiplier = { 1.0f, 0.3f, 0.0f };

extern int ps_fps_limit;
extern int ps_fps_limit_in_menu;

void CCC_Register()
{
    // General
    CMD1(CCC_Help, "help");
    CMD1(CCC_Quit, "quit");
    CMD1(CCC_Start, "start");
    CMD1(CCC_Disconnect, "disconnect");
    CMD1(CCC_SaveCFG, "cfg_save");
    CMD1(CCC_LoadCFG, "cfg_load");

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

    // CMD4(CCC_Float, "r__dtex_range", &r__dtex_range, 5, 175 );
    // CMD3(CCC_Mask, "rs_constant_fps", &psDeviceFlags, rsConstantFPS );
#endif // DEBUG

#ifndef MASTER_GOLD
    CMD3(CCC_Mask, "rs_detail", &psDeviceFlags, rsDrawDetails);
    CMD3(CCC_Mask, "rs_render_statics", &psDeviceFlags, rsDrawStatic);
    CMD3(CCC_Mask, "rs_render_dynamics", &psDeviceFlags, rsDrawDynamic);
    CMD3(CCC_Mask, "rs_render_particles", &psDeviceFlags, rsDrawParticles);
    CMD3(CCC_Mask, "rs_wireframe", &psDeviceFlags, rsWireframe);
#endif

    // Render device states
    CMD4(CCC_Integer, "r__supersample", &ps_r__Supersample, 1, 4);
    CMD4(CCC_Integer, "r__wallmarks_on_skeleton", &ps_r__WallmarksOnSkeleton, 0, 1);

    CMD1(CCC_Editor, "rs_editor");

    CMD4(CCC_Integer, "rs_fps_limit", &ps_fps_limit, 30, 501);
    CMD4(CCC_Integer, "rs_fps_limit_in_menu", &ps_fps_limit_in_menu, 30, 501);
    CMD3(CCC_Mask, "rs_always_active", &psDeviceFlags, rsAlwaysActive);
    CMD3(CCC_Mask, "rs_v_sync", &psDeviceFlags, rsVSync);
    // CMD3(CCC_Mask, "rs_disable_objects_as_crows",&psDeviceFlags, rsDisableObjectsAsCrows );
    CMD1(CCC_Fullscreen, "rs_fullscreen");
    CMD1(CCC_Refresh60hz, "rs_refresh_60hz");
    CMD3(CCC_Mask, "rs_stats", &psDeviceFlags, rsStatistic);
    CMD3(CCC_Mask, "rs_fps", &psDeviceFlags, rsShowFPS);
    CMD3(CCC_Mask, "rs_fps_graph", &psDeviceFlags, rsShowFPSGraph);
    CMD4(CCC_Float, "rs_vis_distance", &psVisDistance, 0.4f, 1.5f);

    CMD3(CCC_Mask, "rs_cam_pos", &psDeviceFlags, rsCameraPos);
#ifdef DEBUG
    CMD3(CCC_Mask, "rs_occ_draw", &psDeviceFlags, rsOcclusionDraw);
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
    CMD1(CCC_VidMonitor, "vid_monitor");
    CMD1(CCC_VidMode, "vid_mode");
    CMD1(CCC_VidWindowMode, "vid_window_mode");

#ifdef DEBUG
    CMD3(CCC_Token, "vid_bpp", &psDeviceMode.BitsPerPixel, vid_bpp_token);
#endif // DEBUG

    CMD1(CCC_VID_Reset, "vid_restart");

    // Sound
    CMD2(CCC_Float, "snd_volume_eff", &psSoundVEffects);
    CMD2(CCC_Float, "snd_volume_music", &psSoundVMusic);
    CMD1(CCC_SND_Restart, "snd_restart");
    CMD3(CCC_Mask, "snd_acceleration", &psSoundFlags, ss_Hardware);
    CMD3(CCC_Mask, "snd_efx", &psSoundFlags, ss_EFX);
    CMD3(CCC_Mask, "snd_use_float32", &psSoundFlags, ss_UseFloat32);
    CMD4(CCC_Integer, "snd_targets", &psSoundTargets, 4, 256);
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

    // Gamepad
    CMD3(CCC_Mask, "gamepad_invert_y", &psControllerInvertY, 1);
    psControllerStickSens = 0.02f;
    CMD4(CCC_Float, "gamepad_stick_sens", &psControllerStickSens, 0.001f, 0.6f);
    psControllerStickDeadZone = 15.f;
    CMD4(CCC_Float, "gamepad_stick_deadzone", &psControllerStickDeadZone, 1.f, 35.f);
    psControllerSensorSens = 0.5f;
    CMD4(CCC_Float, "gamepad_sensor_sens", &psControllerSensorSens, 0.01f, 3.f);
    psControllerSensorDeadZone = 0.005f;
    CMD4(CCC_Float, "gamepad_sensor_deadzone", &psControllerSensorDeadZone, 0.001f, 1.f);
    CMD3(CCC_ControllerSensorEnable, "gamepad_sensors_enable", &psControllerEnableSensors, 1);
    CMD4(CCC_Float, "gamepad_cursor_autohide_time", &psControllerCursorAutohideTime, 0.5f, 3.f);

    // Camera
    CMD2(CCC_Float, "cam_inert", &psCamInert);
    CMD2(CCC_Float, "cam_slide_inert", &psCamSlideInert);

    CMD1(CCC_CenterScreen, "center_screen");

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
#if defined(XR_PLATFORM_WINDOWS) // XXX: enable (remove ifdef) when text console will be available on Linux
    extern int g_svTextConsoleUpdateRate;
    CMD4(CCC_Integer, "sv_console_update_rate", &g_svTextConsoleUpdateRate, 1, 100);
#endif
    extern int g_svDedicateServerUpdateReate;
    CMD4(CCC_Integer, "sv_dedicated_server_update_rate", &g_svDedicateServerUpdateReate, 1, 1000);

    CMD1(CCC_HideConsole, "hide");

#ifdef DEBUG
    extern BOOL debug_destroy;
    CMD4(CCC_Integer, "debug_destroy", &debug_destroy, 0, 1);

    extern int g_bShowRedText;
    CMD4(CCC_Integer, "debug_show_red_text", &g_bShowRedText, 0, 1);
#endif

    CMD4(CCC_Vector3, "ssfx_wetness_multiplier", &ssfx_wetness_multiplier, Fvector3({ 0.1f, 0.1f, 0.0f} ), Fvector3({ 20.0f, 20.0f, 0.0f }));

    CMD4(CCC_Integer, "disable_lens_flare", &ps_disable_lens_flare, 0, 1);
};
