#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <Common/Platform.hpp>
#include <xrCore/xrCore.h>

int main(int argc, char** argv)
{
    doctest::Context context;
    context.applyCommandLine(argc, argv);

    Memory._initialize();

    return context.run();
}
