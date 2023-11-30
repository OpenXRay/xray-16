project(xrEngine)

set(EDITOR_SRC
    "editor_base.cpp"
    "editor_base.h"
    "editor_base_input.cpp"
    "editor_helper.cpp"
    "editor_helper.h"
    "editor_weather.cpp"
)

set(ENGINE_SRC
    "Properties.h"
    "pure.cpp"
    "pure.h"
    "std_classes.h"
)

set(ENGINE_CONSOLE_SRC
    "XR_IOConsole.cpp"
    "XR_IOConsole.h"
    "XR_IOConsole_callback.cpp"
    "XR_IOConsole_control.cpp"
    "XR_IOConsole_get.cpp"
)

set(ENGINE_CONSOLE_COMMANDS_SRC
    "xr_ioc_cmd.cpp"
    "xr_ioc_cmd.h"
)

set(ENGINE_CORE_SRC
    "Engine.cpp"
    "Engine.h"
    "EngineAPI.cpp"
    "EngineAPI.h"
    "EventAPI.cpp"
    "EventAPI.h"
    "mailSlot.cpp"
)

set(ENGINE_NOISE_SRC
    "perlin.cpp"
    "perlin.h"
)

set(ENGINE_TEXT_EDITOR_EDIT_ACTIONS_SRC
    "edit_actions.cpp"
    "edit_actions.h"
)

set(ENGINE_TEXT_EDITOR_LINE_EDITOR_SRC
    "line_editor.cpp"
    "line_editor.h"
)

set(ENGINE_TEXT_EDITOR_LINE_EDITOR_CONTROL_SRC
    "line_edit_control.cpp"
    "line_edit_control.h"
)

set(ENGINE_GAME_API_ANIMATOR_SRC
    "ObjectAnimator.cpp"
    "ObjectAnimator.h"
)

set(ENGINE_GAME_API_CAMERAS_SRC
    "CameraBase.cpp"
    "CameraBase.h"
    "CameraDefs.h"
    "CameraManager.cpp"
    "CameraManager.h"
    "Effector.cpp"
    "Effector.h"
    "EffectorPP.cpp"
    "EffectorPP.h"
)

set(ENGINE_GAME_API_COLLISION_SRC
    "cf_dynamic_mesh.cpp"
    "cf_dynamic_mesh.h"
    "xr_collide_form.cpp"
    "xr_collide_form.h"
)

set(ENGINE_GAME_API_DEBUG_SRC
    "ObjectDump.cpp"
    "ObjectDump.h"
)

set(ENGINE_GAME_API_DEMO_SRC
    "FDemoPlay.cpp"
    "FDemoPlay.h"
    "FDemoRecord.cpp"
    "FDemoRecord.h"
)

set(ENGINE_GAME_API_ENVIRONMENT_SRC
    "Environment.cpp"
    "Environment.h"
    "Environment_misc.cpp"
    "Environment_render.cpp"
    "xrHemisphere.cpp"
    "xrHemisphere.h"
)

set(ENGINE_GAME_API_ENVIRONMENT_EFFECTS_SRC
    "Rain.cpp"
    "Rain.h"
    "thunderbolt.cpp"
    "thunderbolt.h"
    "xr_efflensflare.cpp"
    "xr_efflensflare.h"
)

set(ENGINE_GAME_API_FEELERS_SRC
    "Feel_Sound.h"
    "Feel_Touch.cpp"
    "Feel_Touch.h"
    "Feel_Vision.cpp"
    "Feel_Vision.h"
)

set(ENGINE_GAME_API_HUD_SRC
    "CustomHUD.cpp"
    "CustomHUD.h"
)

set(ENGINE_GAME_API_LEVEL_SRC
    "IGame_Level.cpp"
    "IGame_Level.h"
    "IGame_Level_check_textures.cpp"
)

set(ENGINE_GAME_API_LEVEL_CONTROLLER_SRC
    "xr_level_controller.cpp"
    "xr_level_controller.h"
)

set(ENGINE_GAME_API_LEVEL_CONTROLLER_KEY_BINDINGS_SRC
    "key_binding_registrator_script.cpp"
)

