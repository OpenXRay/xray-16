#include "stdafx.h"
#include "xrAI.h"

#include "game_spawn_constructor.h"

#include <mmsystem.h>

#pragma comment(linker, "/STACK:0x800000,0x400000")

#pragma comment(lib, "winmm.LIB")

#include "xrCore/ModuleLookup.hpp"

#include "factory_api.h"

Factory_Create* create_entity = 0;
Factory_Destroy* destroy_entity = 0;

extern void xrCompiler(LPCSTR name, bool draft_mode, bool pure_covers, LPCSTR out_name);
extern void verify_level_graph(LPCSTR name, bool verbose);

static const char* h_str =
    "-? or -h == this help\n"
    "-f <NAME> == compile level.ai\n"
    "-s <NAME,...> == build game spawn data\n"
    "-verify <NAME> == verify compiled level.ai\n";

void Help() { MessageBox(0, h_str, "Command line options", MB_OK | MB_ICONINFORMATION); }
string_path INI_FILE;

LPCSTR GAME_CONFIG = "game.ltx";

extern void clear_temp_folder();

void execute(LPSTR cmd)
{
    // Load project
    string4096 name;
    name[0] = 0;
    if (strstr(cmd, "-f"))
        sscanf(strstr(cmd, "-f") + 2, "%s", name);
    else if (strstr(cmd, "-s"))
        sscanf(strstr(cmd, "-s") + 2, "%s", name);
    else if (strstr(cmd, "-verify"))
        sscanf(strstr(cmd, "-verify") + xr_strlen("-verify"), "%s", name);

    if (xr_strlen(name))
        xr_strcat(name, "\\");

    string_path prjName;
    prjName[0] = 0;
    bool can_use_name = false;
    if (xr_strlen(name) < sizeof(string_path))
    {
        can_use_name = true;
        FS.update_path(prjName, "$game_levels$", name);
    }

    FS.update_path(INI_FILE, "$game_config$", GAME_CONFIG);

    if (strstr(cmd, "-f"))
    {
        R_ASSERT3(can_use_name, "Too big level name", name);

        char* output = strstr(cmd, "-out");
        string256 temp0;
        if (output)
        {
            output += xr_strlen("-out");
            sscanf(output, "%s", temp0);
            _TrimLeft(temp0);
            output = temp0;
        }
        else
            output = (pstr)LEVEL_GRAPH_NAME;

        xrCompiler(prjName, !!strstr(cmd, "-draft"), !!strstr(cmd, "-pure_covers"), output);
    }
    else
    {
        if (strstr(cmd, "-s"))
        {
            if (xr_strlen(name))
                name[xr_strlen(name) - 1] = 0;
            char* output = strstr(cmd, "-out");
            string256 temp0, temp1;
            if (output)
            {
                output += xr_strlen("-out");
                sscanf(output, "%s", temp0);
                _TrimLeft(temp0);
                output = temp0;
            }
            char* start = strstr(cmd, "-start");
            if (start)
            {
                start += xr_strlen("-start");
                sscanf(start, "%s", temp1);
                _TrimLeft(temp1);
                start = temp1;
            }
            char* no_separator_check = strstr(cmd, "-no_separator_check");
            clear_temp_folder();

            const auto hFactory = XRay::LoadModule("xrSE_Factory");

            R_ASSERT2(hFactory->IsLoaded(), "Factory DLL raised exception during loading or there is no factory DLL at all");

#ifdef XR_X64
            pcstr create_entity_name = "create_entity";
            pcstr destroy_entity_name = "destroy_entity";
#else
            pcstr create_entity_name = "_create_entity@4";
            pcstr destroy_entity_name = "_destroy_entity@4";
#endif
            create_entity = (Factory_Create*)hFactory->GetProcAddress(create_entity_name);
            destroy_entity = (Factory_Destroy*)hFactory->GetProcAddress(destroy_entity_name);

            R_ASSERT(create_entity);
            R_ASSERT(destroy_entity);

            CGameSpawnConstructor(name, output, start, !!no_separator_check);

            create_entity = nullptr;
            destroy_entity = nullptr;
        }
        else if (strstr(cmd, "-verify"))
        {
            R_ASSERT3(can_use_name, "Too big level name", name);
            verify_level_graph(prjName, !strstr(cmd, "-noverbose"));
        }
    }
}

void Startup(LPSTR lpCmdLine)
{
    string4096 cmd;

    xr_strcpy(cmd, lpCmdLine);
    xr_strlwr(cmd);
    if (strstr(cmd, "-?") || strstr(cmd, "-h"))
    {
        Help();
        return;
    }
    if ((strstr(cmd, "-f") == 0) && (strstr(cmd, "-s") == 0) && (strstr(cmd, "-verify") == 0))
    {
        Help();
        return;
    }
    Logger.Initialize("xrAI");
    u32 dwStartupTime = timeGetTime();
    execute(cmd);
    // Show statistic
    string256 stats;
    u32 dwEndTime = timeGetTime();
    xr_sprintf(stats, "Time elapsed: %s", make_time((dwEndTime - dwStartupTime) / 1000).c_str());
    Logger.Success(stats);
    FlushLog();
    Logger.Destroy();
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    xrDebug::Initialize(false);
    Core.Initialize("xrAI");

    Startup(lpCmdLine);

    Core._destroy();

    return 0;
}
