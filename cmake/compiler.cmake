if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0 AND NOT PROJECT_PLATFORM_E2K)
        message(FATAL_ERROR "Building with a gcc version less than 8.0 is not supported.")
    elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0 AND PROJECT_PLATFORM_E2K)
        message(FATAL_ERROR "Building with a MCST lcc version less than 1.25 is not supported.")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # XXX: Remove -fdelayed-template-parsing
    add_compile_options(
        -fdelayed-template-parsing
        -Wno-unused-command-line-argument
        -Wno-inconsistent-missing-override
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT XRAY_USE_DEFAULT_CXX_LIB)
    if (NOT XRAY_CXX_LIB)
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-stdlib=libc++" LIBCPP_AVAILABLE)
        CHECK_CXX_COMPILER_FLAG("-stdlib=libstdc++" LIBSTDCPP_AVAILABLE)

        if (LIBCPP_AVAILABLE)
            set(XRAY_CXX_LIB "libc++" CACHE STRING "" FORCE)
        elseif (LIBSTDCPP_AVAILABLE)
            set(XRAY_CXX_LIB "libstdc++" CACHE STRING "" FORCE)
        else()
            message("Neither libstdc++ nor libc++ are available. Hopefully, system has another custom stdlib?")
        endif()
    endif()

    if (XRAY_CXX_LIB STREQUAL "libstdc++")
        add_compile_options(-stdlib=libstdc++)
    elseif (XRAY_CXX_LIB STREQUAL "libc++")
        add_compile_options(-stdlib=libc++)
        if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            add_compile_options(-lcxxrt)
        else()
            add_compile_options(-lc++abi)
        endif()
    endif()
endif()

add_compile_options(-Wno-attributes)
if (APPLE)
    add_compile_options(-Wl,-undefined,error)
else()
    add_compile_options(-Wl,--no-undefined)
endif()

if (USE_ADDRESS_SANITIZER)
    add_compile_options(
        -fsanitize=address
        -fsanitize=leak
        -fsanitize=undefined
        -fno-omit-frame-pointer
        -fno-optimize-sibling-calls
        -fno-sanitize=vptr
    )

    add_link_options(
        $<$<CXX_COMPILER_ID:Clang>:-shared-libasan>
    )
endif()

if (USE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED)

    if (LTO_SUPPORTED)
        # With clang cmake only enables '-flto=thin' but we want full LTO
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-flto=full)
        else()
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
        endif()
    endif()
endif()

if (PROJECT_PLATFORM_ARM)
    add_compile_options(-mfpu=neon)
elseif (PROJECT_PLATFORM_ARM64)
    #add_compile_options()
elseif (PROJECT_PLATFORM_E2K)
    add_compile_options(-Wno-unknown-pragmas)
elseif (PROJECT_PLATFORM_PPC)
    add_compile_options(
        -maltivec
        -mabi=altivec
    )
    add_compile_definitions(NO_WARN_X86_INTRINSICS)
else()
    add_compile_options(
        -mfpmath=sse
        -msse3
    )
endif()

if (XRAY_LINKER)
    add_link_options(-fuse-ld=${XRAY_LINKER})
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(
        DEBUG
        MIXED
    )
    add_compile_options(-Og)
endif()

add_compile_definitions(
    _MT
    _CPPUNWIND
)
