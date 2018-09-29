#include "stdafx.h"
#include "xrSASH.h"

#include "XR_IOConsole.h"
#include "xr_ioc_cmd.h"

xrSASH ENGINE_API g_SASH;

xrSASH::xrSASH()
    : m_bInited(false), m_bOpenAutomate(false), m_bBenchmarkRunning(false), m_bRunning(false), m_bReinitEngine(false),
      m_bExecutingConsoleCommand(false)
{
    ;
}

xrSASH::~xrSASH()
{
    VERIFY(!m_bRunning);
    VERIFY(!m_bBenchmarkRunning);
}

bool xrSASH::Init(const char* pszParam)
{
    oaVersion ver;
    oaBool res = oaInit(pszParam, &ver);
    if (res)
    {
        m_bInited = true;
        m_bOpenAutomate = true;

        Msg("oa:: Version: %d.%d.%d.%d", ver.Major, ver.Minor, ver.Minor, ver.Custom);

        return true;
    }
    else
    {
        m_bInited = true;
        xr_strcpy(m_strBenchCfgName, pszParam);
        Msg("oa:: Failed to init.");
        Msg("oa:: Running native path.");
        return false;
    }
}

void xrSASH::MainLoop()
{
    m_bRunning = true;
    m_bReinitEngine = false;

    if (m_bOpenAutomate)
    {
        LoopOA();
    }
    else
    {
        // Native benchmarks
        LoopNative();
    }

    m_bRunning = false;
}

void xrSASH::LoopOA()
{
    oaCommand Command;
    bool bExit = false;

    while (!bExit)
    {
        // It must be called on the oaCommand object sent to
        // oaGetNextCommand() before each call to oaGetNextCommand().
        oaInitCommand(&Command);
        switch (oaGetNextCommand(&Command))
        {
        /* No more commands, exit program */
        case OA_CMD_EXIT:
            Msg("SASH:: Exit.");
            bExit = true;
            break;

        /* Run as normal */
        case OA_CMD_RUN:
            // RunApp();
            // Msg("SASH:: GetCurrentOptions.");
            bExit = true;
            break;

        /* Enumerate all in-game options */
        case OA_CMD_GET_ALL_OPTIONS:
            GetAllOptions();
            break;

        /* Return the option values currently set */
        case OA_CMD_GET_CURRENT_OPTIONS:
            GetCurrentOptions();
            break;

        /* Set all in-game options */
        case OA_CMD_SET_OPTIONS:
            SetOptions();
            break;

        /* Enumerate all known benchmarks */
        case OA_CMD_GET_BENCHMARKS:
            GetBenchmarks();
            break;

        /* Run benchmark */
        case OA_CMD_RUN_BENCHMARK: RunBenchmark(Command.BenchmarkName); break;
        }
    }
}

void xrSASH::LoopNative()
{
    string_path in_file;
    FS.update_path(in_file, "$app_data_root$", m_strBenchCfgName);

    CInifile ini(in_file);

    IReader* R = FS.r_open(in_file);
    if (R)
    {
        FS.r_close(R);

        int test_count = ini.line_count("benchmark");
        LPCSTR test_name, t;
        shared_str test_command;

        for (int i = 0; i < test_count; ++i)
        {
            ini.r_line("benchmark", i, &test_name, &t);
            // xr_strcpy(g_sBenchmarkName, test_name);

            test_command = ini.r_string_wb("benchmark", test_name);
            u32 cmdSize = test_command.size() + 1;
            Core.Params = (char*)xr_realloc(Core.Params, cmdSize);
            xr_strcpy(Core.Params, cmdSize, test_command.c_str());
            xr_strlwr(Core.Params);

            RunBenchmark(test_name);

            // Output results
            ReportNative(test_name);
        }
    }
    else
        Msg("oa:: Native path can't find \"%s\" config file.", in_file);

    FlushLog();
}

