#pragma once

namespace XRay
{
class XRCORE_API Module
{
    void* handle;

public:
    Module();
    Module(pcstr moduleName, bool log = true);
    ~Module();

    void* open(pcstr moduleName, bool log = true);
    void close();

    bool exist() const;

    void* operator()() const;

    void* getProcAddress(pcstr procName) const;
};
}
