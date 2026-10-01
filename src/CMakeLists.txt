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
    function(xray_add_vulkan_state_test target source)
        add_executable(${target} "${XRAY_SOURCE_DIR}/${source}")
        target_include_directories(${target} PRIVATE "${XRAY_SOURCE_DIR}")
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

    add_executable(vulkan_deferred_pass_test
        "${XRAY_SOURCE_DIR}/tests/vulkan_deferred_pass.cpp"
        "${XRAY_SOURCE_DIR}/src/Layers/xrRenderVK/DeferredPass.cpp"
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
    target_link_libraries(vulkan_model_geometry_test PRIVATE xrRenderVK xrEngine xrScriptEngine xrCDB xrCore)
    add_test(NAME vulkan_model_geometry COMMAND vulkan_model_geometry_test)

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