void xrSASH::ReportNative(LPCSTR pszTestName)
{
    string_path fname;
    xr_sprintf(fname, sizeof(fname), "%s.result", pszTestName);
    FS.update_path(fname, "$app_data_root$", fname);
    CInifile res(fname, FALSE, FALSE, TRUE);

    // min/max/average
    float fMinFps = flt_max;
    float fMaxFps = flt_min;

    const u32 iWindowSize = 15;

    if (m_aFrimeTimes.size() > iWindowSize * 4)
    {
        for (u32 it = 0; it < m_aFrimeTimes.size() - iWindowSize; it++)
        {
            float fTime = 0;

            for (u32 i = 0; i < iWindowSize; ++i)
                fTime += m_aFrimeTimes[it + i];

            float fFps = iWindowSize / fTime;
            if (fFps < fMinFps)
                fMinFps = fFps;
            if (fFps > fMaxFps)
                fMaxFps = fFps;
        }
    }
    else
    {
        for (u32 it = 0; it < m_aFrimeTimes.size(); it++)
        {
            float fFps = 1.f / m_aFrimeTimes[it];
            if (fFps < fMinFps)
                fMinFps = fFps;
            if (fFps > fMaxFps)
                fMaxFps = fFps;
        }
    }

    // res.w_float ("general","test float", float(1.0f)/10.f, "dx-level required" );
    // res.w_float ("general","renderer", float(GlobalEnv.Render->get_generation())/10.f, "dx-level required" );
    // res.w_float ("general","average", rfps_average, "average for this run" );
    // res.w_float ("general","middle", rfps_middlepoint, "per-frame middle-point");
    float fTotal = 0;
    float fNumFrames = 0;
    for (u32 it = 0; it < m_aFrimeTimes.size(); it++)
    {
        string32 id;
        xr_sprintf(id, sizeof(id), "%07d", it);
        res.w_float("per_frame_stats", id, 1.f / m_aFrimeTimes[it]);
        fTotal += m_aFrimeTimes[it];
        fNumFrames += 1;
    }

    // Output statistics
    res.w_float("general", "average", fNumFrames / fTotal, "average for this run");
    res.w_float("general", "min", fMinFps, "absolute (smoothed) minimum");
    res.w_float("general", "max", fMaxFps, "absolute (smoothed) maximum");
}

void xrSASH::StartBenchmark()
{
    if (!m_bRunning)
        return;

    VERIFY(!m_bBenchmarkRunning);

    m_bBenchmarkRunning = true;
    oaStartBenchmark();

    if (!m_bOpenAutomate)
    {
        m_aFrimeTimes.clear();
        m_aFrimeTimes.reserve(1024);
        m_FrameTimer.Start();
    }
}

void xrSASH::DisplayFrame(float t)
{
    if (!m_bRunning)
        return;

    VERIFY(m_bBenchmarkRunning);
    oaDisplayFrame(t);

    if (!m_bOpenAutomate)
    {
        m_aFrimeTimes.push_back(m_FrameTimer.GetElapsed_sec());
        m_FrameTimer.Start();
    }
}

void xrSASH::EndBenchmark()
{
    if (!m_bRunning)
        return;

    VERIFY(m_bBenchmarkRunning);

    m_bBenchmarkRunning = false;
    oaEndBenchmark();
}

void InitInput();
void destroyInput();
void InitEngine();
void InitSound();
void destroySound();
void destroyEngine();

