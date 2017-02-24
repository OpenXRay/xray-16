#pragma once
#include "xrLCUtil.hpp"

class XRLCUTIL_API ILevelCompilerLogger
{
protected:
    ILevelCompilerLogger() {}
public:
    virtual void Initialize(const char* name) = 0;
    virtual void Destroy() = 0;
    virtual void clMsg(const char* format, ...) = 0;
    virtual void clMsgV(const char* format, va_list args) = 0;
    virtual void clLog(const char* format, ...) = 0;
    virtual void Status(const char* format, ...) = 0;
    virtual void StatusV(const char* format, va_list args) = 0;
    virtual void Progress(float progress) = 0;
    virtual void Phase(const char* phaseName) = 0;
    virtual void Success(const char* msg) = 0;
    virtual void Failure(const char* msg) = 0;
};
