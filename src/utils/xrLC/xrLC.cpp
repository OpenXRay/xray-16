// xrLC.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include <memory>
#include "math.h"
#include "build.h"
#include "Common/FSMacros.hpp"
#include "utils/xrLC_Light/xrLC_GlobalData.h"
#include "xrCore/ModuleLookup.hpp"

CBuild* pBuild = nullptr;
u32 version = 0;

static pcstr h_str =
    "The following keys are supported / required:\n"
    "-? or -h     == this help\n"
    "-o           == modify build options\n"
    "-nosun       == disable sun-lighting\n"
    "-skipinvalid == skip invalid faces\n"
    "-f<NAME>     == compile level in GameData\\Levels\\<NAME>\\\n"
    "\n"
    "NOTE: The last key is required for any functionality\n";

void Help() { MessageBox(nullptr, h_str, "Command line options", MB_OK | MB_ICONINFORMATION); }
typedef int __cdecl xrOptions(b_params* params, u32 version, bool bRunBuild);

void Startup(pstr lpCmdLine)
{
    create_global_data();
    string512 cmd;
    BOOL bModifyOptions = FALSE;

    xr_strcpy(cmd, lpCmdLine);
    xr_strlwr(cmd);
    if (strstr(cmd, "-?") || strstr(cmd, "-h"))
    {
        Help();
        return;
    }
    if (strstr(cmd, "-f") == 0)
    {
        Help();
        return;
    }
    if (strstr(cmd, "-o"))
        bModifyOptions = TRUE;
    if (strstr(cmd, "-gi"))
        g_build_options.b_radiosity = TRUE;
    if (strstr(cmd, "-noise"))
        g_build_options.b_noise = TRUE;
    if (strstr(cmd, "-net"))
        g_build_options.b_net_light = TRUE;
    if (strstr(cmd, "-skipinvalid"))
        g_build_options.b_skipinvalid = TRUE;
    VERIFY(lc_global_data());
    lc_global_data()->b_nosun_set(!!strstr(cmd, "-nosun"));
    // if (strstr(cmd,"-nosun"))                         b_nosun         = TRUE;
    char name[256];
    *name = 0;
    sscanf(strstr(cmd, "-f") + 2, "%s", name);
    string256 temp;
    xr_sprintf(temp, "%s - Levels Compiler", name);
    Logger.Initialize(temp);
    // Faster FPU
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    // Load project
    string_path prjName;
    FS.update_path(prjName, "$game_levels$", strconcat(sizeof(prjName), prjName, name, "\\build.prj"));
    string256 phaseName;
    Logger.Phase(strconcat(sizeof(phaseName), phaseName, "Reading project [", name, "]..."));

    string256 inf;
    IReader* F = FS.r_open(prjName);
    if (nullptr == F)
    {
        xr_sprintf(inf, "Build failed!\nCan't find level: '%s'", name);
        Logger.clMsg(inf);
        Logger.Failure(inf);
        Logger.Destroy();
        return;
    }

    // Version
    F->r_chunk(EB_Version, &version);
    Logger.clMsg("version: %d", version);
    R_ASSERT(XRCL_CURRENT_VERSION == version);

    // Header
    b_params Params;
    F->r_chunk(EB_Parameters, &Params);

    // Show options if needed
    if (bModifyOptions)
    {
        Logger.Phase("Project options...");
        const auto L = XRay::LoadModule("xrLC_Options");

        const auto O = (xrOptions*)L->GetProcAddress("_frmScenePropertiesRun");
        R_ASSERT(O);

        const int R = O(&Params, version, false);
        if (R == 2)
        {
            ExitProcess(0);
        }
    }

    // Conversion
    Logger.Phase("Converting data structures...");
    pBuild = xr_new<CBuild>();
    pBuild->Load(Params, *F);
    FS.r_close(F);

    // Call for builder
    string_path lfn;
    CTimer dwStartupTime;
    dwStartupTime.Start();
    FS.update_path(lfn, _game_levels_, name);
    pBuild->Run(lfn);
    xr_delete(pBuild);

    // Show statistic
    u32 dwEndTime = dwStartupTime.GetElapsed_ms();
    xr_sprintf(inf, "Time elapsed: %s", make_time(dwEndTime / 1000).c_str());
    Logger.clMsg("Build succesful!\n%s", inf);
    if (!strstr(cmd, "-silent"))
        Logger.Success(inf);
    Logger.Destroy();
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize debugging
    xrDebug::Initialize(lpCmdLine);
    Core.Initialize("xrLC");

    if (strstr(Core.Params, "-nosmg"))
        g_using_smooth_groups = false;

    Startup(lpCmdLine);

    Core._destroy();

    return 0;
}
