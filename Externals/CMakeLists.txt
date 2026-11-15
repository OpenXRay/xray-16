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
add_subdirectory(imgui-proj)

set(protobuf_INSTALL          OFF CACHE BOOL "protobuf: statically linked, nothing to install")
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "protobuf: use dynamic MSVC runtime to match rest of build" FORCE)
set(protobuf_BUILD_TESTS      OFF CACHE BOOL "protobuf: no tests")
set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "protobuf: no conformance")
set(protobuf_BUILD_EXAMPLES   OFF CACHE BOOL "protobuf: no examples")
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "protobuf: static lib")
set(protobuf_WITH_ZLIB        OFF CACHE BOOL "protobuf: skip zlib compression")
set(protobuf_DEBUG_POSTFIX    ""  CACHE STRING "protobuf: no debug postfix")
block()
    # Disable LTO for protobuf + GNS so their archives contain native COFF
    # objects rather than LLVM bitcode. clang-cl's lld-link handles bitcode
    # fine, but MSVC link.exe (used by engine.sln via VS) cannot read bitcode
    # and reports every symbol as unresolved. Overriding the project-wide
    # CMAKE_INTERPROCEDURAL_OPTIMIZATION_* values inside this block scopes
    # the override to just these two subdirs.
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASEMASTERGOLD OFF)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MIXED OFF)

    # Newer clang-cl compiling against MSVC STL emits calls to internal
    # helpers like __std_rotate that live in vcruntime140_1.dll. Recent
    # MSVC toolsets (v14.44) don't ship an import .lib for that DLL, so
    # MSVC link.exe can't resolve those symbols. Setting this to 0 tells
    # the STL headers not to use those vectorized algorithm fast paths —
    # falls back to plain header-only std::rotate etc. Slight perf hit
    # in protobuf/GNS internal loops, not hot paths.
    add_compile_definitions(_USE_STD_VECTOR_ALGORITHMS=0)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    if(MSVC)
        set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <DEFINES> <INCLUDES> /fo <OBJECT> <SOURCE>")
    endif()

    add_subdirectory(protobuf)

    set(BUILD_STATIC_LIB ON  CACHE BOOL "GNS: build static library")
    set(BUILD_SHARED_LIB OFF CACHE BOOL "GNS: skip shared library")
    set(Protobuf_USE_STATIC_LIBS ON CACHE BOOL "GNS: link protobuf as static (no PROTOBUF_USE_DLLS)")
    set(BUILD_EXAMPLES   OFF CACHE BOOL "GNS: skip examples")
    set(BUILD_TESTS      OFF CACHE BOOL "GNS: skip tests")
    set(BUILD_TOOLS      OFF CACHE BOOL "GNS: skip tools")
    set(USE_STEAMWEBRTC  OFF CACHE BOOL "GNS: skip WebRTC ICE (use native ICE instead)")
    set(GNS_INSTALL      OFF CACHE BOOL "GNS: statically linked, nothing to install")
    if (WIN32)
        set(USE_CRYPTO       "BCrypt"    CACHE STRING "GNS: AES/SHA via Windows BCrypt")
        set(USE_CRYPTO25519  "Reference" CACHE STRING "GNS: ed25519/curve25519 via portable reference impl")
    else()
        set(USE_CRYPTO       "OpenSSL" CACHE STRING "GNS: AES/SHA via OpenSSL")
        set(USE_CRYPTO25519  "OpenSSL" CACHE STRING "GNS: ed25519/curve25519 via OpenSSL")
    endif()

    add_subdirectory(GameNetworkingSockets)
endblock()

if (NOT TARGET xrLuabind)
    message(FATAL_ERROR
        "You probably have downloaded OpenXRay sources as ZIP archive, you can't do that. Use git to clone the repository.\n"
        "Read the build instructions: https://github.com/OpenXRay/xray-16/wiki"
    )
endif()
