# Copyright (C) 2007-2013 LuaDist.
# Created by Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

cmake_minimum_required(VERSION 3.13)

project(xrLuaJIT C CXX ASM)

# Version
set(MAJVER 2)
set(MINVER 0)
set(RELVER 5)
set(ABIVER 5.1)
set(NODOTABIVER 51)

set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
set(LUAJIT_DIR ${CMAKE_SOURCE_DIR}/Externals/LuaJIT/src CACHE PATH "Location of luajit sources")

option(LUAJIT_BUILD_STATIC_LIB "Build as static library" OFF)
option(LUAJIT_BUILD_APP "Build application" OFF)

# NOTE: Not working because there is no lib_package_rel.c file
option(LUA_USE_RELATIVE_LOADLIB "Use modified loadlib.c with support for relative paths on posix systems (Not working)" OFF)

# Extra flags
option(LUAJIT_DISABLE_FFI "Disable the FFI extension to reduce the size of the LuaJIT executable. But please consider that the FFI library is compiled-in, but NOT loaded by default. It only allocates any memory, if you actually make use of it" OFF)
option(LUAJIT_ENABLE_LUA52COMPAT "Features from Lua 5.2 that are unlikely to break existing code are enabled by default. Some other features that *might* break some existing code (e.g. __pairs or os.execute() return values) can be enabled here. Note: this does not provide full compatibility with Lua 5.2 at this time" OFF)
option(LUAJIT_DISABLE_JIT "Disable the JIT compiler, i.e. turn LuaJIT into a pure interpreter" OFF)
option(LUAJIT_DISABLE_GC64 "Disable LJ_GC64 mode for x64" OFF)
option(LUAJIT_USE_SYSMALLOC "Use the system provided memory allocator (realloc) instead of the bundled memory allocator. This is slower, but sometimes helpful for debugging. It's helpful for Valgrind's memcheck tool, too. This option cannot be enabled on x64, since the built-in allocator is mandatory" OFF)
option(LUAJIT_USE_VALGRIND "This option is required to run LuaJIT under Valgrind. The Valgrind header files must be installed. You should enable debug information, too." OFF)
option(LUAJIT_USE_GDBJIT "This is the client for the GDB JIT API. GDB 7.0 or higher is required to make use of it. See lj_gdbjit.c for details. Enabling this causes a non-negligible overhead, even when not running under GDB" OFF)

option(LUA_USE_APICHECK "Turn on assertions for the Lua/C API to debug problems with lua_* calls. This is rather slow, use only while developing C libraries/embeddings" OFF)
option(LUA_USE_ASSERT "Turn on assertions for the whole LuaJIT VM. This significantly slows down everything. Use only if you suspect a problem with LuaJIT itself" OFF)

#option(LUAJIT_DEBUG "Generate debug information" OFF)

if (WIN32)
    option(LUA_BUILD_WLUA "Build wluajit interpreter for no-console applications." ON)
elseif (APPLE)
    option(LUA_USE_POSIX "Use POSIX functionality." ON)
    option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
