#pragma once
#include "ILevelCompilerLogger.hpp"
#include "xrCore/Threading/Lock.hpp"

class XRLCUTIL_API LevelCompilerLoggerWindow : public ILevelCompilerLogger
{
private:
    bool initialized = false;
    HWND logWindow = 0;
    HWND hwLog = 0;
    HWND hwProgress = 0;
    HWND hwInfo = 0;
    HWND hwStage = 0;
    HWND hwTime = 0;
    HWND hwPText = 0;
    HWND hwPhaseTime = 0;
    Lock csLog
#ifdef CONFIG_PROFILE_LOCKS
        (MUTEX_PROFILE_ID(csLog))
#endif
            ;
    volatile bool close = false;
    char name[256];
    char status[1024];
    char phase[1024];
    float progress = 0.0f;
    u32 phase_start_time = 0;
    bool bStatusChange = false;
    bool bPhaseChange = false;
    u32 phase_total_time = 0;

protected:
    LevelCompilerLoggerWindow();
public:
    virtual void Initialize(const char* name) override;
    virtual void Destroy() override;
    virtual void clMsg(const char* format, ...) override;
    virtual void clMsgV(const char* format, va_list args) override;
    virtual void clLog(const char* format, ...) override;
    virtual void Status(const char* format, ...) override;
    virtual void StatusV(const char* format, va_list args) override;
    virtual void Progress(float f) override;
    virtual void Phase(const char* phaseName) override;
    virtual void Success(const char* msg) override;
    virtual void Failure(const char* msg) override;
    HWND GetWindow() const;
    static LevelCompilerLoggerWindow& instance();

private:
    static void LogThreadProc(void* context);
    void LogThreadProc();
    void ProcessMessages();
};
