#pragma once
#include "xrLCUtil.hpp"

class XRLCUTIL_API CThread
{
public:
    using LogFunc = void (*)(const char* format, ...);

private:
    volatile LogFunc log;
    static void startup(void* P);
    static void StubLog(const char*, ...);

public:
    volatile u32 thID;
    volatile float thProgress;
    volatile BOOL thCompleted;
    volatile BOOL thMessages;
    volatile BOOL thMonitor;
    volatile float thPerformance;
    volatile BOOL thDestroyOnComplete;

    CThread(u32 _ID, LogFunc log)
    {
        this->log = log ? log : StubLog;
        thID = _ID;
        thProgress = 0;
        thCompleted = FALSE;
        thMessages = TRUE;
        thMonitor = FALSE;
        thDestroyOnComplete = TRUE;
    }
    virtual ~CThread() {}
    void Start() { thread_spawn(startup, "worker-thread", 1024 * 1024, this); }
    virtual void Execute() = 0;
};

class XRLCUTIL_API CThreadManager
{
public:
    using ReportStatusFunc = void (*)(const char* format, ...);
    using ReportProgressFunc = void (*)(float f);
    void start(CThread* T);
    void wait(u32 sleep_time = 1000);

private:
    static void StubReportStatus(const char*, ...);
    static void StubReportProgress(float);
    xr_vector<CThread*> threads;
    ReportStatusFunc reportStatus;
    ReportProgressFunc reportProgress;

public:
    CThreadManager(ReportStatusFunc reportStatus, ReportProgressFunc reportProgress)
    {
        this->reportStatus = reportStatus ? reportStatus : StubReportStatus;
        this->reportProgress = reportProgress ? reportProgress : StubReportProgress;
    }
};

IC void get_intervals(u32 max_threads, u32 num_items, u32& threads, u32& stride, u32& rest)
{
    if (max_threads <= num_items)
    {
        threads = max_threads;
        stride = num_items / max_threads;
        rest = num_items % max_threads;
        return;
    }
    threads = num_items;
    stride = 1;
    rest = 0;
}
