#pragma once
#ifdef XRPHYSICS_EXPORTS
#define PHYSICS_TEST_API XR_EXPORT
#else
#define PHYSICS_TEST_API XR_IMPORT
#endif
PHYSICS_TEST_API void RunPhysicsBackendTests();