void xrSASH::GetAllOptions()
{
    Msg("SASH:: GetAllOptions.");
    TryInitEngine();

    oaNamedOptionStruct Option;
    oaInitOption(&Option);

    DescribeOption("renderer", Option.Dependency);
    DescribeOption("vid_mode", Option.Dependency);
    DescribeOption("rs_fullscreen", Option.Dependency);

    DescribeOption("rs_vis_distance", Option.Dependency);
    DescribeOption("r__geometry_lod", Option.Dependency);
    DescribeOption("r__detail_density", Option.Dependency);
    DescribeOption("texture_lod", Option.Dependency);
    DescribeOption("r__tf_aniso", Option.Dependency);
    DescribeOption("ai_use_torch_dynamic_lights", Option.Dependency);

    // r1 only
    Option.Dependency.ParentName = TEXT("renderer");
    Option.Dependency.ComparisonOp = OA_COMP_OP_EQUAL;
    Option.Dependency.ComparisonVal.Enum = (oaString)"renderer_r1";
    Option.Dependency.ComparisonValType = GetOptionType("renderer");
    {
        DescribeOption("r__supersample", Option.Dependency);
        DescribeOption("r1_no_detail_textures", Option.Dependency);
    }

    // >=r2
    oaInitOption(&Option); // Reset dependency info
    // Currently only equal/not equal works
    // Option.Dependency.ParentName = TEXT("renderer");
    // Option.Dependency.ComparisonOp = OA_COMP_OP_GREATER_OR_EQUAL;
    // Option.Dependency.ComparisonVal.Enum = TEXT("renderer_r2");
    // Option.Dependency.ComparisonValType = GetOptionType("renderer");
    {
        DescribeOption("r2_sun", Option.Dependency);
        DescribeOption("r2_sun_quality", Option.Dependency);
        DescribeOption("r2_slight_fade", Option.Dependency);
        DescribeOption("r2_ls_squality", Option.Dependency);
        DescribeOption("r2_detail_bump", Option.Dependency);
    }

    // >=r2.5
    // Option.Dependency.ParentName = TEXT("renderer");
    // Option.Dependency.ComparisonOp = OA_COMP_OP_GREATER_OR_EQUAL;
    // Option.Dependency.ComparisonVal.Enum = TEXT("renderer_r2.5");
    // Option.Dependency.ComparisonValType = GetOptionType("renderer");
    {
        DescribeOption("r2_sun_shafts", Option.Dependency);
        DescribeOption("r2_ssao", Option.Dependency);
        DescribeOption("r2_ssao_opt_data", Option.Dependency);
        DescribeOption("r2_ssao_half_data", Option.Dependency);
        DescribeOption("r2_ssao_hbao", Option.Dependency);
        DescribeOption("r2_soft_water", Option.Dependency);
        DescribeOption("r2_soft_particles", Option.Dependency);
        DescribeOption("r2_dof_enable", Option.Dependency);
        DescribeOption("r2_volumetric_lights", Option.Dependency);
        DescribeOption("r2_steep_parallax", Option.Dependency);
    }

    // >=r3
    // Option.Dependency.ParentName = TEXT("renderer");
    // Option.Dependency.ComparisonOp = OA_COMP_OP_GREATER_OR_EQUAL;
    // Option.Dependency.ComparisonVal.Enum = TEXT("renderer_r3");
    // Option.Dependency.ComparisonValType = GetOptionType("renderer");
    {
        DescribeOption("r3_dynamic_wet_surfaces", Option.Dependency);
        DescribeOption("r3_volumetric_smoke", Option.Dependency);
        DescribeOption("r3_gbuff_opt", Option.Dependency);
        DescribeOption("r3_use_dx10_1", Option.Dependency);
        DescribeOption("r3_minmax_sm", Option.Dependency);
        DescribeOption("r3_msaa", Option.Dependency);
        // >= 2x
        // Option.Dependency.ParentName = TEXT("r3_msaa");
        // Option.Dependency.ComparisonOp = OA_COMP_OP_GREATER_OR_EQUAL;
        // Option.Dependency.ComparisonVal.Enum = TEXT("2x");
        // Option.Dependency.ComparisonValType = GetOptionType("r3_msaa");
        {
            DescribeOption("r3_msaa_opt", Option.Dependency);
            DescribeOption("r3_msaa_alphatest", Option.Dependency);
        }
    }

    ReleaseEngine();
}

