if (OS_MACOS)
    option(LUA_USE_POSIX "Use POSIX functionality." ON)
    option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
else()
    option(LUA_USE_POSIX "Use POSIX functionality." ON)
    option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)

    if (NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD") # OpenBSD has dlopen as a part of libc
        option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
    endif()
endif()

if (OS_MACOS)
    set(ENV{SDKROOT} ${CMAKE_OSX_SYSROOT})
	set(ENV{MACOSX_DEPLOYMENT_TARGET} ${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

# Compiler options
if (PROJECT_PLATFORM_E2K) # E2K: O3 on mcst-lcc approximately equal to O2 at gcc X86/ARM
    set(CCOPT_OPT_LEVEL "-O3")
else()
    set(CCOPT_OPT_LEVEL "-O2")
endif()

if (USE_ADDRESS_SANITIZER)
    set(CCOPT "${CCOPT_OPT_LEVEL} -fno-stack-protector")
else()
    set(CCOPT "${CCOPT_OPT_LEVEL} -fomit-frame-pointer -fno-stack-protector")
endif()

set(CCOPT "${CCOPT} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -U_FORTIFY_SOURCE")

string(REPLACE " " ";" CCOPT "${CCOPT}")

add_compile_options(
    #-Wdeclaration-after-statement
    #-Wredundant-decls
    #-Wshadow
    #-Wpointer-arith
)

if (LUAJIT_DISABLE_FFI)
    list(APPEND XCFLAGS "-DLUAJIT_DISABLE_FFI")
endif()

if (LUAJIT_ENABLE_LUA52COMPAT)
    list(APPEND XCFLAGS "-DLUAJIT_ENABLE_LUA52COMPAT")
endif()

if (LUAJIT_DISABLE_JIT)
    list(APPEND XCFLAGS "-DLUAJIT_DISABLE_JIT")
endif()

if (LUAJIT_DISABLE_GC64)
    list(APPEND XCFLAGS "-DLUAJIT_DISABLE_GC64")
endif()

if (LUAJIT_USE_SYSMALLOC)
    list(APPEND XCFLAGS "-DLUAJIT_USE_SYSMALLOC")
endif()

if (LUAJIT_USE_VALGRIND)
    list(APPEND XCFLAGS "-DLUAJIT_USE_VALGRIND")
endif()

if (LUAJIT_USE_GDBJIT)
    list(APPEND XCFLAGS "-DLUAJIT_USE_GDBJIT")
endif()

if (LUA_USE_APICHECK)
    list(APPEND XCFLAGS "-DLUA_USE_APICHECK")
endif()

if (LUA_USE_ASSERT)
    list(APPEND XCFLAGS "-DLUA_USE_ASSERT")
endif()

set(CCOPTIONS "${CCOPT};${XCFLAGS}")

target_compile_options(xrLuaJIT PRIVATE ${CCOPTIONS})

execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} ${CCOPTIONS} -E lj_arch.h -dM
    WORKING_DIRECTORY "${LUAJIT_DIR}"
    OUTPUT_VARIABLE TESTARCH_OUTPUT
    ERROR_VARIABLE TESTARCH_ERROR
)

if(NOT "${TESTARCH_ERROR}" STREQUAL "")
    message("TESTARCH_ERROR=${TESTARCH_ERROR}")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_X64")
    set(TARGET_LJARCH "x64")
elseif ("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_X86")
    set(TARGET_LJARCH "x86")

    string(APPEND TARGET_XCFLAGS " -march=i686 -msse -msse2 -mfpmath=sse")
elseif ("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_ARM64")
    set(TARGET_LJARCH "arm64")
    if ("${TESTARCH_OUTPUT}" MATCHES "__AARCH64EB__")
        set(TARGET_ARCH "-D__AARCH64EB__=1")
    endif()
elseif ("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_ARM")
    set(TARGET_LJARCH "arm")
elseif ("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_PPC")
    set(TARGET_LJARCH "ppc")
elseif ("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_MIPS")
    if ("${TESTARCH_OUTPUT}" MATCHES "MIPSEL")
        set(TARGET_ARCH "-D__MIPSEL__=1")
    endif()
    if ("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_MIPS64")
        set(TARGET_LJARCH "mips64")
    else()
        set(TARGET_LJARCH "mips")
    endif()
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "e2k") # MCST Elbrus 2000
    set(TARGET_LJARCH "e2k")
else()
    message("${TESTARCH_OUTPUT}")
    message(FATAL_ERROR "Unsupported luajit target architecture (see output above)")
endif()

string(APPEND TARGET_ARCH "-DLUAJIT_TARGET=LUAJIT_ARCH_${TARGET_LJARCH}")

