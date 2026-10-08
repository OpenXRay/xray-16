add_compile_definitions(
    $<$<CONFIG:ReleaseMasterGold>:MASTER_GOLD>
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:XRAY_STATIC_BUILD>
    $<$<CONFIG:ReleaseMasterGold>:XRAY_EXCEPTIONS=0>
    $<$<NOT:$<CONFIG:ReleaseMasterGold>>:XRAY_EXCEPTIONS=1>
)

add_compile_options(${XRAY_ENABLE_WARNINGS})

# Common engine headers validate that the luabind submodule is present. Make
# its public include root available to every engine target that includes them.
include_directories("${XRAY_SOURCE_DIR}/Externals/luabind")
include_directories("${XRAY_SOURCE_DIR}/Externals")

add_subdirectory(utils)
add_subdirectory(Layers)
add_subdirectory(xr_3da)
add_subdirectory(xrAICore)
add_subdirectory(xrUICore)
add_subdirectory(xrCDB)
add_subdirectory(xrCore)
add_subdirectory(xrEngine)
add_subdirectory(xrGame)
add_subdirectory(xrGameSpy)
add_subdirectory(xrMaterialSystem)
add_subdirectory(xrNetServer)
add_subdirectory(xrParticles)
add_subdirectory(xrPhysics)
add_subdirectory(xrScriptEngine)
add_subdirectory(xrSound)