else()
    option(LUA_USE_POSIX "Use POSIX functionality." ON)

    if (NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD") # OpenBSD has dlopen as a part of libc
        option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
    endif()
endif()

# TODO: check if we need luaconf.h
# TODO: add other variables from luaconf.h if we need them
# Configuration for luaconf.h
set(LUA_PATH "LUA_PATH" CACHE STRING "Environment variable to use as package.path")
set(LUA_CPATH "LUA_CPATH" CACHE STRING "Environment variable to use as package.cpath")
set(LUA_INIT "LUA_INIT" CACHE STRING "Environment variable for initial script")

if (APPLE)
    set(ENV{SDKROOT} ${CMAKE_OSX_SYSROOT})
    set(ENV{MACOSX_DEPLOYMENT_TARGET} ${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

# Clean unnecessary files in LuaJIT source directory
execute_process(
    COMMAND make clean
    WORKING_DIRECTORY ${LUAJIT_DIR}
)

# TODO: check windows supports same target name for static and shared lib
if (LUAJIT_BUILD_STATIC_LIB)
    add_library(xrLuaJIT STATIC)
else()
    add_library(xrLuaJIT SHARED)
endif()

# Compiler options
if (PROJECT_PLATFORM_E2K) # E2K: O3 on mcst-lcc approximately equal to O2 at gcc X86/ARM
    set(CCOPT_OPT_LEVEL "-O3")
else()
    set(CCOPT_OPT_LEVEL "-O2")
endif()

set(CCOPT "${CMAKE_C_FLAGS} ${CCOPT_OPT_LEVEL}")

if (USE_ADDRESS_SANITIZER)
    set(CCOPT "${CCOPT} -fno-stack-protector")
else()
    set(CCOPT "${CCOPT} -fomit-frame-pointer -fno-stack-protector")
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
    COMMAND ${CMAKE_C_COMPILER} ${CCOPTIONS} -E lj_arch.h -dM
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

if (WIN32)
    #string(APPEND TARGET_STRIP "--strip-unneeded")

    target_link_options(xrLuaJIT
        PRIVATE
        " -shared -Wl,--out-implib,libluajit-${ABIVER}.dll.a"
    )

    if (NOT LUAJIT_BUILD_STATIC_LIB)
        string(APPEND HOST_XCFLAGS " -DLUA_BUILD_AS_DLL")
    endif()

    set(LJVM_MODE peobj)
elseif (APPLE)
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

if (WIN32)
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
            -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
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

list(APPEND LJLIB_C
    "${LUAJIT_DIR}/lib_base.c"
    "${LUAJIT_DIR}/lib_bit.c"
    "${LUAJIT_DIR}/lib_debug.c"
    "${LUAJIT_DIR}/lib_ffi.c"
    "${LUAJIT_DIR}/lib_io.c"
    "${LUAJIT_DIR}/lib_jit.c"
    "${LUAJIT_DIR}/lib_math.c"
    "${LUAJIT_DIR}/lib_os.c"
    "${LUAJIT_DIR}/lib_string.c"
    "${LUAJIT_DIR}/lib_table.c"
)

if (LUA_USE_RELATIVE_LOADLIB)
    list(APPEND LJLIB_C "${LUAJIT_DIR}/lib_package_rel.c")
else()
    list(APPEND LJLIB_C "${LUAJIT_DIR}/lib_package.c")
endif()

macro(add_buildvm_target target mode)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${target}"
        COMMAND "${BUILDVM_FILE}" ARGS -m ${mode} -o ${CMAKE_CURRENT_BINARY_DIR}/${target} ${ARGN}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS buildvm ${ARGN}
    )
endmacro()

if (WIN32)
    add_buildvm_target(lj_vm.obj peobj)
    set(LJ_VM_SRC "${CMAKE_CURRENT_BINARY_DIR}/lj_vm.obj")
else()
    add_buildvm_target(lj_vm.S ${LJVM_MODE})
    set(LJ_VM_SRC "${CMAKE_CURRENT_BINARY_DIR}/lj_vm.S")
    set_source_files_properties("${LJ_VM_SRC}" PROPERTIES LANGUAGE CXX)
endif()

add_buildvm_target("lj_bcdef.h" "bcdef" ${LJLIB_C})
add_buildvm_target("lj_ffdef.h" "ffdef" ${LJLIB_C})
add_buildvm_target("lj_libdef.h" "libdef" ${LJLIB_C})
add_buildvm_target("lj_recdef.h" "recdef" ${LJLIB_C})
add_buildvm_target("lj_folddef.h" "folddef" "${LUAJIT_DIR}/lj_opt_fold.c")
add_buildvm_target("jit/vmdef.lua" "libvm" ${LJLIB_C})

target_sources(xrLuaJIT PRIVATE
    "${LJ_VM_SRC}"
    "${CMAKE_CURRENT_BINARY_DIR}/lj_ffdef.h"
    "${CMAKE_CURRENT_BINARY_DIR}/lj_bcdef.h"
    "${CMAKE_CURRENT_BINARY_DIR}/lj_libdef.h"
    "${CMAKE_CURRENT_BINARY_DIR}/lj_recdef.h"
    "${CMAKE_CURRENT_BINARY_DIR}/lj_folddef.h"
)

if (CMAKE_UNITY_BUILD)
    target_sources(xrLuaJIT PRIVATE "${LUAJIT_DIR}/ljamalg.c")
else()
    # TODO: use lj_asm_ lj_emit lj_target?
    target_sources(xrLuaJIT PRIVATE
        ${LJLIB_C}
        "${LUAJIT_DIR}/lauxlib.h"
        "${LUAJIT_DIR}/lib_aux.c"
        "${LUAJIT_DIR}/lib_init.c"
        "${LUAJIT_DIR}/lj_alloc.c"
        "${LUAJIT_DIR}/lj_alloc.h"
        "${LUAJIT_DIR}/lj_api.c"
        "${LUAJIT_DIR}/lj_asm.c"
        "${LUAJIT_DIR}/lj_asm.h"
        "${LUAJIT_DIR}/lj_assert.c"
        "${LUAJIT_DIR}/lj_bc.c"
        "${LUAJIT_DIR}/lj_bc.h"
        "${LUAJIT_DIR}/lj_bcdump.h"
        "${LUAJIT_DIR}/lj_bcread.c"
        "${LUAJIT_DIR}/lj_bcwrite.c"
        "${LUAJIT_DIR}/lj_buf.c"
        "${LUAJIT_DIR}/lj_buf.h"
        "${LUAJIT_DIR}/lj_carith.c"
        "${LUAJIT_DIR}/lj_carith.h"
        "${LUAJIT_DIR}/lj_ccall.c"
        "${LUAJIT_DIR}/lj_ccall.h"
        "${LUAJIT_DIR}/lj_ccallback.c"
        "${LUAJIT_DIR}/lj_ccallback.h"
        "${LUAJIT_DIR}/lj_cconv.c"
        "${LUAJIT_DIR}/lj_cconv.h"
        "${LUAJIT_DIR}/lj_cdata.c"
        "${LUAJIT_DIR}/lj_cdata.h"
        "${LUAJIT_DIR}/lj_char.c"
        "${LUAJIT_DIR}/lj_char.h"
        "${LUAJIT_DIR}/lj_clib.c"
        "${LUAJIT_DIR}/lj_clib.h"
        "${LUAJIT_DIR}/lj_cparse.c"
        "${LUAJIT_DIR}/lj_cparse.h"
        "${LUAJIT_DIR}/lj_crecord.c"
        "${LUAJIT_DIR}/lj_crecord.h"
        "${LUAJIT_DIR}/lj_ctype.c"
        "${LUAJIT_DIR}/lj_ctype.h"
        "${LUAJIT_DIR}/lj_debug.c"
        "${LUAJIT_DIR}/lj_debug.h"
        "${LUAJIT_DIR}/lj_def.h"
        "${LUAJIT_DIR}/lj_dispatch.c"
        "${LUAJIT_DIR}/lj_dispatch.h"
        "${LUAJIT_DIR}/lj_err.c"
        "${LUAJIT_DIR}/lj_err.h"
        "${LUAJIT_DIR}/lj_errmsg.h"
        "${LUAJIT_DIR}/lj_ff.h"
        "${LUAJIT_DIR}/lj_ffrecord.c"
        "${LUAJIT_DIR}/lj_ffrecord.h"
        "${LUAJIT_DIR}/lj_frame.h"
        "${LUAJIT_DIR}/lj_func.c"
        "${LUAJIT_DIR}/lj_func.h"
        "${LUAJIT_DIR}/lj_gc.c"
        "${LUAJIT_DIR}/lj_gc.h"
        "${LUAJIT_DIR}/lj_gdbjit.c"
        "${LUAJIT_DIR}/lj_gdbjit.h"
        "${LUAJIT_DIR}/lj_ir.c"
        "${LUAJIT_DIR}/lj_ircall.h"
        "${LUAJIT_DIR}/lj_iropt.h"
        "${LUAJIT_DIR}/lj_jit.h"
        "${LUAJIT_DIR}/lj_lex.c"
        "${LUAJIT_DIR}/lj_lex.h"
        "${LUAJIT_DIR}/lj_lib.c"
        "${LUAJIT_DIR}/lj_lib.h"
        "${LUAJIT_DIR}/lj_load.c"
        "${LUAJIT_DIR}/lj_mcode.c"
        "${LUAJIT_DIR}/lj_mcode.h"
        "${LUAJIT_DIR}/lj_meta.c"
        "${LUAJIT_DIR}/lj_meta.h"
        "${LUAJIT_DIR}/lj_obj.c"
        "${LUAJIT_DIR}/lj_obj.h"
        "${LUAJIT_DIR}/lj_opt_dce.c"
        "${LUAJIT_DIR}/lj_opt_fold.c"
        "${LUAJIT_DIR}/lj_opt_loop.c"
        "${LUAJIT_DIR}/lj_opt_mem.c"
        "${LUAJIT_DIR}/lj_opt_narrow.c"
        "${LUAJIT_DIR}/lj_opt_sink.c"
        "${LUAJIT_DIR}/lj_opt_split.c"
        "${LUAJIT_DIR}/lj_parse.c"
        "${LUAJIT_DIR}/lj_parse.h"
        "${LUAJIT_DIR}/lj_prng.c"
        "${LUAJIT_DIR}/lj_prng.h"
        "${LUAJIT_DIR}/lj_profile.c"
        "${LUAJIT_DIR}/lj_profile.h"
        "${LUAJIT_DIR}/lj_record.c"
        "${LUAJIT_DIR}/lj_record.h"
        "${LUAJIT_DIR}/lj_snap.c"
        "${LUAJIT_DIR}/lj_snap.h"
        "${LUAJIT_DIR}/lj_state.c"
        "${LUAJIT_DIR}/lj_state.h"
        "${LUAJIT_DIR}/lj_str.c"
        "${LUAJIT_DIR}/lj_str.h"
        "${LUAJIT_DIR}/lj_strfmt.c"
        "${LUAJIT_DIR}/lj_strfmt.h"
        "${LUAJIT_DIR}/lj_strfmt_num.c"
        "${LUAJIT_DIR}/lj_strscan.c"
        "${LUAJIT_DIR}/lj_strscan.h"
        "${LUAJIT_DIR}/lj_tab.c"
        "${LUAJIT_DIR}/lj_tab.h"
        "${LUAJIT_DIR}/lj_trace.c"
        "${LUAJIT_DIR}/lj_trace.h"
        "${LUAJIT_DIR}/lj_traceerr.h"
        "${LUAJIT_DIR}/lj_udata.c"
        "${LUAJIT_DIR}/lj_udata.h"
        "${LUAJIT_DIR}/lj_vm.h"
        "${LUAJIT_DIR}/lj_vmevent.c"
        "${LUAJIT_DIR}/lj_vmevent.h"
        "${LUAJIT_DIR}/lj_vmmath.c"
        "${LUAJIT_DIR}/lua.h"
        "${LUAJIT_DIR}/lua.hpp"
        "${LUAJIT_DIR}/luaconf.h"
        "${LUAJIT_DIR}/luajit.h"
        "${LUAJIT_DIR}/lualib.h"
    )
endif()

target_include_directories(xrLuaJIT
    PUBLIC
    "${CMAKE_SOURCE_DIR}/Externals/LuaJIT/src"

    PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(xrLuaJIT
    PRIVATE
    $<$<BOOL:${LUA_USE_POSIX}>:m>
    $<$<BOOL:${LUA_USE_DLOPEN}>:dl>
)

target_compile_options(xrLuaJIT
    PRIVATE
    -Wno-comment
)

set_target_properties(xrLuaJIT PROPERTIES
    PREFIX ""
    UNITY_BUILD OFF
)

install(
    TARGETS xrLuaJIT LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if (LUAJIT_BUILD_APP)
    add_executable(luajit
        "${LUAJIT_DIR}/luajit.c"
    )

    target_link_libraries(luajit
        PRIVATE
        xrLuaJIT
    )

    target_link_options(luajit
        PRIVATE
        ${TARGET_XLDFLAGS}
    )

    # On Windows build a no-console variant also
    if (LUA_BUILD_WLUA)
        # TODO: check if it works
        add_executable(wluajit WIN32
            "${LUAJIT_DIR}/wmain.c"
            "${LUAJIT_DIR}/luajit.c"
            "${LUAJIT_DIR}/luajit.rc"
        )
        target_link_libraries(wluajit
            PRIVATE
            xrLuaJIT
        )
    endif()
endif()