# TODO: add PREFIX, TARGET_XCFLAGS, TARGET_DYNXLDOPTS, MULTILIB here (lines 289-302 in Makefile)
# TODO: use TARGET_STRIP flags?

if (OS_WINDOWS)
    #string(APPEND TARGET_STRIP "--strip-unneeded")

    target_link_options(xrLuaJIT
        PRIVATE
        " -shared -Wl,--out-implib,libluajit-${ABIVER}.dll.a"
    )

    if (NOT LUAJIT_BUILD_STATIC_LIB)
        string(APPEND HOST_XCFLAGS " -DLUA_BUILD_AS_DLL")
    endif()

    set(LJVM_MODE peobj)
elseif (OS_MACOS)
    if (CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL "")
        message(FATAL_ERROR "Missing export MACOSX_DEPLOYMENT_TARGET=XX.YY")
    endif()

    #string(APPEND TARGET_STRIP "-x")
    # XXX: doesn't compile with Apple Clang
    #string(APPEND TARGET_XSHLDFLAGS " -dynamiclib -single_module -undefined dynamic_lookup -fPIC")
    #string(APPEND TARGET_XSHLDFLAGS " -install_name ${TARGET_DYLIBPATH} -compatibility_version ${MAJVER}.${MINVER} -current_version ${MAJVER}.${MINVER}.${RELVER}")

    if (${TARGET_LJARCH} STREQUAL "x64")
        string(APPEND TARGET_XLDFLAGS " -pagezero_size 10000 -image_base 100000000 -image_base 7fff04c4a000")
    elseif (${TARGET_LJARCH} STREQUAL "arm64")
        string(APPEND TARGET_XCFLAGS " -fno-omit-frame-pointer")
    endif()

    set(LJVM_MODE machasm)
else()
    set(LJVM_MODE elfasm)
endif()

# TODO: add HOST_SYS != TARGET_SYS code (lines 354-372 in Makefile)
#string(APPEND HOST_XCFLAGS "-DLUA_BUILD_AS_DLL -DLUAJIT_OS=LUAJIT_OS_WINDOWS")
# TODO: add "-DTARGET_OS_IPHONE=1" on iOS
#string(APPEND HOST_XCFLAGS "-DLUAJIT_OS=LUAJIT_OS_OSX")

# NOTE: Defines in DASM_FLAGS should contain a space after -D for minilua to work

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_LE 1")
    list(APPEND DASM_FLAGS "-D ENDIAN_LE")
else()
    list(APPEND DASM_FLAGS "-D ENDIAN_BE")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_BITS 64")
    list(APPEND DASM_FLAGS "-D P64")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_HASJIT 1")
    list(APPEND DASM_FLAGS "-D JIT")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_HASFFI 1")
    list(APPEND DASM_FLAGS "-D FFI")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_DUALNUM 1")
    list(APPEND DASM_FLAGS "-D DUALNUM")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_HASFPU 1")
    list(APPEND DASM_FLAGS "-D FPU")
    list(APPEND TARGET_ARCH "-DLJ_ARCH_HASFPU=1")
else()
    list(APPEND TARGET_ARCH "-DLJ_ARCH_HASFPU=0")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_ABI_SOFTFP 1")
    list(APPEND TARGET_ARCH "-DLJ_ABI_SOFTFP=1")
else()
    list(APPEND DASM_FLAGS "-D HFABI")
    list(APPEND TARGET_ARCH "-DLJ_ABI_SOFTFP=0")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_NO_UNWIND 1")
    list(APPEND DASM_FLAGS "-D NO_UNWIND")
    list(APPEND TARGET_ARCH "-DLUAJIT_NO_UNWIND")
endif()

if ("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_VERSION")
    string(REGEX MATCH "LJ_ARCH_VERSION ([0-9]+)$" _ "${TESTARCH_OUTPUT}")
    list(APPEND DASM_FLAGS "-D VER=${CMAKE_MATCH_1}")
else()
    list(APPEND DASM_FLAGS "-D VER=")
endif()

set(DASM_ARCH ${TARGET_LJARCH})

if (OS_WINDOWS)
    list(APPEND DASM_FLAGS "-D WIN")
endif()

if (TARGET_LJARCH STREQUAL "x64")
    if (NOT "${TESTARCH_OUTPUT}" MATCHES "LJ_FR2 1")
        set(DASM_ARCH "x86")
    endif()
elseif (TARGET_LJARCH STREQUAL "arm")
    if (APPLE)
        list(APPEND DASM_FLAGS "-D IOS")
    endif()
elseif ("${TESTARCH_OUTPUT}" MATCHES "LJ_TARGET_MIPSR6")
    list(APPEND DASM_FLAGS "-D MIPSR6")
