if (XRAY_USE_LUAJIT)
    add_subdirectory(LuaJIT-proj)
else()
    find_package(Lua51 REQUIRED)
endif()

add_subdirectory(xrLuaFix)
add_subdirectory(luabind)
add_subdirectory(GameSpy)
add_subdirectory(OPCODE)
add_subdirectory(ode)
add_subdirectory(imgui-proj)

if (OS_WINDOWS)
    add_subdirectory(BugTrap-proj)
    add_subdirectory(zlib-proj)
    add_subdirectory(DiscordGameSDK)
endif()

if (NOT TARGET xrLuabind)
    message(FATAL_ERROR
        "You probably have downloaded OpenXRay sources as ZIP archive, you can't do that. Use git to clone the repository.\n"
        "Read the build instructions: https://github.com/OpenXRay/xray-16/wiki"
    )
endif()