void xrSASH::GetCurrentOptions()
{
    Msg("SASH:: GetCurrentOptions.");
    TryInitEngine();

    GetOption("renderer");
    GetOption("vid_mode");
    GetOption("rs_fullscreen");

    GetOption("rs_vis_distance");
    GetOption("r__geometry_lod");
    GetOption("r__detail_density");
    GetOption("texture_lod");
    GetOption("r__tf_aniso");
    GetOption("ai_use_torch_dynamic_lights");

    // r1 only
    GetOption("r__supersample");
    GetOption("r1_no_detail_textures");

    // >=r2
    GetOption("r2_sun");
    GetOption("r2_sun_quality");
    GetOption("r2_slight_fade");
    GetOption("r2_ls_squality");
    GetOption("r2_detail_bump");

    // >=r2.5
    GetOption("r2_sun_shafts");
    GetOption("r2_ssao");
    GetOption("r2_ssao_opt_data");
    GetOption("r2_ssao_half_data");
    GetOption("r2_ssao_hbao");
    GetOption("r2_soft_water");
    GetOption("r2_soft_particles");
    GetOption("r2_dof_enable");
    GetOption("r2_volumetric_lights");
    GetOption("r2_steep_parallax");

    // >=r3
    GetOption("r3_dynamic_wet_surfaces");
    GetOption("r3_volumetric_smoke");
    GetOption("r3_use_dx10_1");
    GetOption("r3_minmax_sm");
    GetOption("r3_msaa");
    GetOption("r3_msaa_opt");
    GetOption("r3_msaa_alphatest");
    GetOption("r3_gbuff_opt");

    ReleaseEngine();
}

void xrSASH::SetOptions()
{
    Msg("SASH:: SetOptions.");
    TryInitEngine();

    oaNamedOption* Option;

    while ((Option = oaGetNextOption()) != NULL)
        SetOption(Option);

    // Console->Save();
    Console->Execute("cfg_save");

    ReleaseEngine();
}

void xrSASH::GetBenchmarks()
{
    Msg("SASH:: GetBenchmarks.");
    /* foreach known available benchmark */
    {
        /* Set BenchmarkName to a unique string identifying the benchmark */

        oaAddBenchmark(TEXT("dummy"));
        // sashAddBenchmark(TEXT("crates"));
        // sashAddBenchmark(TEXT("map1"));
    }
}

void Startup();

void xrSASH::RunBenchmark(LPCSTR pszName)
{
    Msg("SASH:: RunBenchmark.");

    TryInitEngine(false);

    Startup();

    m_bReinitEngine = true;

    // no need to release engine. Startup will close everything itself.
}

void xrSASH::TryInitEngine(bool bNoRun)
{
    if (m_bReinitEngine)
    {
        InitEngine();
        // It was destroyed on previous exit
        Console->Initialize();
    }

    xr_strcpy(Console->ConfigFile, "user.ltx");
    if (strstr(Core.Params, "-ltx "))
    {
        string64 c_name;
        sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
        xr_strcpy(Console->ConfigFile, c_name);
    }

    if (strstr(Core.Params, "-gl"))
        Console->Execute("renderer renderer_gl");
    else if (strstr(Core.Params, "-r4"))
        Console->Execute("renderer renderer_r4");
    else if (strstr(Core.Params, "-r3"))
        Console->Execute("renderer renderer_r3");
    else if (strstr(Core.Params, "-r2.5"))
        Console->Execute("renderer renderer_r2.5");
    else if (strstr(Core.Params, "-r2a"))
        Console->Execute("renderer renderer_r2a");
    else if (strstr(Core.Params, "-r2"))
        Console->Execute("renderer renderer_r2");
    else
    {
        CCC_LoadCFG_custom* pTmp = new CCC_LoadCFG_custom("renderer ");
        pTmp->Execute(Console->ConfigFile);
        if (m_bOpenAutomate)
            pTmp->Execute("SASH.ltx");
        else
            pTmp->Execute(Console->ConfigFile);
        xr_delete(pTmp);
    }

    InitInput();

    Engine.External.Initialize();

    Console->Execute("unbindall");
    Console->ExecuteScript(Console->ConfigFile);
    if (m_bOpenAutomate)
    {
        // Overwrite setting using SASH.ltx if has any.
        xr_strcpy(Console->ConfigFile, "SASH.ltx");
        Console->ExecuteScript(Console->ConfigFile);
    }

    if (bNoRun)
    {
        InitSound();
        Device.Create();
    }
}

