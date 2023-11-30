project(xrPhysics)

set(KERNEL
    "stdafx.cpp"
    "StdAfx.h"
    "xrPhysics.cpp"
    "xrPhysics.h"
)

set(PHYSICS
    "console_vars.cpp"
    "console_vars.h"
    "ShellHit.cpp"
)

set(PHYSICS_ACTIVATION_BOX
    "IActivationShape.cpp"
    "IActivationShape.h"
    "PHActivationShape.cpp"
    "PHActivationShape.h"
)

set(PHYSICS_BASE
    "DamageSource.h"
    "ode_include.h"
    "ode_redefine.h"
    "PhysicsCommon.h"
    "PhysicsExternalCommon.cpp"
    "PhysicsExternalCommon.h"
)

set(PHYSICS_BASE_DEBUG
    "debug_output.cpp"
    "debug_output.h"
)

set(PHYSICS_BASE_IMPACT
    "PHImpact.h"
)

set(PHYSICS_BASE_OBJECTS
    "PHDefs.h"
)

set(PHYSICS_BASE_OBJECTS_BASE_SHELL
    "IPhysicsShellHolder.h"
    "PhysicsShell.cpp"
    "PhysicsShell.h"
)

set(PHYSICS_BASE_OBJECTS_BASE_SHELL_ANIMATOR
    "PhysicsShellAnimatorBoneData.h"
    "PhysicsShellAnimator.cpp"
    "PhysicsShellAnimator.h"
)

set(PHYSICS_BASE_OBJECTS_BODY_EFFECTORS
    "PHBaseBodyEffector.h"
    "PHContactBodyEffector.cpp"
    "PHContactBodyEffector.h"
)

set(PHYSICS_BASE_OBJECTS_BREAKABLE
    "PHFracture.cpp"
    "PHFracture.h"
    "PHJointDestroyInfo.cpp"
    "PHJointDestroyInfo.h"
    "PHShellSplitter.cpp"
    "PHShellSplitter.h"
)

set(PHYSICS_BASE_OBJECTS_COLLIDE_VALIDATOR
    "ICollideValidator.h"
    "PHCollideValidator.cpp"
    "PHCollideValidator.h"
)

set(PHYSICS_BASE_OBJECTS_DISABLING
    "DisablingParams.cpp"
    "DisablingParams.h"
    "PHDisabling.cpp"
    "PHDisabling.h"
)

set(PHYSICS_BASE_OBJECTS_ELEMENT
    "PHElement.cpp"
    "PHElement.h"
    "PHElementInline.h"
    "PHElementNetState.cpp"
)

set(PHYSICS_BASE_OBJECTS_ELEMENT_GEOM
    "ExtendedGeom.cpp"
    "ExtendedGeom.h"
    "Geometry.cpp"
    "Geometry.h"
    "PHGeometryOwner.cpp"
    "PHGeometryOwner.h"
)

set(PHYSICS_BASE_OBJECTS_JOINT
    "PHJoint.cpp"
    "PHJoint.h"
)

set(PHYSICS_BASE_OBJECTS_MOVE_STORAGE
    "PHMoveStorage.cpp"
    "PHMoveStorage.h"
)

set(PHYSICS_BASE_OBJECTS_PH_ISLAND
    "PHIsland.cpp"
    "PHIsland.h"
)

set(PHYSICS_BASE_OBJECTS_PH_NET
    "../xrServerEntities/PHNetState.cpp"
    "../xrServerEntities/PHNetState.h"
)

set(PHYSICS_BASE_OBJECTS_PH_OBJECT
    "PHObject.cpp"
    "PHObject.h"
    "PHUpdateObject.h"
)

set(PHYSICS_BASE_OBJECTS_PHY_MOVE_ACTOR_CHARACTER
    "PHActorCharacter.cpp"
    "PHActorCharacter.h"
    "PHActorCharacterInline.h"
)

set(PHYSICS_BASE_OBJECTS_PHY_MOVE_AI_CHARACTER
    "PHAICharacter.cpp"
    "PHAICharacter.h"
)

set(PHYSICS_BASE_OBJECTS_PHY_MOVE_BASE_CHARACTER
    "IColisiondamageInfo.h"
    "MovementBoxDynamicActivate.cpp"
    "MovementBoxDynamicActivate.h"
    "PHCharacter.cpp"
    "PHCharacter.h"
)

