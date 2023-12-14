add_library(xrEngine SHARED)

target_sources_grouped(
    TARGET xrEngine
    NAME "editor"
    FILES
    editor_base.cpp
    editor_base.h
    editor_base_input.cpp
    editor_helper.cpp
    editor_helper.h
    editor_weather.cpp
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine"
    FILES
    Properties.h
    pure.cpp
    pure.h
    std_classes.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Console"
    FILES
    XR_IOConsole.cpp
    XR_IOConsole.h
    XR_IOConsole_callback.cpp
    XR_IOConsole_control.cpp
    XR_IOConsole_get.cpp
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Console\\commands"
    FILES
    xr_ioc_cmd.cpp
    xr_ioc_cmd.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Core"
    FILES
    Engine.cpp
    Engine.h
    EngineAPI.cpp
    EngineAPI.h
    EventAPI.cpp
    EventAPI.h
    mailSlot.cpp
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Noise"
    FILES
    perlin.cpp
    perlin.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\text_editor\\edit_actions"
    FILES
    edit_actions.cpp
    edit_actions.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\text_editor\\line_editor"
    FILES
    line_editor.cpp
    line_editor.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\text_editor\\line_editor_control"
    FILES
    line_edit_control.cpp
    line_edit_control.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Animator"
    FILES
    ObjectAnimator.cpp
    ObjectAnimator.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Cameras"
    FILES
    CameraBase.cpp
    CameraBase.h
    CameraDefs.h
    CameraManager.cpp
    CameraManager.h
    Effector.cpp
    Effector.h
    EffectorPP.cpp
    EffectorPP.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Collision"
    FILES
    cf_dynamic_mesh.cpp
    cf_dynamic_mesh.h
    xr_collide_form.cpp
    xr_collide_form.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Debug"
    FILES
    ObjectDump.cpp
    ObjectDump.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Demo"
    FILES
    FDemoPlay.cpp
    FDemoPlay.h
    FDemoRecord.cpp
    FDemoRecord.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Environment"
    FILES
    Environment.cpp
    Environment.h
    Environment_misc.cpp
    Environment_render.cpp
    xrHemisphere.cpp
    xrHemisphere.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Environment\\Effects"
    FILES
    Rain.cpp
    Rain.h
    thunderbolt.cpp
    thunderbolt.h
    xr_efflensflare.cpp
    xr_efflensflare.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Feelers"
    FILES
    Feel_Sound.h
    Feel_Touch.cpp
    Feel_Touch.h
    Feel_Vision.cpp
    Feel_Vision.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\HUD"
    FILES
    CustomHUD.cpp
    CustomHUD.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Level"
    FILES
    IGame_Level.cpp
    IGame_Level.h
    IGame_Level_check_textures.cpp
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Level Controller"
    FILES
    xr_level_controller.cpp
    xr_level_controller.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Level Controller\\KeyBindings"
    FILES
    key_binding_registrator_script.cpp
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\Objects"
    FILES
    IGame_ObjectPool.cpp
    IGame_ObjectPool.h
    IGame_Persistent.cpp
    IGame_Persistent.h
    IGame_Persistent_Effects.cpp
    pure_relcase.cpp
    pure_relcase.h
    xr_object.h
    xr_object_list.cpp
    xr_object_list.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Engine\\Game API\\StringTable"
    FILES
    StringTable/StringTable.cpp
    StringTable/StringTable.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "General"
    FILES
    defines.cpp
    defines.h
    embedded_resources_management.h
    main.cpp
    main.h
    mp_logging.h
    splash.cpp
    splash.h
    stdafx.cpp
    stdafx.h
    x_ray.cpp
    x_ray.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "General\\Profiler"
    FILES
    profiler.cpp
    profiler.h
    profiler_inline.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Interfaces\\Collidable"
    FILES
    ICollidable.cpp
    ICollidable.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Interfaces\\ILoadingScreen"
    FILES
    ILoadingScreen.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Interfaces\\Input"
    FILES
    IInputReceiver.cpp
    IInputReceiver.h
    xr_input.cpp
    xr_input.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Interfaces\\Physics"
    FILES
    IObjectPhysicsCollision.h
    IPHdebug.h
    IPhysicsGeometry.h
    IPhysicsShell.h
    phdebug.cpp
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Interfaces\\Render"
    FILES
    IRenderable.cpp
    IRenderable.h
    Render.cpp
    Render.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Interfaces\\Sheduler"
    FILES
    ISheduled.cpp
    ISheduled.h
    xrSheduler.cpp
    xrSheduler.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "OpenAutomate"
    FILES
    xrSASH.cpp
    xrSASH.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Render\\Device"
    FILES
    device.cpp
    device.h
    Device_create.cpp
    Device_destroy.cpp
    Device_Initialize.cpp
    Device_mode.cpp
    Device_overdraw.cpp
    IPerformanceAlert.hpp
    PerformanceAlert.cpp
    PerformanceAlert.hpp
    StatGraph.cpp
    StatGraph.h
    Stats.cpp
    Stats.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Render\\Execution & 3D\\Shaders"
    FILES
    WaveForm.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Render\\Execution & 3D\\Shaders\\Textures"
    FILES
    #tntQAVI.cpp
    #tntQAVI.h
    xrImage_Resampler.cpp
    xrImage_Resampler.h
    xrTheora_Stream.cpp
    xrTheora_Stream.h
    xrTheora_Surface.cpp
    xrTheora_Surface.h
)

#target_sources_grouped(
#    TARGET xrEngine
#    NAME "Render\\Execution & 3D\\TextConsole"
#    FILES
#    Text_Console.cpp
#    Text_Console.h
#    Text_Console_WndProc.cpp
#)

target_sources_grouped(
    TARGET xrEngine
    NAME "Render\\Font output"
    FILES
    GameFont.cpp
    GameFont.h
    IGameFont.hpp
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Render\\Lighting"
    FILES
    LightAnimLibrary.cpp
    LightAnimLibrary.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Render\\Particle system"
    FILES
    PS_instance.cpp
    PS_instance.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Render\\Visibility"
    FILES
    ShadersExternalData.h
    vis_common.h
    vis_object_data.h
)

target_sources_grouped(
    TARGET xrEngine
    NAME "Render\\Visuals\\Skeleton"
    FILES
    EnnumerateVertices.h
)

# XXX: This is only used in utils, should be moved somewhere to utils
#target_sources_grouped(
#    TARGET xrEngine
#    NAME ""
#    FILES
#    xrLoadSurface.cpp
#)

target_include_directories(xrEngine
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/Externals/imgui"
    "${CMAKE_SOURCE_DIR}/Externals/OpenAutomate/inc"
)

target_link_libraries(xrEngine
    PUBLIC
    xrMiscMath

    PRIVATE
    xrAPI
    xrCDB
    xrCore
    xrSound
    xrScriptEngine
    xrNetServer
    xrImGui
    ${OPENAL_LIBRARY}
    ${OGG_LIBRARIES}
    Theora::Theora
)

target_compile_definitions(xrEngine
    PRIVATE
    ENGINE_BUILD
    PURE_DYNAMIC_CAST
)

set_target_properties(xrEngine PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrEngine
    PRIVATE
    stdafx.h
)

install(TARGETS xrEngine LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
