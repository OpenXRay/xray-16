#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "BackendFixture.h"
#include <cstdio>
#include <exception>

int main()
{
    Core.Initialize("xrPhysicsTests", "", false);
    int result = 0;
    try
    {
        RunPhysicsBackendTests();
        std::puts("Physics backend checks passed");
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        result = 1;
    }
    Core._destroy();
    return result;
}
