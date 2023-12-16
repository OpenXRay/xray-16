#include "stdafx.h"

#include "XR_IOConsole.h"
#include "xrSASH.h"

// global variables
ENGINE_API bool g_bBenchmark = false;
string512 g_sBenchmarkName;

namespace
{
void RunBenchmark(pcstr name)
{
    g_bBenchmark = true;
    string_path cfgPath;
    FS.update_path(cfgPath, "$app_data_root$", name);
    CInifile ini(cfgPath);
    const u32 benchmarkCount = ini.line_count("benchmark");
    const size_t hyphenLtxLen = xr_strlen("-ltx ");
    for (u32 i = 0; i < benchmarkCount; i++)
    {
        pcstr benchmarkName, t;
        ini.r_line("benchmark", i, &benchmarkName, &t);
        xr_strcpy(g_sBenchmarkName, benchmarkName);
        shared_str benchmarkCommand = ini.r_string_wb("benchmark", benchmarkName);
        const auto cmdSize = benchmarkCommand.size() + 1;
        Core.Params = (char*)xr_realloc(Core.Params, cmdSize);
        xr_strcpy(Core.Params, cmdSize, benchmarkCommand.c_str());
        xr_strlwr(Core.Params);
        //InitInput();
        Engine.External.Initialize();
        //if (i)
        //    InitEngine();
        xr_strcpy(Console->ConfigFile, "user.ltx");
        if (strstr(Core.Params, "-ltx "))
        {
            string64 cfgName;
            sscanf(strstr(Core.Params, "-ltx ") + hyphenLtxLen, "%[^ ] ", cfgName);
            xr_strcpy(Console->ConfigFile, cfgName);
        }
        //Startup();
    }
}

bool CheckBenchmark()
{
    pcstr benchName = "-batch_benchmark ";
    if (strstr(Core.Params, benchName))
    {
        const size_t sz = xr_strlen(benchName);
        string64 benchmarkName;
        sscanf(strstr(Core.Params, benchName) + sz, "%[^ ] ", benchmarkName);
        RunBenchmark(benchmarkName);
        return true;
    }

    pcstr sashName = "-openautomate ";
    if (strstr(Core.Params, sashName))
    {
        const size_t sz = xr_strlen(sashName);
        string512 sashArg;
        sscanf(strstr(Core.Params, sashName) + sz, "%[^ ] ", sashArg);

        if (g_SASH.Init(sashArg))
            g_SASH.MainLoop();

        return true;
    }

    return false;
}
}
