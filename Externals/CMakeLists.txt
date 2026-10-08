if (XRAY_USE_LUAJIT)
    add_subdirectory(LuaJIT-proj)
else()
    find_package(Lua51 REQUIRED)
endif()

add_subdirectory(luabind)
add_subdirectory(xrLuaFix)
add_subdirectory(OPCODE)
add_subdirectory(ode)

# Silence all warnings for external libraries
add_compile_options(${XRAY_DISABLE_WARNINGS})

add_subdirectory(GameSpy)

if (ANDROID)
    # The bundled GameSpy sources use pthread_cancel on their Linux path,
    # while Android's Bionic pthread API intentionally omits it. Keep the
    # compatibility shim in the parent project so the GameSpy submodule stays
    # at its upstream commit.
    target_sources(GameSpy-oxr PRIVATE
        "${CMAKE_SOURCE_DIR}/src/Common/AndroidGameSpyThreadCompat.c"
    )
    target_compile_options(GameSpy-oxr PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-include>
        $<$<COMPILE_LANGUAGE:C>:${CMAKE_SOURCE_DIR}/src/Common/AndroidGameSpyThreadCompat.h>
    )
endif()

add_subdirectory(imgui-proj)

if (XRAY_STATIC_PIC)
    # The Android SDL activity loads the engine as libmain.so.  Keep the
    # static external archives PIC without requiring changes in submodules.
    foreach (xray_static_target IN ITEMS xrLuaFix GameSpy-oxr xrImGui)
        if (TARGET ${xray_static_target})
            set_property(TARGET ${xray_static_target} PROPERTY POSITION_INDEPENDENT_CODE ON)
        endif()
    endforeach()
endif()

if (NOT TARGET xrLuabind)
    message(FATAL_ERROR
        "You probably have downloaded OpenXRay sources as ZIP archive, you can't do that. Use git to clone the repository.\n"
        "Read the build instructions: https://github.com/OpenXRay/xray-16/wiki"
    )
endif()