set(PHYSICS_BASE_OBJECTS_PHY_MOVE_CAPTURE
    "IPHCapture.h"
    "PHCapture.cpp"
    "PHCapture.h"
    "PHCaptureInit.cpp"
)

set(PHYSICS_BASE_OBJECTS_PHY_MOVE_SIMPLE_CHARACTER
    "ElevatorState.cpp"
    "ElevatorState.h"
    "IClimableObject.h"
    "IElevatorState.h"
    "PHSimpleCharacter.cpp"
    "PHSimpleCharacter.h"
    "PHSimpleCharacterInline.h"
)

set(PHYSICS_BASE_OBJECTS_SCRIPT
    "iphysics_scripted.h"
    "physics_scripted.cpp"
    "physics_scripted.h"
)

set(PHYSICS_BASE_OBJECTS_SHELL
    "PHShell.cpp"
    "PHShell.h"
    "PHShellActivate.cpp"
    "PHShellBuildJoint.h"
    "PHShellNetState.cpp"
)

set(PHYSICS_BASE_OBJECTS_SPLITED_SHELL
    "PHSplitedShell.cpp"
    "PHSplitedShell.h"
)

set(PHYSICS_BASE_OBJECTS_STATIC_SHELL
    "IPHStaticGeomShell.h"
    "PHStaticGeomShell.cpp"
    "PHStaticGeomShell.h"
)

set(PHYSICS_BASE_OBJECTS_UTILS
    "ph_valid_ode.h"
    "phvalide.cpp"
    "phvalide.h"
    "PHValideValues.h"
)
set(PHYSICS_BASE_OBJECTS_UTILS_INTERPOLATION
    "PHInterpolation.cpp"
    "PHInterpolation.h"
)

set(PHYSICS_BASE_OBJECTS_UTILS_MATH
    "CalculateTriangle.h"
    "MathUtils.cpp"
    "MathUtils.h"
    "MathUtilsOde.h"
    "matrix_utils.h"
    "PHDynamicData.cpp"
    "PHDynamicData.h"
    "SpaceUtils.h"
)

set(PHYSICS_BASE_OBJECTS_UTILS_STORAGE
    "BlockAllocator.h"
    "CycleConstStorage.h"
)

set(PHYSICS_BASE_OBJECTS_WORLD
    "GeometryBits.cpp"
    "GeometryBits.h"
    "IPHWorld.h"
    "params.cpp"
    "params.h"
    "PHItemList.h"
    "PHWorld.cpp"
    "PHWorld.h"
    "Physics.cpp"
    "Physics.h"
)

set(CAMERA_COLLISION
    "ActorCameraCollision.cpp"
    "ActorCameraCollision.h"
)

set(COLLIDERS_CYL
    "dcylinder/dCylinder.cpp"
    "dcylinder/dCylinder.h"
)

set(COLLIDERS_RAY_MOTIONS
    "dRayMotions.cpp"
    "dRayMotions.h"
)

set(COLLIDERS_TRI
    "tri-colliderknoopc/__aabb_tri.h"
    "tri-colliderknoopc/dcTriangle.h"
    #"tri-colliderknoopc/dcTriListCollider.cpp"
    #"tri-colliderknoopc/dcTriListCollider.h"
    "tri-colliderknoopc/dSortTriPrimitive.cpp"
    "tri-colliderknoopc/dSortTriPrimitive.h"
    "tri-colliderknoopc/dTriBox.cpp"
    "tri-colliderknoopc/dTriBox.h"
    "tri-colliderknoopc/dTriCallideK.cpp"
    "tri-colliderknoopc/dTriCollideK.h"
    "tri-colliderknoopc/dTriColliderCommon.h"
    "tri-colliderknoopc/dTriColliderMath.h"
    "tri-colliderknoopc/dTriCylinder.cpp"
    "tri-colliderknoopc/dTriCylinder.h"
    "tri-colliderknoopc/dTriList.cpp"
    "tri-colliderknoopc/dTriList.h"
    "tri-colliderknoopc/dTriSphere.cpp"
    "tri-colliderknoopc/dTriSphere.h"
    "tri-colliderknoopc/dxTriList.h"
    "tri-colliderknoopc/TriPrimitiveCollideClassDef.h")