void xrSASH::ReleaseEngine()
{
    m_bReinitEngine = true;

    destroyInput();
    Console->Destroy();
    destroySound();
    destroyEngine();
}

oaOptionDataType xrSASH::GetOptionType(pcstr pszOptionName)
{
    CConsole::vecCMD_IT I = Console->Commands.find(pszOptionName);
    if (I == Console->Commands.end())
    {
        Msg("SASH:: Option \"%s\" not found.", pszOptionName);
        VERIFY(I != Console->Commands.end());
        return OA_TYPE_BOOL;
    }

    IConsole_Command* pCmd = I->second;
    CCC_Mask* pMask = dynamic_cast<CCC_Mask*>(pCmd);
    CCC_Token* pToken = dynamic_cast<CCC_Token*>(pCmd);
    CCC_Float* pFloat = dynamic_cast<CCC_Float*>(pCmd);
    CCC_Integer* pInt = dynamic_cast<CCC_Integer*>(pCmd);

    if (pMask)
        return OA_TYPE_BOOL;
    else if (pToken)
        return OA_TYPE_ENUM;
    else if (pFloat)
        return OA_TYPE_FLOAT;
    else if (pInt)
        return OA_TYPE_INT;
    else
    {
        VERIFY(!"Unsupported console command type.");
        return OA_TYPE_BOOL;
    }
}

void xrSASH::DescribeOption(pcstr pszOptionName, const oaOptionDependency& Dependency)
{
    oaNamedOptionStruct Option;
    oaInitOption(&Option);

    Option.Dependency = Dependency;

    CConsole::vecCMD_IT I = Console->Commands.find(pszOptionName);
    if (I == Console->Commands.end())
    {
        Msg("SASH:: Option \"%s\" not found.", pszOptionName);
        VERIFY(I != Console->Commands.end());
        return;
    }

    IConsole_Command* pCmd = I->second;
    CCC_Mask* pMask = dynamic_cast<CCC_Mask*>(pCmd);
    CCC_Token* pToken = dynamic_cast<CCC_Token*>(pCmd);
    CCC_Float* pFloat = dynamic_cast<CCC_Float*>(pCmd);
    CCC_Integer* pInt = dynamic_cast<CCC_Integer*>(pCmd);

    Option.Name = pszOptionName;

    Msg("SASH:: Registering option \"%s\".", pszOptionName);

    if (pMask)
    {
        Option.DataType = OA_TYPE_BOOL;
        oaAddOption(&Option);
    }
    else if (pToken)
    {
        Option.DataType = OA_TYPE_ENUM;
        const xr_token* pXRToken = pToken->GetToken();

        while (pXRToken->name)
        {
            Option.Value.Enum = (char*)pXRToken->name;
            oaAddOption(&Option);
            ++pXRToken;
        }
    }
    else if (pFloat)
    {
        Option.DataType = OA_TYPE_FLOAT;
        float mn, mx;

        pFloat->GetBounds(mn, mx);
        Option.MinValue.Float = mn;
        Option.MaxValue.Float = mx;
        Option.NumSteps = (int)((mx - mn) / 0.1f);
        oaAddOption(&Option);
    }
    else if (pInt)
    {
        Option.DataType = OA_TYPE_INT;
        int mn, mx;
        pInt->GetBounds(mn, mx);
        Option.MinValue.Int = mn;
        Option.MaxValue.Int = mx;
        oaAddOption(&Option);
    }
    else
    {
        VERIFY(!"Unsupported console command type.");
    }
}

