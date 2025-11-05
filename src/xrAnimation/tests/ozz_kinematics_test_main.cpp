#include "Common/Platform.hpp"
#include "xrCore/xrCore.h"
#include "gtest/gtest.h"

int main(int argc, char** argv)
{
    Core.Initialize("ozz_kinematics_tests", nullptr, false, nullptr, true);
    ::testing::InitGoogleTest(&argc, argv);
    const int result = RUN_ALL_TESTS();
    Core._destroy();
    return result;
}
