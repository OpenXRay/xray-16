add_library(xrPhysics)
target_sources(xrPhysics PRIVATE
    ../xrServerEntities/PHNetState.cpp
    ActorCameraCollision.cpp collisiondamagereceiver.cpp console_vars.cpp
    debug_output.cpp DisablingParams.cpp ElevatorState.cpp ExtendedGeom.cpp
    Geometry.cpp GeometryBits.cpp IActivationShape.cpp MathUtils.cpp
    MovementBoxDynamicActivate.cpp params.cpp PHActivationShape.cpp
    PHActorCharacter.cpp PHAICharacter.cpp PHCapture.cpp PHCaptureInit.cpp
    PHCharacter.cpp PHCollideValidator.cpp PHCommander.cpp PHContactBodyEffector.cpp
    PHDisabling.cpp PHElement.cpp PHElementNetState.cpp
    PHFracture.cpp PHGeometryOwner.cpp PHInterpolation.cpp PHJoint.cpp
    PHJointDestroyInfo.cpp PHObject.cpp PHScriptCall.cpp PHShell.cpp
    PHShellActivate.cpp PHShellNetState.cpp PHShellSplitter.cpp PHSimpleCalls.cpp
    PHSimpleCallsScript.cpp PHSimpleCharacter.cpp PHSplitedShell.cpp
    PHStaticGeomShell.cpp phvalide.cpp PHWorld.cpp PHWorldScript.cpp Physics.cpp
    PhysicsExternalCommon.cpp PhysicsShell.cpp PhysicsShellAnimator.cpp
    PhysicsShellScript.cpp ShellHit.cpp stdafx.cpp xrPhysics.cpp)
target_include_directories(xrPhysics PUBLIC "${XRAY_SOURCE_DIR}/Externals"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${XRAY_SOURCE_DIR}/src"
    "${XRAY_SOURCE_DIR}/Externals/imgui")
target_link_libraries(xrPhysics PUBLIC xrPhysicsCore PRIVATE xrCore xrEngine
    xrMaterialSystem xrMiscMath xrScriptEngine xrAPI xrCDB xrSound)
target_compile_definitions(xrPhysics PRIVATE XRPHYSICS_EXPORTS PURE_DYNAMIC_CAST)
target_precompile_headers(xrPhysics PRIVATE StdAfx.h)
set_target_properties(xrPhysics PROPERTIES PREFIX "")
if (XRAY_GAMEPLAY_BENCHMARK)
    target_sources(xrPhysics PRIVATE GameplayBenchmark.cpp)
    target_compile_definitions(xrPhysics PRIVATE XRAY_GAMEPLAY_BENCHMARK
        XRAY_NATIVE_JOLT_PHYSICS $<$<BOOL:${XRAY_USE_JOLT_CDB}>:XRAY_BENCHMARK_JOLT_CDB>)
endif()
if (BUILD_SHARED_LIBS)
    install(TARGETS xrPhysics LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()
