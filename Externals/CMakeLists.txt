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
    add_subdirectory(libtheora-proj)
    add_subdirectory(libvorbis-proj)
    add_subdirectory(lzo-proj)

    add_subdirectory(libogg)
    add_library(Ogg::Ogg ALIAS ogg)

    #set(CMAKE_FIND_DEBUG_MODE TRUE)
    find_package(DXSDK)
    #set(CMAKE_FIND_DEBUG_MODE FALSE)

    add_subdirectory(zlib-proj)

    find_package(DbgHelp)
    find_package(FaultRep)
    find_package(EAX)
    find_package(OpenAL)
    
    add_subdirectory(DiscordGameSDK)
endif()

if (NOT TARGET xrLuabind)
    message(FATAL_ERROR
        "You probably have downloaded OpenXRay sources as ZIP archive, you can't do that. Use git to clone the repository.\n"
        "Read the build instructions: https://github.com/OpenXRay/xray-16/wiki"
    )
endif()