set(ENGINE_GAME_API_MATERIALSYSTEM_SRC
    "GameMtlLib.cpp"
    "GameMtlLib.h"
    "GameMtlLib_Engine.cpp"
)

set(ENGINE_GAME_API_OBJECTS_SRC
    "IGame_ObjectPool.cpp"
    "IGame_ObjectPool.h"
    "IGame_Persistent.cpp"
    "IGame_Persistent.h"
    "IGame_Persistent_Effects.cpp"
    "pure_relcase.cpp"
    "pure_relcase.h"
    "xr_object.h"
    "xr_object_list.cpp"
    "xr_object_list.h"
)

set(ENGINE_GAME_API_STRING_TABLE_SRC
    "StringTable/StringTable.cpp"
    "StringTable/StringTable.h"
)

set(GENERAL_SRC
    "defines.cpp"
    "defines.h"
    "embedded_resources_management.h"
    "main.cpp"
    "main.h"
    "mp_logging.h"
    "splash.cpp"
    "splash.h"
    "stdafx.cpp"
    "stdafx.h"
    "x_ray.cpp"
    "x_ray.h"
)

set(GENERAL_PROFILER_SRC
    "profiler.cpp"
    "profiler.h"
    "profiler_inline.h"
)

set(INTERFACES_COLLIDABLE_SRC
    "ICollidable.cpp"
    "ICollidable.h"
)

set(INTERFACES_ILOADINGSCREEN_SRC
    "ILoadingScreen.h"
)

set(INTERFACES_INPUT_SRC
    "IInputReceiver.cpp"
    "IInputReceiver.h"
    "xr_input.cpp"
    "xr_input.h"
)

set(INTERFACES_PHYSICS_SRC
    "IObjectPhysicsCollision.h"
    "IPHdebug.h"
    "IPhysicsGeometry.h"
    "IPhysicsShell.h"
    "phdebug.cpp"
)

set(INTERFACES_RENDER_SRC
    "IRenderable.cpp"
    "IRenderable.h"
    "Render.cpp"
    "Render.h"
)

set(INTERFACES_SHEDULER_SRC
    "ISheduled.cpp"
    "ISheduled.h"
    "xrSheduler.cpp"
    "xrSheduler.h"
)

set(OPEN_AUTOMATE_SRC
    "xrSASH.cpp"
    "xrSASH.h"
)

set(RENDER_DEVICE_SRC
    "device.cpp"
    "device.h"
    "Device_create.cpp"
    "Device_destroy.cpp"
    "Device_Initialize.cpp"
    "Device_mode.cpp"
    "Device_overdraw.cpp"
    "IPerformanceAlert.hpp"
    "PerformanceAlert.cpp"
    "PerformanceAlert.hpp"
    "StatGraph.cpp"
    "StatGraph.h"
    "Stats.cpp"
    "Stats.h"
)

set(RENDER_EXECUTION_AND_3D_SHADERS_SRC
    "WaveForm.h"
)

set(RENDER_EXECUTION_AND_3D_SHADERS_TEXTURES_SRC
    #"tntQAVI.cpp"
    #"tntQAVI.h"
    "xrImage_Resampler.cpp"
    "xrImage_Resampler.h"
    "xrTheora_Stream.cpp"
    "xrTheora_Stream.h"
    "xrTheora_Surface.cpp"
    "xrTheora_Surface.h"
)

#set(RENDER_EXECUTION_AND_3D_TEXT_CONSOLE_SRC
#    "Text_Console.cpp"
#    "Text_Console.h"
#    "Text_Console_WndProc.cpp"
#)

set(RENDER_FONT_OUTPUT_SRC
    "GameFont.cpp"
    "GameFont.h"
    "IGameFont.hpp"
)

set(RENDER_LIGHTING_SRC
    "LightAnimLibrary.cpp"
    "LightAnimLibrary.h"
)

set(RENDER_PARTICLE_SYSTEM_SRC
    "PS_instance.cpp"
    "PS_instance.h"
)

set(RENDER_VISIBILITY_SRC
    "ShadersExternalData.h"
    "vis_common.h"
    "vis_object_data.h"
)

set(RENDER_VISUALS_SKELETON_SRC
    "EnnumerateVertices.h"
)

