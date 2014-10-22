#pragma once

using namespace System;

namespace XRay
{
namespace ManagedApi
{
namespace Core
{

public ref class Core
{
public:
    delegate void LogCallback(String^ str);
internal:
    static LogCallback^ ManagedLogCallback;
private:
    Core();
public:
    static void Initialize(String^ appName, LogCallback^ logCallback, bool initFs, String^ fsFileName);
    static void Initialize(String^ appName, LogCallback^ logCallback, bool initFs);
    static void Initialize(String^ appName, LogCallback^ logCallback);
    static void Initialize(String^ appName);
    static void Destroy();
};

}
}
}