set(DAMAGE_RECEIVER
    "collisiondamagereceiver.cpp"
    "icollisiondamagereceiver.h"
)

source_group("kernel" FILES ${KERNEL})
source_group("physics" FILES ${PHYSICS})
source_group("physics\\ActivationBox" FILES ${PHYSICS_ACTIVATION_BOX})
source_group("physics\\Base" FILES ${PHYSICS_BASE})
source_group("physics\\Base\\Debug" FILES ${PHYSICS_BASE_DEBUG})
source_group("physics\\Base\\Impact" FILES ${PHYSICS_BASE_IMPACT})
source_group("physics\\Base\\Objects" FILES ${PHYSICS_BASE_OBJECTS})
source_group("physics\\Base\\Objects\\BaseShell" FILES ${PHYSICS_BASE_OBJECTS_BASE_SHELL})
source_group("physics\\Base\\Objects\\BaseShell\\PhysicsShellAnimator" FILES ${PHYSICS_BASE_OBJECTS_BASE_SHELL_ANIMATOR})
source_group("physics\\Base\\Objects\\BodyEffectors" FILES ${PHYSICS_BASE_OBJECTS_BODY_EFFECTORS})
source_group("physics\\Base\\Objects\\Breakable" FILES ${PHYSICS_BASE_OBJECTS_BREAKABLE})
source_group("physics\\Base\\Objects\\CollideValidator" FILES ${PHYSICS_BASE_OBJECTS_COLLIDE_VALIDATOR})
source_group("physics\\Base\\Objects\\Disabling" FILES ${PHYSICS_BASE_OBJECTS_DISABLING})
source_group("physics\\Base\\Objects\\Element" FILES ${PHYSICS_BASE_OBJECTS_ELEMENT})
source_group("physics\\Base\\Objects\\Element\\Geom" FILES ${PHYSICS_BASE_OBJECTS_ELEMENT_GEOM})
source_group("physics\\Base\\Objects\\Joint" FILES ${PHYSICS_BASE_OBJECTS_JOINT})
source_group("physics\\Base\\Objects\\MoveStorage" FILES ${PHYSICS_BASE_OBJECTS_MOVE_STORAGE})
source_group("physics\\Base\\Objects\\PHIsland" FILES ${PHYSICS_BASE_OBJECTS_PH_ISLAND})
source_group("physics\\Base\\Objects\\PHNet" FILES ${PHYSICS_BASE_OBJECTS_PH_NET})
source_group("physics\\Base\\Objects\\PHObject" FILES ${PHYSICS_BASE_OBJECTS_PH_OBJECT})
source_group("physics\\Base\\Objects\\PhyMove\\ActorCharacter" FILES ${PHYSICS_BASE_OBJECTS_PHY_MOVE_ACTOR_CHARACTER})
source_group("physics\\Base\\Objects\\PhyMove\\AICharacter" FILES ${PHYSICS_BASE_OBJECTS_PHY_MOVE_AI_CHARACTER})
source_group("physics\\Base\\Objects\\PhyMove\\BaseCharacter" FILES ${PHYSICS_BASE_OBJECTS_PHY_MOVE_BASE_CHARACTER})
source_group("physics\\Base\\Objects\\PhyMove\\Capture" FILES ${PHYSICS_BASE_OBJECTS_PHY_MOVE_CAPTURE})
source_group("physics\\Base\\Objects\\PhyMove\\SimpleCharacter" FILES ${PHYSICS_BASE_OBJECTS_PHY_MOVE_SIMPLE_CHARACTER})
source_group("physics\\Base\\Objects\\script" FILES ${PHYSICS_BASE_OBJECTS_SCRIPT})
source_group("physics\\Base\\Objects\\Shell" FILES ${PHYSICS_BASE_OBJECTS_SHELL})
source_group("physics\\Base\\Objects\\SplitedShell" FILES ${PHYSICS_BASE_OBJECTS_SPLITED_SHELL})
source_group("physics\\Base\\Objects\\StaticShell" FILES ${PHYSICS_BASE_OBJECTS_STATIC_SHELL})
source_group("physics\\Base\\Objects\\utils" FILES ${PHYSICS_BASE_OBJECTS_UTILS})
source_group("physics\\Base\\Objects\\utils\\Interpolation" FILES ${PHYSICS_BASE_OBJECTS_UTILS_INTERPOLATION})
source_group("physics\\Base\\Objects\\utils\\Math" FILES ${PHYSICS_BASE_OBJECTS_UTILS_MATH})
source_group("physics\\Base\\Objects\\utils\\Storage" FILES ${PHYSICS_BASE_OBJECTS_UTILS_STORAGE})
source_group("physics\\Base\\Objects\\World" FILES ${PHYSICS_BASE_OBJECTS_WORLD})
source_group("physics\\Camera collision" FILES ${CAMERA_COLLISION})
source_group("physics\\colliders\\cyl" FILES ${COLLIDERS_CYL})
source_group("physics\\colliders\\RayMotions" FILES ${COLLIDERS_RAY_MOTIONS})
source_group("physics\\colliders\\tri" FILES ${COLLIDERS_TRI})
source_group("physics\\Damage receiver" FILES ${DAMAGE_RECEIVER})

