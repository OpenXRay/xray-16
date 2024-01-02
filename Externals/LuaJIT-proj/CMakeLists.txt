# Copyright (C) 2007-2013 LuaDist.
# Created by Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

project(xrLuaJIT C CXX ASM)

set(MAJVER 2)
set(MINVER 0)
set(RELVER 5)
set(ABIVER 5.1)
set(NODOTABIVER 51)

option(LUAJIT_BUILD_STATIC_LIB "Build as static library" ON)
option(LUAJIT_BUILD_APP "Build application" OFF)

if (OS_WINDOWS)
    option(LUA_BUILD_WLUA "Build wluajit interpreter for no-console applications." ON)
endif()

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

set(LUAJIT_DIR ${CMAKE_SOURCE_DIR}/Externals/LuaJIT/src CACHE PATH "Location of luajit sources")
set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")

# TODO move this block after rebasing
if (OS_WINDOWS)
    option(LUA_BUILD_WLUA "Build wluajit interpreter for no-console applications." ON)
elseif (APPLE)
    option(LUA_USE_POSIX "Use POSIX functionality." ON)
    option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
else()
    option(LUA_USE_POSIX "Use POSIX functionality." ON)

    # OpenBSD has dl as a part of libc
    # NetBSD includes dl functions automatically
    if (NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" AND NOT CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
        # This option only links dl and doesn't change anything other from that.
        option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
    endif()
endif()

# TODO: check if we need luaconf.h
# TODO: add other variables from luaconf.h if we need them
# Configuration for luaconf.h
set(LUA_PATH "LUA_PATH" CACHE STRING "Environment variable to use as package.path")
set(LUA_CPATH "LUA_CPATH" CACHE STRING "Environment variable to use as package.cpath")
set(LUA_INIT "LUA_INIT" CACHE STRING "Environment variable for initial script")

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

if (OS_WINDOWS)
	include(LuaJIT-msvcbuild.cmake)
else()
    include(LuaJIT-main.cmake)
endif()

set(DASM_ARCH ${TARGET_LJARCH})

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

if (OS_WINDOWS)
    add_buildvm_target(lj_vm.obj peobj)
    set(LJ_VM_SRC "${CMAKE_CURRENT_BINARY_DIR}/lj_vm.obj")
else()
    add_buildvm_target(lj_vm.S ${LJVM_MODE})
    set(LJ_VM_SRC "${CMAKE_CURRENT_BINARY_DIR}/lj_vm.S")
    #set_source_files_properties("${LJ_VM_SRC}" PROPERTIES LANGUAGE CXX)
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

    if (OS_MACOS)
        target_link_options(luajit
            PRIVATE
            ${TARGET_XLDFLAGS}
        )
    endif()

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
