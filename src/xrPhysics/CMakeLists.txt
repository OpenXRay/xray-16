if (STATIC_BUILD)
    add_library(xrPhysics STATIC)
else()
    add_library(xrPhysics SHARED)
endif()

target_sources_grouped(
    TARGET xrPhysics
    NAME "Kernel"
    FILES
    stdafx.cpp
    StdAfx.h
    xrPhysics.cpp
    xrPhysics.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics"
    FILES
    console_vars.cpp
    console_vars.h
    ShellHit.cpp
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\ActivationBox"
    FILES
    IActivationShape.cpp
    IActivationShape.h
    PHActivationShape.cpp
    PHActivationShape.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base"
    FILES
    DamageSource.h
    ode_include.h
    ode_redefine.h
    PhysicsCommon.h
    PhysicsExternalCommon.cpp
    PhysicsExternalCommon.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Debug"
    FILES
    debug_output.cpp
    debug_output.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Impact"
    FILES
    PHImpact.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects"
    FILES
    PHDefs.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\BaseShell"
    FILES
    IPhysicsShellHolder.h
    PhysicsShell.cpp
    PhysicsShell.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\BaseShell\\PhysicsShellAnimator"
    FILES
    PhysicsShellAnimatorBoneData.h
    PhysicsShellAnimator.cpp
    PhysicsShellAnimator.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\BodyEffectors"
    FILES
    PHBaseBodyEffector.h
    PHContactBodyEffector.cpp
    PHContactBodyEffector.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\Breakable"
    FILES
    PHFracture.cpp
    PHFracture.h
    PHJointDestroyInfo.cpp
    PHJointDestroyInfo.h
    PHShellSplitter.cpp
    PHShellSplitter.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\CollideValidator"
    FILES
    ICollideValidator.h
    PHCollideValidator.cpp
    PHCollideValidator.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\Disabling"
    FILES
    DisablingParams.cpp
    DisablingParams.h
    PHDisabling.cpp
    PHDisabling.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\Element"
    FILES
    PHElement.cpp
    PHElement.h
    PHElementInline.h
    PHElementNetState.cpp
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\Element\\Geom"
    FILES
    ExtendedGeom.cpp
    ExtendedGeom.h
    Geometry.cpp
    Geometry.h
    PHGeometryOwner.cpp
    PHGeometryOwner.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\Joint"
    FILES
    PHJoint.cpp
    PHJoint.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\MoveStorage"
    FILES
    PHMoveStorage.cpp
    PHMoveStorage.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\PHIsland"
    FILES
    PHIsland.cpp
    PHIsland.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\PHNet"
    FILES
    ../xrServerEntities/PHNetState.cpp
    ../xrServerEntities/PHNetState.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\PHObject"
    FILES
    PHObject.cpp
    PHObject.h
    PHUpdateObject.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\PhyMove\\ActorCharacter"
    FILES
    PHActorCharacter.cpp
    PHActorCharacter.h
    PHActorCharacterInline.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\PhyMove\\AICharacter"
    FILES
    PHAICharacter.cpp
    PHAICharacter.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\PhyMove\\BaseCharacter"
    FILES
    IColisiondamageInfo.h
    MovementBoxDynamicActivate.cpp
    MovementBoxDynamicActivate.h
    PHCharacter.cpp
    PHCharacter.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\PhyMove\\Capture"
    FILES
    IPHCapture.h
    PHCapture.cpp
    PHCapture.h
    PHCaptureInit.cpp
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\PhyMove\\SimpleCharacter"
    FILES
    ElevatorState.cpp
    ElevatorState.h
    IClimableObject.h
    IElevatorState.h
    PHSimpleCharacter.cpp
    PHSimpleCharacter.h
    PHSimpleCharacterInline.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\script"
    FILES
    iphysics_scripted.h
    physics_scripted.cpp
    physics_scripted.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\Shell"
    FILES
    PHShell.cpp
    PHShell.h
    PHShellActivate.cpp
    PHShellBuildJoint.h
    PHShellNetState.cpp
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\SplitedShell"
    FILES
    PHSplitedShell.cpp
    PHSplitedShell.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\StaticShell"
    FILES
    IPHStaticGeomShell.h
    PHStaticGeomShell.cpp
    PHStaticGeomShell.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\utils"
    FILES
    ph_valid_ode.h
    phvalide.cpp
    phvalide.h
    PHValideValues.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\utils\\Interpolation"
    FILES
    PHInterpolation.cpp
    PHInterpolation.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\utils\\Math"
    FILES
    CalculateTriangle.h
    MathUtils.cpp
    MathUtils.h
    MathUtilsOde.h
    matrix_utils.h
    PHDynamicData.cpp
    PHDynamicData.h
    SpaceUtils.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\utils\\Storage"
    FILES
    BlockAllocator.h
    CycleConstStorage.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Base\\Objects\\World"
    FILES
    GeometryBits.cpp
    GeometryBits.h
    IPHWorld.h
    params.cpp
    params.h
    PHItemList.h
    PHWorld.cpp
    PHWorld.h
    Physics.cpp
    Physics.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Camera collision"
    FILES
    ActorCameraCollision.cpp
    ActorCameraCollision.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\colliders\\cyl"
    FILES
    dcylinder/dCylinder.cpp
    dcylinder/dCylinder.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\colliders\\RayMotions"
    FILES
    dRayMotions.cpp
    dRayMotions.h
)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\colliders\\tri"
    FILES
    tri-colliderknoopc/__aabb_tri.h
    tri-colliderknoopc/dcTriangle.h
    #tri-colliderknoopc/dcTriListCollider.cpp
    #tri-colliderknoopc/dcTriListCollider.h
    tri-colliderknoopc/dSortTriPrimitive.cpp
    tri-colliderknoopc/dSortTriPrimitive.h
    tri-colliderknoopc/dTriBox.cpp
    tri-colliderknoopc/dTriBox.h
    tri-colliderknoopc/dTriCallideK.cpp
    tri-colliderknoopc/dTriCollideK.h
    tri-colliderknoopc/dTriColliderCommon.h
    tri-colliderknoopc/dTriColliderMath.h
    tri-colliderknoopc/dTriCylinder.cpp
    tri-colliderknoopc/dTriCylinder.h
    tri-colliderknoopc/dTriList.cpp
    tri-colliderknoopc/dTriList.h
    tri-colliderknoopc/dTriSphere.cpp
    tri-colliderknoopc/dTriSphere.h
    tri-colliderknoopc/dxTriList.h
    tri-colliderknoopc/TriPrimitiveCollideClassDef.h)

target_sources_grouped(
    TARGET xrPhysics
    NAME "Physics\\Damage receiver"
    FILES
    collisiondamagereceiver.cpp
    icollisiondamagereceiver.h
)


target_include_directories(xrPhysics
    PUBLIC
    "${CMAKE_SOURCE_DIR}/Externals"

    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/Externals/imgui"
    "${CMAKE_SOURCE_DIR}/Externals/ode/include"
)

target_link_libraries(xrPhysics
    PUBLIC
    xrODE
    PRIVATE
    xrCore
    xrEngine
    xrMiscMath
    xrScriptEngine
    xrAPI
    xrCDB
    xrSound
)

target_compile_definitions(xrPhysics
    PRIVATE
    XRPHYSICS_EXPORTS
)

set_target_properties(xrPhysics PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)

target_precompile_headers(xrPhysics
    PRIVATE
    StdAfx.h
)

if (NOT STATIC_BUILD)
    install(TARGETS xrPhysics LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
