#include "stdafx.h"

#include "utils/xrLC_Light/xrlc_light.h"
//#pragma comment(linker,"/STACK:0x800000,0x400000")

static const char* h_str =
    "The following keys are supported / required:\n"
    "-? or -h   == this help\n"
    "-f<NAME>   == compile level in gamedata\\levels\\<NAME>\\\n"
    "\n"
    "NOTE: The last key is required for any functionality\n";

void Help() { MessageBox(0, h_str, "Command line options", MB_OK | MB_ICONINFORMATION); }
void Startup(pstr lpCmdLine)
{
    char cmd[512];
    bool bNet = false;
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
    if (strstr(cmd, "-net"))
        bNet = true;
    // Load project
    char name[256];
    *name = 0;
    sscanf(strstr(cmd, "-f") + 2, "%s", name);
    string256 temp;
    xr_sprintf(temp, "%s - Detail Compiler", name);
    Logger.Initialize(temp);

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
    xrDebug::Initialize(lpCmdLine);
    Core.Initialize("xrDO");

    Startup(lpCmdLine);

    return 0;
}
