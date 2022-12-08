#pragma once

#ifdef XR_PLATFORM_WINDOWS
#include <OpenAutomate.h>

// struct oaOptionDependencyStruct;
// typedef struct oaOptionDependencyStruct oaOptionDependency;

// struct oaNamedOptionStruct;
// typedef struct oaNamedOptionStruct oaNamedOption;
#endif

class ENGINE_API xrSASH
{
public:
    ~xrSASH();

    // Execution control
    bool Init(const char* pszParam);
    void MainLoop();

    bool IsRunning() { return m_bRunning; }
    bool IsBenchmarkRunning() { return m_bBenchmarkRunning; }

    // Event handlers
    void StartBenchmark();
    void DisplayFrame(float t);
    void EndBenchmark();

    // Error report
    void OnConsoleInvalidSyntax(bool bLastLine, const char* pszMsg, ...);

private:
    // Internal loops
#ifdef XR_PLATFORM_WINDOWS
    void LoopOA();
#endif
    void LoopNative();

    // Native specific
    void ReportNative(pcstr pszTestName);

#ifdef XR_PLATFORM_WINDOWS
    // OA command handlers
    void GetAllOptions();
    void GetCurrentOptions();
    void SetOptions();
    void GetBenchmarks();
#endif

    void RunBenchmark(pcstr pszName);

    // Effectively restores/releases some engine systems
    void TryInitEngine(bool bNoRun = true);
    void ReleaseEngine();

#ifdef XR_PLATFORM_WINDOWS
    // OA option handling
    void DescribeOption(pcstr pszOptionName, const oaOptionDependency& Dependency);
    oaOptionDataType GetOptionType(pcstr pszOptionName);
    void GetOption(pcstr pszOptionName);
    void SetOption(oaNamedOption* pOption);

    // OA Error report
    void Message(oaErrorType MessageType, const char* pszMsg);
    void Message(oaErrorType MessageType, const char* pszMsg, va_list& mark);
#endif

private:
    // States
    bool m_bInited{};
    bool m_bOpenAutomate{};
    bool m_bRunning{};
    bool m_bBenchmarkRunning{};
    bool m_bReinitEngine{};

    // Guards
    bool m_bExecutingConsoleCommand{}; // Guard to pass to OA only those command that were issued by OA

    // Native benchmarking
    string64 m_strBenchCfgName;
    CTimer m_FrameTimer;
    xr_vector<float> m_aFrimeTimes;
};

extern xrSASH ENGINE_API g_SASH;
