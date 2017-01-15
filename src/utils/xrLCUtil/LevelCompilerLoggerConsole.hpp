#pragma once
#include "ILevelCompilerLogger.hpp"

class XRLCUTIL_API LevelCompilerLoggerConsole : public ILevelCompilerLogger
{
public:
    LevelCompilerLoggerConsole();
    virtual void Initialize(const char* name) override;
    virtual void Destroy() override;
    virtual void clMsg(const char* format, ...) override;
    virtual void clMsgV(const char* format, va_list args) override;
    virtual void clLog(const char* format, ...) override;
    virtual void Status(const char* format, ...) override;
    virtual void StatusV(const char* format, va_list args) override;
    virtual void Progress(float progress) override;
    virtual void Phase(const char* phaseName) override;
    virtual void Success(const char* msg) override;
    virtual void Failure(const char* msg) override;
};