if (XRAY_BUILD_VULKAN_TESTS)
    add_executable(android_renderer_choice_test "${XRAY_SOURCE_DIR}/tests/android_renderer_choice.cpp")
    target_include_directories(android_renderer_choice_test PRIVATE "${XRAY_SOURCE_DIR}")
    target_compile_options(android_renderer_choice_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    add_test(NAME android_renderer_choice COMMAND android_renderer_choice_test)
    function(xray_add_vulkan_state_test target source)
        add_executable(${target} "${XRAY_SOURCE_DIR}/${source}")
        target_include_directories(${target} PRIVATE "${XRAY_SOURCE_DIR}" "${XRAY_SOURCE_DIR}/src")
        target_compile_definitions(${target} PRIVATE DEBUG)
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
        )
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    xray_add_vulkan_state_test(vulkan_device_resource_state_test tests/vulkan_device_resource_state.cpp)
    xray_add_vulkan_state_test(vulkan_frame_phase_test tests/vulkan_frame_phase.cpp)
    xray_add_vulkan_state_test(vulkan_camera_state_test tests/vulkan_camera_state.cpp)
    xray_add_vulkan_state_test(vulkan_slide_windows_test tests/vulkan_slide_windows.cpp)
    xray_add_vulkan_state_test(vulkan_skeleton_bones_test tests/vulkan_skeleton_bones.cpp)
    xray_add_vulkan_state_test(vulkan_skeleton_motions_test tests/vulkan_skeleton_motions.cpp)
    xray_add_vulkan_state_test(vulkan_motion_playback_test tests/vulkan_motion_playback.cpp)
    xray_add_vulkan_state_test(vulkan_particle_catalog_test tests/vulkan_particle_catalog.cpp)
    xray_add_vulkan_state_test(vulkan_wallmark_geometry_test tests/vulkan_wallmark_geometry.cpp)
    xray_add_vulkan_state_test(vulkan_screen_copy_pass_test tests/vulkan_screen_copy_pass.cpp)
    target_sources(vulkan_screen_copy_pass_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/ScreenCopyPass.cpp")
    target_include_directories(vulkan_screen_copy_pass_test PRIVATE
        "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include")
    xray_add_vulkan_state_test(vulkan_frame_lifecycle_test tests/vulkan_frame_lifecycle.cpp)
    target_sources(vulkan_frame_lifecycle_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/FrameContext.cpp")
    target_include_directories(vulkan_frame_lifecycle_test PRIVATE
        "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include")
    xray_add_vulkan_state_test(vulkan_buffer_upload_test tests/vulkan_buffer_upload.cpp)
    target_sources(vulkan_buffer_upload_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/BufferUpload.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/BufferResource.cpp")
    target_include_directories(vulkan_buffer_upload_test PRIVATE
        "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include")
    xray_add_vulkan_state_test(vulkan_game_capabilities_test tests/vulkan_game_capabilities.cpp)
    target_sources(vulkan_game_capabilities_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/VulkanHardware.cpp")
    target_include_directories(vulkan_game_capabilities_test PRIVATE
        "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include")
    xray_add_vulkan_state_test(vulkan_surface_lifecycle_test tests/vulkan_surface_lifecycle.cpp)
    xray_add_vulkan_state_test(vulkan_frame_metrics_test tests/vulkan_frame_metrics.cpp)
    target_sources(vulkan_wallmark_geometry_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/WallmarkGeometry.cpp")
    target_sources(vulkan_particle_catalog_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/ParticleCatalog.cpp")
    target_sources(vulkan_motion_playback_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/MotionPlayback.cpp")
    target_sources(vulkan_skeleton_motions_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/SkeletonMotions.cpp")
    target_sources(vulkan_skeleton_bones_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/SkeletonBones.cpp")
    target_include_directories(vulkan_skeleton_bones_test PRIVATE
        "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include")
    target_sources(vulkan_slide_windows_test PRIVATE
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/SlidingWindows.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/ModelGeometry.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/LevelModels.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/SpecialVisuals.cpp")

    add_executable(vulkan_deferred_pass_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_deferred_pass.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/DeferredPass.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/GameShaderResources.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/ShaderModule.cpp"
    )
    target_include_directories(vulkan_deferred_pass_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include"
    )
    target_compile_definitions(vulkan_deferred_pass_test PRIVATE DEBUG)
    target_compile_options(vulkan_deferred_pass_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    add_test(NAME vulkan_deferred_pass COMMAND vulkan_deferred_pass_test)

    add_executable(vulkan_level_visibility_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_level_visibility.cpp"
    )
    target_include_directories(vulkan_level_visibility_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(vulkan_level_visibility_test PRIVATE DEBUG)
    target_compile_options(vulkan_level_visibility_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    target_link_libraries(vulkan_level_visibility_test PRIVATE xrRenderVK xrCDB xrCore)
    add_test(NAME vulkan_level_visibility COMMAND vulkan_level_visibility_test)

    add_executable(vulkan_level_models_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_level_models.cpp"
    )
    target_include_directories(vulkan_level_models_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(vulkan_level_models_test PRIVATE DEBUG)
    target_compile_options(vulkan_level_models_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    target_link_libraries(vulkan_level_models_test PRIVATE xrRenderVK xrCDB xrCore)
    add_test(NAME vulkan_level_models COMMAND vulkan_level_models_test)

    add_executable(vulkan_shader_material_library_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_shader_material_library.cpp")
    target_include_directories(vulkan_shader_material_library_test PRIVATE
        "${XRAY_SOURCE_DIR}" "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>)
    target_compile_options(vulkan_shader_material_library_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    target_link_libraries(vulkan_shader_material_library_test PRIVATE xrRenderVK xrEngine xrScriptEngine xrCDB xrCore)
    add_test(NAME vulkan_shader_material_library COMMAND vulkan_shader_material_library_test)

    add_executable(vulkan_detail_assets_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_detail_assets.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/DetailAssets.cpp")
    target_include_directories(vulkan_detail_assets_test PRIVATE "${XRAY_SOURCE_DIR}")
    target_compile_options(vulkan_detail_assets_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    add_test(NAME vulkan_detail_assets COMMAND vulkan_detail_assets_test)

    add_executable(vulkan_fog_parameters_test "${XRAY_SOURCE_DIR}/tests/vulkan_fog_parameters.cpp")
    target_include_directories(vulkan_fog_parameters_test PRIVATE "${XRAY_SOURCE_DIR}")
    target_compile_options(vulkan_fog_parameters_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    add_test(NAME vulkan_fog_parameters COMMAND vulkan_fog_parameters_test)

    add_executable(vulkan_local_shadow_targets_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_local_shadow_targets.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/LocalShadowTargets.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/BufferResource.cpp")
    target_include_directories(vulkan_local_shadow_targets_test PRIVATE
        "${XRAY_SOURCE_DIR}" "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include")
    target_compile_options(vulkan_local_shadow_targets_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    add_test(NAME vulkan_local_shadow_targets COMMAND vulkan_local_shadow_targets_test)

    add_executable(vulkan_water_targets_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_water_targets.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/WaterTargets.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/BufferResource.cpp")
    target_include_directories(vulkan_water_targets_test PRIVATE
        "${XRAY_SOURCE_DIR}" "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include")
    target_compile_options(vulkan_water_targets_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    add_test(NAME vulkan_water_targets COMMAND vulkan_water_targets_test)

    add_executable(vulkan_water_local_pipeline_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_water_local_pipeline.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/DeferredPass.cpp")
    target_include_directories(vulkan_water_local_pipeline_test PRIVATE
        "${XRAY_SOURCE_DIR}" "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include")
    target_compile_options(vulkan_water_local_pipeline_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    add_test(NAME vulkan_water_local_pipeline COMMAND vulkan_water_local_pipeline_test)

    add_executable(vulkan_visual_catalog_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_visual_catalog.cpp"
    )
    target_include_directories(vulkan_visual_catalog_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(vulkan_visual_catalog_test PRIVATE DEBUG)
    target_compile_options(vulkan_visual_catalog_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    target_link_libraries(vulkan_visual_catalog_test PRIVATE xrRenderVK xrCDB xrCore)
    add_test(NAME vulkan_visual_catalog COMMAND vulkan_visual_catalog_test)

    add_executable(vulkan_model_geometry_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_model_geometry.cpp"
    )
    target_include_directories(vulkan_model_geometry_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(vulkan_model_geometry_test PRIVATE DEBUG)
    target_compile_options(vulkan_model_geometry_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    target_link_libraries(vulkan_model_geometry_test PRIVATE xrRenderVK xrEngine xrAPI xrScriptEngine xrCDB xrCore xrMaterialSystem)
    add_test(NAME vulkan_model_geometry COMMAND vulkan_model_geometry_test)

    add_executable(vulkan_kinematics_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_kinematics.cpp"
    )
    target_include_directories(vulkan_kinematics_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(vulkan_kinematics_test PRIVATE DEBUG)
    target_compile_options(vulkan_kinematics_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    target_link_libraries(vulkan_kinematics_test PRIVATE xrRenderVK xrEngine xrAPI xrScriptEngine xrCDB xrCore xrMaterialSystem)
    add_test(NAME vulkan_kinematics COMMAND vulkan_kinematics_test)

    add_executable(vulkan_particle_visual_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_particle_visual.cpp"
    )
    target_include_directories(vulkan_particle_visual_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(vulkan_particle_visual_test PRIVATE DEBUG)
    target_compile_options(vulkan_particle_visual_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    target_link_libraries(vulkan_particle_visual_test PRIVATE xrRenderVK xrParticles xrEngine xrAPI xrScriptEngine xrCDB xrCore xrMaterialSystem)
    add_test(NAME vulkan_particle_visual COMMAND vulkan_particle_visual_test)

    add_executable(vulkan_renderer_module_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_renderer_module.cpp"
    )
    target_include_directories(vulkan_renderer_module_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(vulkan_renderer_module_test PRIVATE DEBUG)
    target_compile_options(vulkan_renderer_module_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    target_link_libraries(vulkan_renderer_module_test PRIVATE xrRenderVK xrParticles xrEngine xrAPI xrScriptEngine xrCDB xrCore xrMaterialSystem)
    add_test(NAME vulkan_renderer_module COMMAND vulkan_renderer_module_test)
    add_executable(vulkan_ui_flow_test "${XRAY_SOURCE_DIR}/tests/vulkan_ui_flow.cpp")
    target_include_directories(vulkan_ui_flow_test PRIVATE "${XRAY_SOURCE_DIR}" "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>)
    target_compile_definitions(vulkan_ui_flow_test PRIVATE DEBUG)
    target_compile_options(vulkan_ui_flow_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    target_link_libraries(vulkan_ui_flow_test PRIVATE xrRenderVK xrParticles xrEngine xrAPI xrScriptEngine xrCDB xrCore xrMaterialSystem)
    add_test(NAME vulkan_ui_flow COMMAND vulkan_ui_flow_test)
    add_executable(vulkan_asset_flow_test "${XRAY_SOURCE_DIR}/tests/vulkan_asset_flow.cpp")
    target_include_directories(vulkan_asset_flow_test PRIVATE "${XRAY_SOURCE_DIR}" "${XRAY_SOURCE_DIR}/src"
        $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>)
    target_compile_definitions(vulkan_asset_flow_test PRIVATE DEBUG)
    target_compile_options(vulkan_asset_flow_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
    target_link_libraries(vulkan_asset_flow_test PRIVATE xrRenderVK xrParticles xrEngine xrAPI xrScriptEngine xrCDB xrCore xrMaterialSystem)
    add_test(NAME vulkan_asset_flow COMMAND vulkan_asset_flow_test)

    if (NOT ANDROID AND UNIX)
        add_executable(vulkan_validation_host_test "${XRAY_SOURCE_DIR}/tests/vulkan_validation_host.cpp")
        target_include_directories(vulkan_validation_host_test PRIVATE "${XRAY_SOURCE_DIR}"
            $<TARGET_PROPERTY:xrRenderVK,INCLUDE_DIRECTORIES>)
        target_link_libraries(vulkan_validation_host_test PRIVATE xrRenderVK xrParticles xrEngine xrAPI xrScriptEngine xrCDB xrCore xrMaterialSystem SDL2::SDL2)
        # Requires an X11/Wayland Vulkan surface and an installed validation layer.
        # Invoke explicitly under xvfb-run when those are available.
    endif()

    add_executable(vulkan_shader_module_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_shader_module.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/ShaderModule.cpp"
    )
    target_include_directories(vulkan_shader_module_test PRIVATE
        "${XRAY_SOURCE_DIR}"
        "${XRAY_SOURCE_DIR}/Externals/Vulkan-Headers/include"
    )
    target_compile_definitions(vulkan_shader_module_test PRIVATE DEBUG)
    target_compile_options(vulkan_shader_module_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>
    )
    add_test(NAME vulkan_shader_module COMMAND vulkan_shader_module_test)
endif()
