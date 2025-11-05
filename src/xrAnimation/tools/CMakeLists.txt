set(_xrAnimation_tool_targets "")

# Find Vulkan SDK (required for ozz_animation_viewer Vulkan renderer)
# Vulkan SDK is a system dependency - install from:
#   Windows: https://vulkan.lunarg.com/
#   Linux: sudo apt-get install libvulkan-dev vulkan-tools
find_package(Vulkan COMPONENTS glslc)
if(Vulkan_FOUND)
    message(STATUS "Vulkan SDK found: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan library: ${Vulkan_LIBRARIES}")
    if(Vulkan_glslc_FOUND)
        message(STATUS "Vulkan glslc shader compiler found")
    endif()

    # Check for shader compiler (glslc preferred, glslangValidator as fallback)
    find_program(GLSLC glslc HINTS ${Vulkan_INCLUDE_DIRS}/../bin ENV VULKAN_SDK)
    if(NOT GLSLC)
        find_program(GLSLC glslangValidator HINTS ${Vulkan_INCLUDE_DIRS}/../bin ENV VULKAN_SDK)
        if(GLSLC)
            set(USE_GLSLANG_VALIDATOR TRUE)
            message(STATUS "Using glslangValidator for shader compilation")
        endif()
    else()
        message(STATUS "Found glslc: ${GLSLC}")
    endif()
else()
    message(WARNING "Vulkan SDK not found. Vulkan renderer will not be available.")
    message(WARNING "Install Vulkan SDK or run: sudo apt-get install libvulkan-dev vulkan-tools glslang-tools")
endif()

# Find X11 libraries (required for GLFW on Linux)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        message(STATUS "X11 found: ${X11_LIBRARIES}")
    endif()
endif()

# Shader compilation function
function(add_shader TARGET SHADER)
    if(NOT GLSLC)
        message(WARNING "Shader compiler not found. Shaders will not be compiled.")
        return()
    endif()

    set(SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/${SHADER})
    set(SHADER_SPIRV ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER}.spv)

    if(USE_GLSLANG_VALIDATOR)
        # glslangValidator syntax: glslangValidator -V input.vert -o output.spv
        add_custom_command(
            OUTPUT ${SHADER_SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${GLSLC} -V ${SHADER_SOURCE} -o ${SHADER_SPIRV}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling shader ${SHADER} with glslangValidator"
        )
    else()
        # glslc syntax
        add_custom_command(
            OUTPUT ${SHADER_SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${GLSLC} ${SHADER_SOURCE} -o ${SHADER_SPIRV}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling shader ${SHADER} with glslc"
        )
    endif()

    target_sources(${TARGET} PRIVATE ${SHADER_SPIRV})
endfunction()

# X-Ray to ozz converter tool (optional until implementation lands)
set(_converter_src ${CMAKE_CURRENT_SOURCE_DIR}/xray_to_ozz_converter.cpp)
if(EXISTS ${_converter_src})
    add_executable(xray_to_ozz_converter
        xray_to_ozz_converter.cpp
        ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples/framework/mesh.cc
    )

    target_link_libraries(xray_to_ozz_converter
        PRIVATE
        xrAnimation
        xrCore
        xrEngine
        xrScriptEngine
        ozz_animation_offline
        ozz_animation
        ozz_base
    )

    target_include_directories(xray_to_ozz_converter
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/xrAnimation
        ${CMAKE_SOURCE_DIR}/src/xrAnimation/Include
        ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/include
        ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples
        ${CMAKE_SOURCE_DIR}/Externals/imgui
    )

    set_target_properties(xray_to_ozz_converter PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}
    )

    list(APPEND _xrAnimation_tool_targets xray_to_ozz_converter)
endif()

# ozz animation viewer tool (using ozz playback sample)
# Now supported on all platforms including Visual Studio with GLFW submodule
set(_ozz_animation_viewer_supported ON)

if(_ozz_animation_viewer_supported AND Vulkan_FOUND)
    # Renderer source files
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
        ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples/framework/mesh.cc
        ${RENDERER_SOURCES}
        ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/Externals/imgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/Externals/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/Externals/imgui/backends/imgui_impl_vulkan.cpp
    )

    # Link required runtime libraries and Vulkan
    target_link_libraries(ozz_animation_viewer
        PRIVATE
        glfw
        xrAnimation
        xrCore
        xrScriptEngine
        xrEngine
        ozz_animation
        ozz_base
        Vulkan::Vulkan
    )

    # Link X11 libraries on Linux
    if(UNIX AND NOT APPLE AND X11_FOUND)
        target_link_libraries(ozz_animation_viewer
            PRIVATE
            ${X11_LIBRARIES}
        )
    endif()

    target_include_directories(ozz_animation_viewer
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer
        ${CMAKE_SOURCE_DIR}/src/xrAnimation
        ${CMAKE_SOURCE_DIR}/src/xrAnimation/Include
        ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/include
        ${CMAKE_SOURCE_DIR}/Externals/ozz-animation/samples
        ${CMAKE_SOURCE_DIR}/Externals/imgui
        ${CMAKE_SOURCE_DIR}/Externals/imgui/backends
        ${CMAKE_SOURCE_DIR}/Externals/vma
        ${CMAKE_SOURCE_DIR}/Externals/tracy/public
        ${CMAKE_SOURCE_DIR}/Externals/entt/single_include
        ${CMAKE_SOURCE_DIR}/sdk/include/SDL2
    )

    target_compile_definitions(ozz_animation_viewer
        PRIVATE
        OZZ_SHADER_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders"
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )


    # Add shaders if compiler is available
    if(GLSLC)
        # Triangle test shaders (validation) - only compile what exists
        add_shader(ozz_animation_viewer triangle.vert)
        add_shader(ozz_animation_viewer triangle.frag)

        # Skeleton visualization shaders
        add_shader(ozz_animation_viewer skeleton_instanced.vert)
        add_shader(ozz_animation_viewer skeleton_instanced.frag)

        # Debug line shaders
        add_shader(ozz_animation_viewer debug_lines.vert)
        add_shader(ozz_animation_viewer debug_lines.frag)

        # Shaded bone shaders
        add_shader(ozz_animation_viewer shaded_bone.vert)
        add_shader(ozz_animation_viewer shaded_bone.frag)

        # GPU instanced bone shaders
        add_shader(ozz_animation_viewer bone_instanced.vert)
        add_shader(ozz_animation_viewer bone_instanced.frag)

        # Skinned mesh instanced shaders
        add_shader(ozz_animation_viewer skinned_mesh_instanced.vert)
        add_shader(ozz_animation_viewer skinned_mesh_instanced.frag)
    endif()
    # Set properties
    set_target_properties(ozz_animation_viewer PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}
    )

    list(APPEND _xrAnimation_tool_targets ozz_animation_viewer)
elseif(_ozz_animation_viewer_supported AND NOT Vulkan_FOUND)
    message(WARNING "Skipping ozz_animation_viewer: Vulkan SDK not found")
    set(_ozz_animation_viewer_supported OFF)
endif()

# Install any tools that have been enabled
if(_xrAnimation_tool_targets)
    install(TARGETS ${_xrAnimation_tool_targets}
        RUNTIME DESTINATION bin
    )
endif()
