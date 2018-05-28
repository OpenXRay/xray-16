#pragma once
#include <memory>

namespace XRay
{
class XRCORE_API ModuleHandle
{
    void* handle;
    bool dontUnload;

public:
    ModuleHandle(const bool dontUnload = false);
    ModuleHandle(pcstr moduleName, bool dontUnload = false);
    ~ModuleHandle();

    void* Open(pcstr moduleName);
    void Close();

    bool IsLoaded() const;

    void* operator()() const;

    void* GetProcAddress(pcstr procName) const;
};

using Module = std::unique_ptr<ModuleHandle>;

inline auto LoadModule(bool dontUnload = false)
{
    return std::make_unique<ModuleHandle>(dontUnload);
}

inline auto LoadModule(pcstr moduleName, bool dontUnload = false)
{
    return std::make_unique<ModuleHandle>(moduleName, dontUnload);
}
}