endif()

if (TARGET_LJARCH STREQUAL "ppc")
    if ("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_SQRT 1")
        list(APPEND DASM_FLAGS "-D SQRT")
    endif()
    if ("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_ROUND 1")
        list(APPEND DASM_FLAGS "-D ROUND")
    endif()
    if ("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_PPC32ON64 1")
        list(APPEND DASM_FLAGS "-D GPR64")
    endif()
    if ("${TESTARCH_OUTPUT}" MATCHES "LJ_ARCH_PPC64")
        set(DASM_ARCH "ppc64")
    endif()
endif()

set(HOST_ACFLAGS "${CMAKE_C_FLAGS} ${CCOPTIONS} ${TARGET_ARCH}")
set(HOST_ALDFLAGS "${CMAKE_C_FLAGS}")

string(APPEND TARGET_XCFLAGS " -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -U_FORTIFY_SOURCE")

separate_arguments(HOST_ACFLAGS)
separate_arguments(HOST_ALDFLAGS)
separate_arguments(DASM_FLAGS)
separate_arguments(TARGET_XCFLAGS)

target_compile_options(xrLuaJIT
    PRIVATE
    ${TARGET_XCFLAGS}
)

set(DASM_DASC "${LUAJIT_DIR}/vm_${DASM_ARCH}.dasc")
set(DASM "${LUAJIT_DIR}/../dynasm/dynasm.lua")

if (PROJECT_PLATFORM_E2K)
    set(BUILDVM_ARCH "${LUAJIT_DIR}/host/buildvm_arch.h")
else()
    set(BUILDVM_ARCH "${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h")
endif()

# Generate buildvm arch header
if (NOT PROJECT_PLATFORM_E2K)
    set(MINILUA_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/minilua")
    set(MINILUA_FILE "${MINILUA_BINARY_DIR}/minilua")

    add_custom_command(
        OUTPUT "${MINILUA_FILE}"
        COMMAND ${CMAKE_COMMAND}
            -B${MINILUA_BINARY_DIR}
            -G${CMAKE_GENERATOR}
            -S${CMAKE_CURRENT_SOURCE_DIR}/HostBuildTools/minilua
            -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
            -DCMAKE_BUILD_TYPE=Release
            -DLUAJIT_DIR=${LUAJIT_DIR}
            -DLUA_USE_POSIX=${LUA_USE_POSIX}
            -DHOST_ACFLAGS="${HOST_ACFLAGS}"
            -DHOST_ALDFLAGS="${HOST_ALDFLAGS}"
        COMMAND ${CMAKE_COMMAND} --build ${MINILUA_BINARY_DIR} --config Release --verbose
    )

    add_custom_command(OUTPUT ${BUILDVM_ARCH}
        COMMAND "${MINILUA_FILE}" ${DASM} ${DASM_FLAGS} -o ${BUILDVM_ARCH} ${DASM_DASC}
        DEPENDS "${MINILUA_FILE}"
    )

    add_custom_target(buildvm_arch
        DEPENDS ${BUILDVM_ARCH}
    )

    set_target_properties(buildvm_arch PROPERTIES
         ADDITIONAL_CLEAN_FILES "${MINILUA_BINARY_DIR}"
    )
endif()

# Buildvm
set(BUILDVM_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/buildvm")
set(BUILDVM_FILE "${BUILDVM_BINARY_DIR}/buildvm")

add_custom_command(
    OUTPUT "${BUILDVM_FILE}"
    COMMAND ${CMAKE_COMMAND}
        -B${BUILDVM_BINARY_DIR}
        -G${CMAKE_GENERATOR}
        -S${CMAKE_CURRENT_SOURCE_DIR}/HostBuildTools/buildvm
		-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
        -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
        -DCMAKE_BUILD_TYPE=Release
        -DLUAJIT_DIR=${LUAJIT_DIR}
        -DBUILDVM_ARCH=${BUILDVM_ARCH}
        -DHOST_ACFLAGS="${HOST_ACFLAGS}"
        -DHOST_ALDFLAGS="${HOST_ALDFLAGS}"
    COMMAND ${CMAKE_COMMAND} --build ${BUILDVM_BINARY_DIR} --config Release --verbose
)

add_custom_target(buildvm
    DEPENDS "${BUILDVM_FILE}"
)

set_target_properties(buildvm PROPERTIES
    ADDITIONAL_CLEAN_FILES "${BUILDVM_BINARY_DIR}"
)

if (NOT PROJECT_PLATFORM_E2K)
    add_dependencies(buildvm buildvm_arch)
endif()

target_compile_options(xrLuaJIT
    PRIVATE
    -Wno-comment
)
