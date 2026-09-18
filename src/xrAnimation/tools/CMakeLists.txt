set(_xrAnimation_tool_targets "")

set(_converter_src ${CMAKE_CURRENT_SOURCE_DIR}/xray_to_ozz_converter.cpp)
if(EXISTS ${_converter_src})
    add_executable(xray_to_ozz_converter
        xray_to_ozz_converter.cpp
        stdafx.h
        stdafx.cpp
    )

    target_link_libraries(xray_to_ozz_converter
        PRIVATE
        xrAnimation
        xrCore
        ozz_animation_offline
        ozz_animation
        ozz_base
    )

    target_include_directories(xray_to_ozz_converter
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/xrAnimation
        ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/include
        ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples
    )

    set_target_properties(xray_to_ozz_converter PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}
    )

    target_precompile_headers(xray_to_ozz_converter PRIVATE stdafx.h)

    list(APPEND _xrAnimation_tool_targets xray_to_ozz_converter)
endif()

option(XRAY_BUILD_OZZ_VIEWER "Build the standalone ozz animation viewer" OFF)

if(XRAY_BUILD_OZZ_VIEWER)
    find_package(Vulkan COMPONENTS glslc)

    if(NOT Vulkan_FOUND)
        message(WARNING "XRAY_BUILD_OZZ_VIEWER=ON but Vulkan SDK not found. Skipping viewer.")
    else()
        find_program(GLSLC glslc HINTS ${Vulkan_INCLUDE_DIRS}/../bin ENV VULKAN_SDK)
        if(NOT GLSLC)
            find_program(GLSLC glslangValidator HINTS ${Vulkan_INCLUDE_DIRS}/../bin ENV VULKAN_SDK)
            if(GLSLC)
                set(USE_GLSLANG_VALIDATOR TRUE)
            endif()
        endif()

        function(add_shader TARGET SHADER)
            if(NOT GLSLC)
                return()
            endif()
            set(SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/${SHADER})
            set(SHADER_SPIRV ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER}.spv)
            if(USE_GLSLANG_VALIDATOR)
                add_custom_command(
                    OUTPUT ${SHADER_SPIRV}
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
                    COMMAND ${GLSLC} -V ${SHADER_SOURCE} -o ${SHADER_SPIRV}
                    DEPENDS ${SHADER_SOURCE}
                )
            else()
                add_custom_command(
                    OUTPUT ${SHADER_SPIRV}
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
                    COMMAND ${GLSLC} ${SHADER_SOURCE} -o ${SHADER_SPIRV}
                    DEPENDS ${SHADER_SOURCE}
                )
            endif()
            target_sources(${TARGET} PRIVATE ${SHADER_SPIRV})
        endfunction()

        set(RENDERER_SOURCES
            renderer/VMAImplementation.cpp
            renderer/VulkanDevice.cpp
            renderer/VulkanBuffer.cpp
            renderer/VulkanCommandBuffer.cpp
            renderer/VulkanDescriptors.cpp
            renderer/VulkanPipeline.cpp
            renderer/MeshData.cpp
            renderer/InstancedSkeletonRenderer.cpp
            renderer/InstancedMeshRenderer.cpp
            renderer/DebugRenderer.cpp
            renderer/Camera.cpp
            renderer/VulkanRenderer.cpp
        )

        add_executable(ozz_animation_viewer
            ozz_animation_viewer.cpp
            SimpleObjLoader.cpp
            ozz_animation_viewer/AnimationECS_DebugRender.cpp
            ozz_animation_viewer/AnimationECS_IK.cpp
            ${RENDERER_SOURCES}
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_draw.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_tables.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_widgets.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_demo.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/backends/imgui_impl_sdl3.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/backends/imgui_impl_vulkan.cpp
        )

        set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_draw.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_tables.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_widgets.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_demo.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/backends/imgui_impl_sdl3.cpp
            ${CMAKE_SOURCE_DIR}/Externals/imgui/backends/imgui_impl_vulkan.cpp
            PROPERTIES SKIP_PRECOMPILE_HEADERS ON
        )

        target_link_libraries(ozz_animation_viewer
            PRIVATE
            SDL3::SDL3-static
            xrAnimation
            xrCore
            ozz_animation
            ozz_base
            Vulkan::Vulkan
        )

        target_include_directories(ozz_animation_viewer
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/renderer
            ${CMAKE_SOURCE_DIR}/src/xrAnimation
            ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/include
            ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples
            ${CMAKE_SOURCE_DIR}/Externals/imgui
            ${CMAKE_SOURCE_DIR}/Externals/imgui/backends
            ${CMAKE_SOURCE_DIR}/Externals/vma
            ${CMAKE_SOURCE_DIR}/Externals/entt/single_include
            ${CMAKE_SOURCE_DIR}/Externals/SDL/include
        )

        target_compile_definitions(ozz_animation_viewer
            PRIVATE
            OZZ_SHADER_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders"
            NOMINMAX
        )

        if(GLSLC)
            add_shader(ozz_animation_viewer triangle.vert)
            add_shader(ozz_animation_viewer triangle.frag)
            add_shader(ozz_animation_viewer skeleton_instanced.vert)
            add_shader(ozz_animation_viewer skeleton_instanced.frag)
            add_shader(ozz_animation_viewer debug_lines.vert)
            add_shader(ozz_animation_viewer debug_lines.frag)
            add_shader(ozz_animation_viewer shaded_bone.vert)
            add_shader(ozz_animation_viewer shaded_bone.frag)
            add_shader(ozz_animation_viewer bone_instanced.vert)
            add_shader(ozz_animation_viewer bone_instanced.frag)
            add_shader(ozz_animation_viewer skinned_mesh_instanced.vert)
            add_shader(ozz_animation_viewer skinned_mesh_instanced.frag)
        endif()

        set_target_properties(ozz_animation_viewer PROPERTIES
            UNITY_BUILD OFF
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}
        )

        target_precompile_headers(ozz_animation_viewer PRIVATE stdafx.h)

        list(APPEND _xrAnimation_tool_targets ozz_animation_viewer)
    endif()
endif()

if(_xrAnimation_tool_targets)
    install(TARGETS ${_xrAnimation_tool_targets}
        RUNTIME DESTINATION bin
    )
endif()
