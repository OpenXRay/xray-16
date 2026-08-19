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
if (APPLE)
    set(MVK_CONFIG_LOG_LEVEL "warn" CACHE STRING "MoltenVK log level")
    set(MVK_EXCLUDE_SPIRV_TOOLS ON CACHE BOOL "MoltenVK: skip SPIRV-Tools (no debug disasm)")
    set(MVK_BUILD_SHADER_CONVERTER_TOOL OFF CACHE BOOL "MoltenVK: skip standalone shader converter tool")
    add_subdirectory(MoltenVK)
endif()

find_package(Vulkan)
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
if (XRAY_USE_NRD)
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NRD/CMakeLists.txt")
        message(FATAL_ERROR "XRAY_USE_NRD=ON but Externals/NRD is missing")
    endif()
    set(NRD_STATIC_LIBRARY ON CACHE BOOL "Build NRD as static library" FORCE)
    set(NRD_NRI ON CACHE BOOL "Pull NRI for NRDIntegration" FORCE)
    set(NRD_EMBEDS_SPIRV_SHADERS ON CACHE BOOL "NRD embeds SPIRV shaders" FORCE)
    set(NRD_EMBEDS_DXIL_SHADERS OFF CACHE BOOL "NRD: no DXIL on Linux Vulkan" FORCE)
    set(NRD_EMBEDS_DXBC_SHADERS OFF CACHE BOOL "NRD: no DXBC on Linux Vulkan" FORCE)
    set(NRD_NORMAL_ENCODING "3" CACHE STRING "NRD normal encoding (RGBA16_UNORM)" FORCE)
    set(NRD_ROUGHNESS_ENCODING "1" CACHE STRING "NRD roughness encoding (LINEAR)" FORCE)
    set(NRI_ENABLE_NVAPI OFF CACHE BOOL "" FORCE)
    set(NRI_ENABLE_AMDAGS OFF CACHE BOOL "" FORCE)
    set(NRI_ENABLE_NVTX_SUPPORT OFF CACHE BOOL "" FORCE)
    set(NRI_ENABLE_NONE_SUPPORT OFF CACHE BOOL "" FORCE)
    set(NRI_ENABLE_D3D11_SUPPORT OFF CACHE BOOL "" FORCE)
    set(NRI_ENABLE_D3D12_SUPPORT OFF CACHE BOOL "" FORCE)
    set(NRI_ENABLE_VK_SUPPORT ON CACHE BOOL "" FORCE)
    add_subdirectory(NRD)

    set(_xray_nri_vk_core "${CMAKE_BINARY_DIR}/_deps/vulkan_headers-src/include/vulkan/vulkan_core.h")
    set(_xray_nri_vk_ok FALSE)
    if (EXISTS "${_xray_nri_vk_core}")
        file(STRINGS "${_xray_nri_vk_core}" _xray_nri_vk_ver REGEX "^#define VK_HEADER_VERSION ")
        if (_xray_nri_vk_ver MATCHES "VK_HEADER_VERSION ([0-9]+)" AND CMAKE_MATCH_1 GREATER_EQUAL 354)
            set(_xray_nri_vk_ok TRUE)
        endif()
    endif()
    if (NOT _xray_nri_vk_ok)
        set(_xray_nri_vk_zip "${CMAKE_BINARY_DIR}/_deps/vulkan_headers-v1.4.354.zip")
        set(_xray_nri_vk_extract "${CMAKE_BINARY_DIR}/_deps/vulkan_headers-v1.4.354-extract")
        message(STATUS "NRD: upgrading Vulkan-Headers for NRI to v1.4.354")
        file(DOWNLOAD
            "https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/v1.4.354.zip"
            "${_xray_nri_vk_zip}"
            SHOW_PROGRESS
            STATUS _xray_nri_vk_dl
        )
        list(GET _xray_nri_vk_dl 0 _xray_nri_vk_dl_code)
        if (NOT _xray_nri_vk_dl_code EQUAL 0)
            message(FATAL_ERROR "NRD: failed to download Vulkan-Headers v1.4.354")
        endif()
        file(REMOVE_RECURSE "${_xray_nri_vk_extract}")
        file(ARCHIVE_EXTRACT INPUT "${_xray_nri_vk_zip}" DESTINATION "${_xray_nri_vk_extract}")
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/vulkan_headers-src")
        file(RENAME "${_xray_nri_vk_extract}/Vulkan-Headers-1.4.354" "${CMAKE_BINARY_DIR}/_deps/vulkan_headers-src")
        file(REMOVE_RECURSE "${_xray_nri_vk_extract}")
    endif()

    if (SHADERMAKE_PATH AND NOT EXISTS "${SHADERMAKE_PATH}")
        get_filename_component(_sm_dir "${SHADERMAKE_PATH}" DIRECTORY)
        get_filename_component(_sm_alt "${_sm_dir}/../ShaderMake" ABSOLUTE)
        if (EXISTS "${_sm_alt}")
            file(MAKE_DIRECTORY "${_sm_dir}")
            file(CREATE_LINK "${_sm_alt}" "${SHADERMAKE_PATH}" SYMBOLIC)
            message(STATUS "NRD: linked ShaderMake -> ${SHADERMAKE_PATH}")
        endif()
    endif()

    message(STATUS "NRD: enabled (NRI + SPIRV, normalEnc=3 roughnessEnc=1)")
endif()
