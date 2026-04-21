if (XRAY_USE_LUAJIT)
    add_subdirectory(LuaJIT-proj)
else()
    find_package(Lua51 REQUIRED)
endif()

add_library(sse2neon INTERFACE)
target_include_directories(sse2neon INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(luabind)
add_subdirectory(xrLuaFix)
add_subdirectory(OPCODE)
add_subdirectory(ode)

set(NVRHI_BUILD_SHARED OFF CACHE BOOL "Build NVRHI as static library")
if (WIN32)
    set(NVRHI_WITH_DX11 ON CACHE BOOL "Enable D3D11 backend")
    set(NVRHI_WITH_DX12 ON CACHE BOOL "Enable D3D12 backend")
    set(NVRHI_WITH_VULKAN ON CACHE BOOL "Enable Vulkan backend")
else()
    set(NVRHI_WITH_DX11 OFF CACHE BOOL "No D3D11 off-Windows")
    set(NVRHI_WITH_DX12 OFF CACHE BOOL "No D3D12 off-Windows")
    set(NVRHI_WITH_VULKAN ON CACHE BOOL "Enable Vulkan backend")
endif()
set(NVRHI_WITH_VALIDATION ON CACHE BOOL "Enable validation layer for development")
set(NVRHI_INSTALL OFF CACHE BOOL "Disable NVRHI install rules")
add_subdirectory(nvrhi)

# Slang shader compiler
set(SLANG_ENABLE_CUDA OFF CACHE BOOL "Disable CUDA support")
set(SLANG_ENABLE_OPTIX OFF CACHE BOOL "Disable OptiX support")
set(SLANG_ENABLE_NVAPI OFF CACHE BOOL "Disable NVAPI support")
set(SLANG_ENABLE_XLIB OFF CACHE BOOL "Disable Xlib support")
set(SLANG_ENABLE_DXIL ON CACHE BOOL "Enable DXIL generation for DX12")
set(SLANG_ENABLE_GFX OFF CACHE BOOL "Disable slang-gfx (we use NVRHI)")
set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "Disable language server")
set(SLANG_ENABLE_SLANGC ON CACHE BOOL "Enable command line tool (needed to build the neural standard module)")
set(SLANG_ENABLE_SPLIT_DEBUG_INFO OFF CACHE BOOL "Disable slang split debug info (incompatible with XRay's output dir override)")
set(SLANG_ENABLE_SLANGI OFF CACHE BOOL "Disable interpreter")
set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "Disable runtime")
set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Disable Slang tests")
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Disable Slang examples")
set(SLANG_ENABLE_REPLAYER OFF CACHE BOOL "Disable slang-replay tool")
set(SLANG_ENABLE_PREBUILT_BINARIES OFF CACHE BOOL "Build from source")
# Enable parallel compilation for MSVC
if(MSVC)
    add_compile_options(/MP)
endif()
add_subdirectory(slang)

# Silence all warnings for external libraries
add_compile_options(${XRAY_DISABLE_WARNINGS})

add_subdirectory(GameSpy)
add_subdirectory(imgui-proj)
if (WIN32)
    add_subdirectory(DiscordGameSDK)
endif()

if (NOT TARGET xrLuabind)
    message(FATAL_ERROR
        "You probably have downloaded OpenXRay sources as ZIP archive, you can't do that. Use git to clone the repository.\n"
        "Read the build instructions: https://github.com/OpenXRay/xray-16/wiki"
    )
endif()
