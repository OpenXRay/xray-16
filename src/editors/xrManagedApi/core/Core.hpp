#pragma once

using namespace System;

namespace XRay
{
namespace ManagedApi
{
namespace Core
{
public
delegate void LogCallback(String ^ str);

public
ref class Core abstract sealed
{
    internal : static LogCallback ^ ManagedLogCallback;

public:
    static void Initialize(String ^ appName, LogCallback ^ logCallback, bool initFs, String ^ fsFileName);
    static void Initialize(String ^ appName, LogCallback ^ logCallback, bool initFs);
    static void Initialize(String ^ appName, LogCallback ^ logCallback);
    static void Initialize(String ^ appName);
    static void Destroy();

    static const String ^ GetBuildDate();
    static const UInt32 GetBuildId();
};
}
}
}