set(SRC_FILES
    ${KERNEL}
    ${PHYSICS}
    ${PHYSICS_ACTIVATION_BOX}
    ${PHYSICS_BASE}
    ${PHYSICS_BASE_DEBUG}
    ${PHYSICS_BASE_IMPACT}
    ${PHYSICS_BASE_OBJECTS}
    ${PHYSICS_BASE_OBJECTS_BASE_SHELL}
    ${PHYSICS_BASE_OBJECTS_BASE_SHELL_ANIMATOR}
    ${PHYSICS_BASE_OBJECTS_BODY_EFFECTORS}
    ${PHYSICS_BASE_OBJECTS_BREAKABLE}
    ${PHYSICS_BASE_OBJECTS_COLLIDE_VALIDATOR}
    ${PHYSICS_BASE_OBJECTS_DISABLING}
    ${PHYSICS_BASE_OBJECTS_ELEMENT}
    ${PHYSICS_BASE_OBJECTS_ELEMENT_GEOM}
    ${PHYSICS_BASE_OBJECTS_JOINT}
    ${PHYSICS_BASE_OBJECTS_MOVE_STORAGE}
    ${PHYSICS_BASE_OBJECTS_PH_ISLAND}
    ${PHYSICS_BASE_OBJECTS_PH_NET}
    ${PHYSICS_BASE_OBJECTS_PH_OBJECT}
    ${PHYSICS_BASE_OBJECTS_PHY_MOVE_ACTOR_CHARACTER}
    ${PHYSICS_BASE_OBJECTS_PHY_MOVE_AI_CHARACTER}
    ${PHYSICS_BASE_OBJECTS_PHY_MOVE_BASE_CHARACTER}
    ${PHYSICS_BASE_OBJECTS_PHY_MOVE_CAPTURE}
    ${PHYSICS_BASE_OBJECTS_PHY_MOVE_SIMPLE_CHARACTER}
    ${PHYSICS_BASE_OBJECTS_SCRIPT}
    ${PHYSICS_BASE_OBJECTS_SHELL}
    ${PHYSICS_BASE_OBJECTS_SPLITED_SHELL}
    ${PHYSICS_BASE_OBJECTS_STATIC_SHELL}
    ${PHYSICS_BASE_OBJECTS_UTILS}
    ${PHYSICS_BASE_OBJECTS_UTILS_INTERPOLATION}
    ${PHYSICS_BASE_OBJECTS_UTILS_MATH}
    ${PHYSICS_BASE_OBJECTS_UTILS_STORAGE}
    ${PHYSICS_BASE_OBJECTS_WORLD}
    ${CAMERA_COLLISION}
    ${COLLIDERS_CYL}
    ${COLLIDERS_RAY_MOTIONS}
    ${COLLIDERS_TRI}
    ${DAMAGE_RECEIVER}
)

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE} ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/ode/include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
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

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRPHYSICS_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "StdAfx.h"
)

if (NOT STATIC_BUILD)
    install(TARGETS ${PROJECT_NAME} LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
endif()