source_group("editor" FILES ${EDITOR_SRC})
source_group("Engine" FILES ${ENGINE_SRC})
source_group("Engine\\Console" FILES ${ENGINE_CONSOLE_SRC})
source_group("Engine\\Console\\commands" FILES ${ENGINE_CONSOLE_COMMANDS_SRC})
source_group("Engine\\Core" FILES ${ENGINE_CORE_SRC})
source_group("Engine\\Noise" FILES ${ENGINE_NOISE_SRC})
source_group("Engine\\text_editor\\edit_actions" FILES ${ENGINE_TEXT_EDITOR_EDIT_ACTIONS_SRC})
source_group("Engine\\text_editor\\line_editor" FILES ${ENGINE_TEXT_EDITOR_LINE_EDITOR_SRC})
source_group("Engine\\text_editor\\line_editor_control" FILES ${ENGINE_TEXT_EDITOR_LINE_EDITOR_CONTROL_SRC})
source_group("Engine\\Game API\\Animator" FILES ${ENGINE_GAME_API_ANIMATOR_SRC})
source_group("Engine\\Game API\\Cameras" FILES ${ENGINE_GAME_API_CAMERAS_SRC})
source_group("Engine\\Game API\\Collision" FILES ${ENGINE_GAME_API_COLLISION_SRC})
source_group("Engine\\Game API\\Debug" FILES ${ENGINE_GAME_API_DEBUG_SRC})
source_group("Engine\\Game API\\Demo" FILES ${ENGINE_GAME_API_DEMO_SRC})
source_group("Engine\\Game API\\Environment" FILES ${ENGINE_GAME_API_ENVIRONMENT_SRC})
source_group("Engine\\Game API\\Environment\\Effects" FILES ${ENGINE_GAME_API_ENVIRONMENT_EFFECTS_SRC})
source_group("Engine\\Game API\\Feelers" FILES ${ENGINE_GAME_API_FEELERS_SRC})
source_group("Engine\\Game API\\HUD" FILES ${ENGINE_GAME_API_HUD_SRC})
source_group("Engine\\Game API\\Level" FILES ${ENGINE_GAME_API_LEVEL_SRC})
source_group("Engine\\Game API\\Level Controller" FILES ${ENGINE_GAME_API_LEVEL_CONTROLLER_SRC})
source_group("Engine\\Game API\\Level Controller\\KeyBindings" FILES ${ENGINE_GAME_API_LEVEL_CONTROLLER_KEY_BINDINGS_SRC})
source_group("Engine\\Game API\\MaterialSystem" FILES ${ENGINE_GAME_API_MATERIALSYSTEM_SRC})
source_group("Engine\\Game API\\Objects" FILES ${ENGINE_GAME_API_OBJECTS_SRC})
source_group("Engine\\Game API\\StringTable" FILES ${ENGINE_GAME_API_STRING_TABLE_SRC})
source_group("General" FILES ${GENERAL_SRC})
source_group("General\\Profiler" FILES ${GENERAL_PROFILER_SRC})
source_group("Interfaces\\Collidable" FILES ${INTERFACES_COLLIDABLE_SRC})
source_group("Interfaces\\ILoadingScreen" FILES ${INTERFACES_ILOADINGSCREEN_SRC})
source_group("Interfaces\\Input" FILES ${INTERFACES_INPUT_SRC})
source_group("Interfaces\\Physics" FILES ${INTERFACES_PHYSICS_SRC})
source_group("Interfaces\\Render" FILES ${INTERFACES_RENDER_SRC})
source_group("Interfaces\\Sheduler" FILES ${INTERFACES_SHEDULER_SRC})
source_group("Interfaces\\Sound")
source_group("Interfaces\\SPATIAL")
source_group("OpenAutomate" FILES ${OPEN_AUTOMATE_SRC})
source_group("Render\\Device" FILES ${RENDER_DEVICE_SRC})
source_group("Render\\Execution & 3D\\Shaders" FILES ${RENDER_EXECUTION_AND_3D_SHADERS_SRC})
source_group("Render\\Execution & 3D\\Shaders\\Textures" FILES ${RENDER_EXECUTION_AND_3D_SHADERS_TEXTURES_SRC})
#source_group("Render\\Execution & 3D\\TextConsole" FILES ${RENDER_EXECUTION_AND_3D_TEXT_CONSOLE_SRC})
source_group("Render\\Font output" FILES ${RENDER_FONT_OUTPUT_SRC})
source_group("Render\\Lighting" FILES ${RENDER_LIGHTING_SRC})
source_group("Render\\Particle system" FILES ${RENDER_PARTICLE_SYSTEM_SRC})
source_group("Render\\Visibility" FILES ${RENDER_VISIBILITY_SRC})
source_group("Render\\Visuals\\Skeleton" FILES ${RENDER_VISUALS_SKELETON_SRC})

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE}
    ${EDITOR_SRC}
    ${ENGINE_SRC}
    ${ENGINE_CONSOLE_SRC}
    ${ENGINE_CONSOLE_COMMANDS_SRC}
    ${ENGINE_CORE_SRC}
    ${ENGINE_NOISE_SRC}
    ${ENGINE_TEXT_EDITOR_EDIT_ACTIONS_SRC}
    ${ENGINE_TEXT_EDITOR_LINE_EDITOR_SRC}
    ${ENGINE_TEXT_EDITOR_LINE_EDITOR_CONTROL_SRC}
    ${ENGINE_GAME_API_ANIMATOR_SRC}
    ${ENGINE_GAME_API_CAMERAS_SRC}
    ${ENGINE_GAME_API_COLLISION_SRC}
    ${ENGINE_GAME_API_DEBUG_SRC}
    ${ENGINE_GAME_API_DEMO_SRC}
    ${ENGINE_GAME_API_ENVIRONMENT_SRC}
    ${ENGINE_GAME_API_ENVIRONMENT_EFFECTS_SRC}
    ${ENGINE_GAME_API_FEELERS_SRC}
    ${ENGINE_GAME_API_HUD_SRC}
    ${ENGINE_GAME_API_LEVEL_SRC}
    ${ENGINE_GAME_API_LEVEL_CONTROLLER_SRC}
    ${ENGINE_GAME_API_LEVEL_CONTROLLER_KEY_BINDINGS_SRC}
    ${ENGINE_GAME_API_MATERIALSYSTEM_SRC}
    ${ENGINE_GAME_API_OBJECTS_SRC}
    ${ENGINE_GAME_API_STRING_TABLE_SRC}
    ${GENERAL_SRC}
    ${GENERAL_PROFILER_SRC}
    ${INTERFACES_COLLIDABLE_SRC}
    ${INTERFACES_ILOADINGSCREEN_SRC}
    ${INTERFACES_INPUT_SRC}
    ${INTERFACES_PHYSICS_SRC}
    ${INTERFACES_RENDER_SRC}
    ${INTERFACES_SHEDULER_SRC}
    ${OPEN_AUTOMATE_SRC}
    ${RENDER_DEVICE_SRC}
    ${RENDER_EXECUTION_AND_3D_SHADERS_SRC}
    ${RENDER_EXECUTION_AND_3D_SHADERS_TEXTURES_SRC}
    #${RENDER_EXECUTION_AND_3D_TEXT_CONSOLE_SRC}
    ${RENDER_FONT_OUTPUT_SRC}
    ${RENDER_LIGHTING_SRC}
    ${RENDER_PARTICLE_SYSTEM_SRC}
    ${RENDER_VISIBILITY_SRC}
    ${RENDER_VISUALS_SKELETON_SRC}
)

# XXX: This is only used in utils, should be moved somewhere to utils
#set(SRC_FILES
#    "xrLoadSurface.cpp"
#)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals/OpenAutomate/inc
    ${CMAKE_SOURCE_DIR}/Externals/luabind
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    xrMiscMath
    PRIVATE
    xrAPI
    xrCDB
    xrCore
    xrLuabind
    xrSound
    xrScriptEngine
    xrNetServer
    xrImGui
    $<$<BOOL:${STATIC_BUILD}>:xrRender_GL>
    $<$<BOOL:${STATIC_BUILD}>:xrGame>
    ${LUA_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${OGG_LIBRARIES}
    Theora::Theora
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DENGINE_BUILD
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "stdafx.h"
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
