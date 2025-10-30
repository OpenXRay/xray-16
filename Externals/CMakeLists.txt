if (XRAY_USE_LUAJIT)
    add_subdirectory(LuaJIT-proj)
else()
    find_package(Lua51 REQUIRED)
endif()

add_subdirectory(luabind)
add_subdirectory(xrLuaFix)
add_subdirectory(OPCODE)
add_subdirectory(ode)

# NVRHI graphics abstraction layer
set(NVRHI_BUILD_SHARED OFF CACHE BOOL "Build NVRHI as static library")
set(NVRHI_WITH_DX11 ON CACHE BOOL "Build NVRHI D3D11 backend")
set(NVRHI_WITH_DX12 OFF CACHE BOOL "Disable DX12 for now")
set(NVRHI_WITH_VULKAN OFF CACHE BOOL "Disable Vulkan for now")
set(NVRHI_WITH_VALIDATION ON CACHE BOOL "Enable validation layer for development")
set(NVRHI_INSTALL OFF CACHE BOOL "Disable NVRHI install rules")
add_subdirectory(nvrhi)

# Silence all warnings for external libraries
add_compile_options(${XRAY_DISABLE_WARNINGS})

add_subdirectory(GameSpy)
add_subdirectory(imgui-proj)

if (NOT TARGET xrLuabind)
    message(FATAL_ERROR
        "You probably have downloaded OpenXRay sources as ZIP archive, you can't do that. Use git to clone the repository.\n"
        "Read the build instructions: https://github.com/OpenXRay/xray-16/wiki"
    )
endif()
