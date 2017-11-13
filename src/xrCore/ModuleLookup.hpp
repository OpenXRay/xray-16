#pragma once

namespace XRay
{
class XRCORE_API Module
{
    void* handle;
    bool dontUnload;

public:
    Module(const bool dontUnload = false);
    Module(pcstr moduleName, bool dontUnload = false);
    ~Module();

    void* open(pcstr moduleName);
    void close();

    bool exist() const;

    void* operator()() const;

    void* getProcAddress(pcstr procName) const;
};
}
