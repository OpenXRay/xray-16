#include "stdafx.h"
#include "process.h"

#include "utils/xrLC_Light/xrlc_light.h"
#include "utils/xrLCUtil/LevelCompilerLoggerWindow.hpp"
#include "xrCore/cdecl_cast.hpp"
#include "utils/xrLCUtil/xrLCUtil.hpp"
//#pragma comment(linker,"/STACK:0x800000,0x400000")

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.LIB")
#pragma comment(lib, "xrCDB.lib")
#pragma comment(lib, "xrCore.lib")
#pragma comment(lib, "xrLC_Light.lib")
#pragma comment(lib, "xrLCUtil.lib")

ILevelCompilerLogger& Logger = LevelCompilerLoggerWindow();

CThread::LogFunc ProxyMsg = cdecl_cast([](const char* format, ...) {
    va_list args;
    va_start(args, format);
    Logger.clMsgV(format, args);
    va_end(args);
});

CThreadManager::ReportStatusFunc ProxyStatus = cdecl_cast([](const char* format, ...) {
    va_list args;
    va_start(args, format);
    Logger.StatusV(format, args);
    va_end(args);
});

CThreadManager::ReportProgressFunc ProxyProgress = cdecl_cast([](float progress) { Logger.Progress(progress); });

static const char* h_str =
    "The following keys are supported / required:\n"
    "-? or -h   == this help\n"
    "-f<NAME>   == compile level in gamedata\\levels\\<NAME>\\\n"
    "-o         == modify build options\n"
    "\n"
    "NOTE: The last key is required for any functionality\n";

void Help() { MessageBox(0, h_str, "Command line options", MB_OK | MB_ICONINFORMATION); }
void Startup(LPSTR lpCmdLine)
{
    char cmd[512];
    //  BOOL bModifyOptions     = FALSE;
    bool bNet = false;
    xr_strcpy(cmd, lpCmdLine);
    _strlwr(cmd);
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
    //  if (strstr(cmd,"-o"))                               bModifyOptions = TRUE;
    if (strstr(cmd, "-net"))
        bNet = true;
    // Load project
    char name[256];
    *name = 0;
    sscanf(strstr(cmd, "-f") + 2, "%s", name);
    string256 temp;
    xr_sprintf(temp, "%s - Detail Compiler", name);
    Logger.Initialize(temp);

    // FS.update_path    (name,"$game_levels$",name);
    FS.get_path("$level$")->_set(name);

    CTimer dwStartupTime;
    dwStartupTime.Start();

    xrCompileDO(bNet);

    // Show statistic
    char stats[256];
    xr_sprintf(stats, "Time elapsed: %s", make_time((dwStartupTime.GetElapsed_ms()) / 1000).c_str());

    if (!strstr(cmd, "-silent"))
        Logger.Success(stats);
    Logger.Destroy();
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize debugging
    xrDebug::Initialize(false);
    Core._initialize("xrDO");
    Startup(lpCmdLine);

    return 0;
}
