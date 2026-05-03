if (XRAY_USE_LUAJIT)
    add_subdirectory(LuaJIT-proj)
else()
    find_package(Lua51 REQUIRED)
endif()

add_library(sse2neon INTERFACE)
target_include_directories(sse2neon INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

set(SDL_SHARED              OFF CACHE BOOL "Build SDL3 as static library")
set(SDL_STATIC              ON  CACHE BOOL "Build SDL3 as static library")
set(SDL_TEST_LIBRARY        OFF CACHE BOOL "Skip SDL3 test library")
set(SDL_INSTALL             OFF CACHE BOOL "Disable SDL3 install rules")
set(SDL_DISABLE_INSTALL     ON  CACHE BOOL "Disable SDL3 install rules")
set(SDL_DISABLE_INSTALL_DOCS ON CACHE BOOL "Disable SDL3 docs install")
add_subdirectory(SDL)

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

set(protobuf_INSTALL          ON  CACHE BOOL "protobuf: export targets so GNS install export set is consistent")
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

    add_subdirectory(protobuf)

    set(BUILD_STATIC_LIB ON  CACHE BOOL "GNS: build static library")
    set(BUILD_SHARED_LIB OFF CACHE BOOL "GNS: skip shared library")
    set(Protobuf_USE_STATIC_LIBS ON CACHE BOOL "GNS: link protobuf as static (no PROTOBUF_USE_DLLS)")
    set(BUILD_EXAMPLES   OFF CACHE BOOL "GNS: skip examples")
    set(BUILD_TESTS      OFF CACHE BOOL "GNS: skip tests")
    set(BUILD_TOOLS      OFF CACHE BOOL "GNS: skip tools")
    set(USE_STEAMWEBRTC  OFF CACHE BOOL "GNS: skip WebRTC ICE (use native ICE instead)")
    if (WIN32)
        set(USE_CRYPTO       "BCrypt"    CACHE STRING "GNS: AES/SHA via Windows BCrypt")
        set(USE_CRYPTO25519  "Reference" CACHE STRING "GNS: ed25519/curve25519 via portable reference impl")
    else()
        set(USE_CRYPTO       "OpenSSL" CACHE STRING "GNS: AES/SHA via OpenSSL")
        set(USE_CRYPTO25519  "OpenSSL" CACHE STRING "GNS: ed25519/curve25519 via OpenSSL")
    endif()

    function(install)
    endfunction()
    function(configure_package_config_file)
    endfunction()
    function(write_basic_package_version_file)
    endfunction()
    add_subdirectory(GameNetworkingSockets)
endblock()

if (NOT TARGET xrLuabind)
    message(FATAL_ERROR
        "You probably have downloaded OpenXRay sources as ZIP archive, you can't do that. Use git to clone the repository.\n"
        "Read the build instructions: https://github.com/OpenXRay/xray-16/wiki"
    )
endif()