void xrSASH::GetOption(pcstr pszOptionName)
{
    oaValue Val;

    CConsole::vecCMD_IT I = Console->Commands.find(pszOptionName);
    if (I == Console->Commands.end())
    {
        Msg("SASH:: Option \"%s\" not found.", pszOptionName);
        VERIFY(I != Console->Commands.end());
        return;
    }

    IConsole_Command* pCmd = I->second;
    CCC_Mask* pMask = dynamic_cast<CCC_Mask*>(pCmd);
    CCC_Token* pToken = dynamic_cast<CCC_Token*>(pCmd);
    CCC_Float* pFloat = dynamic_cast<CCC_Float*>(pCmd);
    CCC_Integer* pInt = dynamic_cast<CCC_Integer*>(pCmd);

    Msg("SASH:: Getting option \"%s\".", pszOptionName);

    if (pMask)
    {
        Val.Bool = pMask->GetValue() ? OA_TRUE : OA_FALSE;
        oaAddOptionValue(pszOptionName, OA_TYPE_BOOL, &Val);
    }
    else if (pToken)
    {
        IConsole_Command::TStatus stat;
        pToken->GetStatus(stat);
        Val.Enum = stat;
        oaAddOptionValue(pszOptionName, OA_TYPE_ENUM, &Val);
    }
    else if (pFloat)
    {
        Val.Float = pFloat->GetValue();
        oaAddOptionValue(pszOptionName, OA_TYPE_FLOAT, &Val);
    }
    else if (pInt)
    {
        Val.Int = pInt->GetValue();
        oaAddOptionValue(pszOptionName, OA_TYPE_INT, &Val);
    }
    else
    {
        VERIFY(!"Unsupported console command type.");
    }
}

void xrSASH::SetOption(oaNamedOption* pOption)
{
    /*
    * Set option value to persist for subsequent runs of the game
    * to the given value. Option->Name will be the name of the value,
    * and Option->Value will contain the appropriate value.
    */
    CConsole::vecCMD_IT I = Console->Commands.find(pOption->Name);
    if (I == Console->Commands.end())
    {
        Msg("SASH:: Option \"%s\" not found.", pOption->Name);
        VERIFY(I != Console->Commands.end());
        return;
    }

    IConsole_Command* pCmd = I->second;
    CCC_Mask* pMask = dynamic_cast<CCC_Mask*>(pCmd);
    CCC_Token* pToken = dynamic_cast<CCC_Token*>(pCmd);
    CCC_Float* pFloat = dynamic_cast<CCC_Float*>(pCmd);
    CCC_Integer* pInt = dynamic_cast<CCC_Integer*>(pCmd);

    Msg("SASH:: Setting option \"%s\".", pOption->Name);

    string512 CmdBuf;

    if (pMask)
    {
        xr_sprintf(CmdBuf, "%s %s", pOption->Name, (pOption->Value.Bool ? "1" : "0"));
    }
    else if (pToken)
    {
        xr_sprintf(CmdBuf, "%s %s", pOption->Name, pOption->Value.Enum);
    }
    else if (pFloat)
    {
        xr_sprintf(CmdBuf, "%s %f", pOption->Name, pOption->Value.Float);
    }
    else if (pInt)
    {
        xr_sprintf(CmdBuf, "%s %d", pOption->Name, pOption->Value.Int);
    }
    else
    {
        VERIFY(!"Unsupported console command type.");
    }

    m_bExecutingConsoleCommand = true;
    Console->Execute(CmdBuf);
    m_bExecutingConsoleCommand = false;
}

void xrSASH::Message(oaErrorType MessageType, const char* pszMsg)
{
    VERIFY(m_bInited);

    oaMessage Message;
    oaInitMessage(&Message);
    Message.Error = MessageType;
    Message.Message = pszMsg;
    oaSendSignal(OA_SIGNAL_ERROR, &Message);
}

void xrSASH::Message(oaErrorType MessageType, const char* pszMsg, va_list& mark)
{
    VERIFY(m_bInited);

    string2048 buf;
    int sz = _vsnprintf(buf, sizeof(buf) - 1, pszMsg, mark);
    buf[sizeof(buf) - 1] = 0;

    if (sz)
        Message(MessageType, buf);
}

void xrSASH::OnConsoleInvalidSyntax(bool bLastLine, const char* pszMsg, ...)
{
    if (m_bInited && m_bExecutingConsoleCommand)
    {
        va_list mark;
        va_start(mark, pszMsg);

        if (bLastLine)
            Message(OA_ERROR_INVALID_OPTION_VALUE, pszMsg, mark);
        else
            Message(OA_ERROR_LOG, pszMsg, mark);

        va_end(mark);
    }
}
