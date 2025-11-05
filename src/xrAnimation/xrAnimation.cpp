#include "stdafx.h"
#include "xrAnimation.h"

namespace XRay {
namespace Animation {

// Development function to run animation system tests
void RunAnimationTests() {
    Msg("=== xrAnimation Test Suite ===");

    // Run ozz-animation basic tests
    //Tests::RunOzzBasicTests();

    // Future: Add more test suites here

    Msg("=== xrAnimation Test Suite Complete ===");
}

// Initialize the animation system
bool InitializeAnimationSystem() {
    Msg("* Initializing xrAnimation system...");

    // Future: Initialize global animation resources

    Msg("* xrAnimation system initialized");
    return true;
}

// Shutdown the animation system
void ShutdownAnimationSystem() {
    Msg("* Shutting down xrAnimation system...");

    // Future: Cleanup global animation resources

    Msg("* xrAnimation system shut down");
}

} // namespace Animation
} // namespace XRay
