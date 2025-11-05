#pragma once

namespace XRay {
namespace Animation {

// Initialize the animation system
bool InitializeAnimationSystem();

// Shutdown the animation system  
void ShutdownAnimationSystem();

// Run animation system tests (development only)
void RunAnimationTests();

} // namespace Animation
} // namespace XRay