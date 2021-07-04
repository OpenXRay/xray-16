# Copyright (C) 2007-2013 LuaDist.
# Created by Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

cmake_minimum_required(VERSION 3.13)

project(xrLuajit C CXX ASM)

# Version
set(MAJVER 2)
set(MINVER 0)
set(RELVER 5)
set(ABIVER 5.1)
set(NODOTABIVER 51)

set(LUAJIT_DIR ${CMAKE_SOURCE_DIR}/Externals/LuaJIT/src CACHE PATH "Location of luajit sources")

option(BUILD_STATIC_LIB "Build static library" OFF)
option(BUILD_DYNAMIC_LIB "Build dynamic library" ON)
option(BUILD_LIB_ONLY "Build library only" ON)

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

option(LUAJIT_DEBUG "Generate debug information" OFF)

if (WIN32)
	option(LUA_BUILD_WLUA "Build wluajit interpreter for no-console applications." ON)
elseif (APPLE)
	option(LUA_USE_POSIX "Use POSIX functionality." ON)
	option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
else()
	option(LUA_USE_POSIX "Use POSIX functionality." ON)
	option(LUA_USE_DLOPEN "Use dynamic linker to load modules." ON)
endif()

# TODO: check if we need luaconf.h
# TODO: add other variables from luaconf.h if we need them
# Configuration for luaconf.h
set(LUA_PATH "LUA_PATH" CACHE STRING "Environment variable to use as package.path")
set(LUA_CPATH "LUA_CPATH" CACHE STRING "Environment variable to use as package.cpath")
set(LUA_INIT "LUA_INIT" CACHE STRING "Environment variable for initial script")

# Clean unnecessary files in LuaJIT source directory
execute_process(
	COMMAND ${CMAKE_MAKE_PROGRAM} clean
	WORKING_DIRECTORY ${LUAJIT_DIR}
)

# Compiler options
if (PROJECT_PLATFORM_E2K) # E2K: O3 on mcst-lcc approximately equal to O2 at gcc X86/ARM
	set(CCOPT_OPT_LEVEL "-O3")
else()
	set(CCOPT_OPT_LEVEL "-O2")
endif()

set(CCOPT "${CCOPT_OPT_LEVEL} -fomit-frame-pointer -fno-stack-protector")

# Target-specific compiler options
set(CCOPT_x86 "-march=i686 -msse -msse2 -mfpmath=sse")
set(CCOPT_x64 "")
set(CCOPT_arm "")
set(CCOPT_arm64 "")
set(CCOPT_ppc "")
set(CCOPT_mips "")

if (CCDEBUG)
	set(LUAJIT_DEBUG "-g")
endif()

set(CCWARN "-Wall")
#string(APPEND CCWARN "-Wextra -Wdeclaration-after-statement -Wredundant-decls -Wshadow -Wpointer-arith")

if (LUAJIT_DISABLE_FFI)
	string(APPEND XCFLAGS " -DLUAJIT_DISABLE_FFI")
endif()

if (LUAJIT_ENABLE_LUA52COMPAT)
	string(APPEND XCFLAGS " -DLUAJIT_ENABLE_LUA52COMPAT")
endif()

if (LUAJIT_DISABLE_JIT)
	string(APPEND XCFLAGS " -DLUAJIT_DISABLE_JIT")
endif()

if (LUAJIT_DISABLE_GC64)
	string(APPEND XCFLAGS " -DLUAJIT_DISABLE_GC64")
endif()

if (LUAJIT_USE_SYSMALLOC)
	string(APPEND XCFLAGS " -DLUAJIT_USE_SYSMALLOC")
endif()

if (LUAJIT_USE_VALGRIND)
	string(APPEND XCFLAGS " -DLUAJIT_USE_VALGRIND")
endif()

if (LUAJIT_USE_GDBJIT)
	string(APPEND XCFLAGS " -DLUAJIT_USE_GDBJIT")
endif()

if (LUA_USE_APICHECK)
	string(APPEND XCFLAGS " -DLUA_USE_APICHECK")
endif()

if (LUA_USE_ASSERT)
	string(APPEND XCFLAGS " -DLUA_USE_ASSERT")
endif()

set(ASOPTIONS ${CCOPT} ${CCWARN}${XCFLAGS})
set(CCOPTIONS ${CCDEBUG} ${ASOPTIONS})

set(TESTARCH_C_FLAGS ${CMAKE_C_FLAGS})
string(REPLACE " " ";" TESTARCH_C_FLAGS "${TESTARCH_C_FLAGS}")

set(TESTARCH_FLAGS "${TESTARCH_C_FLAGS} ${CCOPTIONS} -E lj_arch.h -dM")
string(REPLACE " " ";" TESTARCH_FLAGS "${TESTARCH_FLAGS}")

execute_process(
	COMMAND ${CMAKE_C_COMPILER} ${TESTARCH_FLAGS}
	WORKING_DIRECTORY ${LUAJIT_DIR}
	OUTPUT_VARIABLE TARGET_TESTARCH
)

if ("${TARGET_TESTARCH}" MATCHES "LJ_TARGET_X64")
	set(TARGET_LJARCH "x64")
elseif ("${TARGET_TESTARCH}" MATCHES "LJ_TARGET_X86")
	set(TARGET_LJARCH "x86")
elseif ("${TARGET_TESTARCH}" MATCHES "LJ_TARGET_ARM\t")
	set(TARGET_LJARCH "arm")
elseif ("${TARGET_TESTARCH}" MATCHES "LJ_TARGET_ARM64")
	set(TARGET_LJARCH "arm64")
	if ("${TARGET_TESTARCH}" MATCHES "__AARCH64EB__")
		set(TARGET_ARCH "-D__AARCH64EB__=1")
	endif()
elseif ("${TARGET_TESTARCH}" MATCHES "LJ_TARGET_PPC")
	set(TARGET_LJARCH "ppc")
	if ("${TARGET_TESTARCH}" MATCHES "LJ_LE 1")
		set(TARGET_ARCH "-DLJ_ARCH_ENDIAN=LUAJIT_LE")
	else()
		set(TARGET_ARCH "-DLJ_ARCH_ENDIAN=LUAJIT_BE")
	endif()
elseif ("${TARGET_TESTARCH}" MATCHES "LJ_TARGET_MIPS\t")
	if ("${TARGET_TESTARCH}" MATCHES "MIPSEL")
		set(TARGET_ARCH "-D__MIPSEL__=1")
	endif()
	if ("${TARGET_TESTARCH}" MATCHES "LJ_TARGET_MIPS64")
		set(TARGET_LJARCH "mips64")
	else()
		set(TARGET_LJARCH "mips")
	endif()
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "e2k") # MCST Elbrus 2000
	set(TARGET_LJARCH "e2k")
else()
	message(FATAL_ERROR "Unsupported luajit target architecture")
endif()

string(APPEND TARGET_ARCH "-DLUAJIT_TARGET=LUAJIT_ARCH_${TARGET_LJARCH}")
string(APPEND TARGET_XCFLAGS "${CCOPT_${TARGET_LJARCH}}")

# TODO: add PREFIX, TARGET_XCFLAGS, TARGET_DYNXLDOPTS, MULTILIB here (lines 289-302 in Makefile)

# TODO: use TARGET_STRIP flags?

if (WIN32)
	#string(APPEND TARGET_STRIP "--strip-unneeded")

	set(TARGET_DLLDOTANAME "libluajit-${ABIVER}.dll.a")
	string(APPEND TARGET_XSHLDFLAGS " -shared -Wl,--out-implib,${TARGET_DLLDOTANAME}")

	if (BUILD_DYNAMIC_LIB)
		string(APPEND HOST_XCFLAGS " -DLUA_BUILD_AS_DLL")
	endif()

	set(LJVM_MODE peobj)
elseif (APPLE)
	if (${MACOSX_DEPLOYMENT_TARGET} STREQUAL "")
		message(FATAL_ERROR "Missing export MACOSX_DEPLOYMENT_TARGET=XX.YY")
	endif()

	#string(APPEND TARGET_STRIP "-x")

	string(APPEND TARGET_XSHLDFLAGS " -dynamiclib -single_module -undefined dynamic_lookup -fPIC")
	string(APPEND TARGET_XSHLDFLAGS " -install_name ${TARGET_DYLIBPATH} -compatibility_version ${MAJVER}.${MINVER} -current_version ${MAJVER}.${MINVER}.${RELVER}")

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

if ("${TARGET_TESTARCH}" MATCHES "LJ_LE 1")
	string(APPEND DASM_FLAGS "-D ENDIAN_LE")
else()
	string(APPEND DASM_FLAGS "-D ENDIAN_BE")
endif()

if ("${TARGET_TESTARCH}" MATCHES "LJ_ARCH_BITS 64")
	string(APPEND DASM_FLAGS " -D P64")
endif()

if ("${TARGET_TESTARCH}" MATCHES "LJ_HASJIT 1")
	string(APPEND DASM_FLAGS " -D JIT")
endif()

if ("${TARGET_TESTARCH}" MATCHES "LJ_HASFFI 1")
	string(APPEND DASM_FLAGS " -D FFI")
endif()

if ("${TARGET_TESTARCH}" MATCHES "LJ_DUALNUM 1")
	string(APPEND DASM_FLAGS " -D DUALNUM")
endif()

if ("${TARGET_TESTARCH}" MATCHES "LJ_ARCH_HASFPU 1")
	string(APPEND DASM_FLAGS " -D FPU")
	string(APPEND TARGET_ARCH " -DLJ_ARCH_HASFPU=1")
else()
	string(APPEND TARGET_ARCH " -DLJ_ARCH_HASFPU=0")
endif()

if ("${TARGET_TESTARCH}" MATCHES "LJ_ABI_SOFTFP 1")
	string(APPEND TARGET_ARCH " -DLJ_ABI_SOFTFP=1")
else()
	string(APPEND DASM_FLAGS " -D HFABI")
	string(APPEND TARGET_ARCH " -DLJ_ABI_SOFTFP=0")
endif()

if ("${TARGET_TESTARCH}" MATCHES "LJ_NO_UNWIND 1")
	string(APPEND DASM_FLAGS " -D NO_UNWIND")
	string(APPEND TARGET_ARCH " -DLUAJIT_NO_UNWIND")
endif()

if ("${TARGET_TESTARCH}" MATCHES "LJ_ARCH_VERSION")
	string(REGEX MATCH "LJ_ARCH_VERSION\t\t([0-9]+)$" _ "${TARGET_TESTARCH}")
	string(APPEND DASM_FLAGS " -D VER=${CMAKE_MATCH_1}")
else()
	string(APPEND DASM_FLAGS " -D VER=")
endif()

set(DASM_ARCH ${TARGET_LJARCH})

if (WIN32)
	string(APPEND DASM_FLAGS " -D WIN")
endif()

if (TARGET_LJARCH STREQUAL "x64")
	if (NOT "${TARGET_TESTARCH}" MATCHES "LJ_FR2 1")
		set(DASM_ARCH "x86")
	endif()
elseif (TARGET_LJARCH STREQUAL "arm")
	if (APPLE)
		string(APPEND DASM_FLAGS " -D IOS")
	endif()
elseif ("${TARGET_TESTARCH}" MATCHES "LJ_TARGET_MIPSR6")
	string(APPEND DASM_FLAGS " -D MIPSR6")
endif()

if (TARGET_LJARCH STREQUAL "ppc")
	if ("${TARGET_TESTARCH}" MATCHES "LJ_ARCH_SQRT 1")
		string(APPEND DASM_FLAGS " -D SQRT")
	endif()
	if ("${TARGET_TESTARCH}" MATCHES "LJ_ARCH_ROUND 1")
		string(APPEND DASM_FLAGS " -D ROUND")
	endif()
	if ("${TARGET_TESTARCH}" MATCHES "LJ_ARCH_PPC32ON64 1")
		string(APPEND DASM_FLAGS " -D GPR64")
	endif()
endif()

set(HOST_ACFLAGS "${CMAKE_C_FLAGS} ${CCOPTIONS} ${TARGET_ARCH}")
set(HOST_ALDFLAGS "${CMAKE_C_FLAGS}")

string(APPEND TARGET_XCFLAGS " -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -U_FORTIFY_SOURCE")

string(REPLACE " " ";" CCOPTIONS "${CCOPTIONS}")
string(REPLACE " " ";" HOST_ACFLAGS "${HOST_ACFLAGS}")
string(REPLACE " " ";" HOST_ALDFLAGS "${HOST_ALDFLAGS}")
string(REPLACE " " ";" DASM_FLAGS "${DASM_FLAGS}")
string(REPLACE " " ";" TARGET_XCFLAGS "${TARGET_XCFLAGS}")

set(DASM_DASC ${LUAJIT_DIR}/vm_${DASM_ARCH}.dasc)
set(DASM ${LUAJIT_DIR}/../dynasm/dynasm.lua)

if (PROJECT_PLATFORM_E2K)
	set(BUILDVM_ARCH "${LUAJIT_DIR}/host/buildvm_arch.h")
else()
	set(BUILDVM_ARCH "${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h")
endif()

# Generate buildvm arch header
if (NOT PROJECT_PLATFORM_E2K)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/minilua/minilua"
		COMMAND ${CMAKE_COMMAND}
			-B"HostBuildTools/minilua"
			-H"${CMAKE_CURRENT_SOURCE_DIR}/HostBuildTools/minilua"
			-DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
			-DCMAKE_BUILD_TYPE:STRING="Release"
			-DLUAJIT_DIR="${LUAJIT_DIR}"
			-DHOST_ACFLAGS="${HOST_ACFLAGS}"
			-DHOST_ALDFLAGS="${HOST_ALDFLAGS}"
		COMMAND ${CMAKE_COMMAND} --build HostBuildTools/minilua --config Release
	)

	add_custom_command(OUTPUT ${BUILDVM_ARCH}
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/minilua/minilua ${DASM} ${DASM_FLAGS} -o ${BUILDVM_ARCH} ${DASM_DASC}
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/minilua/minilua"
	)

	add_custom_target(buildvm_arch
		DEPENDS ${BUILDVM_ARCH}
	)
endif()

# Buildvm
set(BUILDVM_SRC
	"${LUAJIT_DIR}/host/buildvm_asm.c"
	"${LUAJIT_DIR}/host/buildvm_fold.c"
	"${LUAJIT_DIR}/host/buildvm_lib.c"
	#"${LUAJIT_DIR}/host/buildvm_libbc.h"
	"${LUAJIT_DIR}/host/buildvm_peobj.c"
	"${LUAJIT_DIR}/host/buildvm.c"
	"${LUAJIT_DIR}/host/buildvm.h"
)

group_sources(BUILDVM_SRC)

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/buildvm/buildvm"
	COMMAND ${CMAKE_COMMAND}
		-B"HostBuildTools/buildvm"
		-H"${CMAKE_CURRENT_SOURCE_DIR}/HostBuildTools/buildvm"
		-DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
		-DCMAKE_BUILD_TYPE:STRING="Release"
		-DLUAJIT_DIR="${LUAJIT_DIR}"
		-DBUILDVM_SRC="${BUILDVM_SRC}"
		-DBUILDVM_ARCH="${BUILDVM_ARCH}"
		-DHOST_ACFLAGS="${HOST_ACFLAGS}"
		-DHOST_ALDFLAGS="${HOST_ALDFLAGS}"
	COMMAND ${CMAKE_COMMAND} --build HostBuildTools/buildvm --config Release
)

add_custom_target(buildvm
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/buildvm/buildvm"
)

if (NOT PROJECT_PLATFORM_E2K)
	add_dependencies(buildvm buildvm_arch)
endif()

set(LJLIB_C
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
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}
		COMMAND ${CMAKE_CURRENT_BINARY_DIR}/HostBuildTools/buildvm/buildvm ARGS -m ${mode} -o ${CMAKE_CURRENT_BINARY_DIR}/${target} ${ARGN}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS buildvm ${ARGN}
	)
endmacro(add_buildvm_target)

if (WIN32)
	add_buildvm_target(lj_vm.obj peobj)
	set(LJ_VM_SRC "${CMAKE_CURRENT_BINARY_DIR}/lj_vm.obj")
else()
	add_buildvm_target(lj_vm.S ${LJVM_MODE})
	set(LJ_VM_SRC "${CMAKE_CURRENT_BINARY_DIR}/lj_vm.S")
	set_source_files_properties(${LJ_VM_SRC} PROPERTIES LANGUAGE CXX)
endif()

add_buildvm_target("lj_bcdef.h" "bcdef" ${LJLIB_C})
add_buildvm_target("lj_ffdef.h" "ffdef" ${LJLIB_C})
add_buildvm_target("lj_libdef.h" "libdef" ${LJLIB_C})
add_buildvm_target("lj_recdef.h" "recdef" ${LJLIB_C})
add_buildvm_target("lj_folddef.h" "folddef" "${LUAJIT_DIR}/lj_opt_fold.c")
add_buildvm_target("jit/vmdef.lua" "libvm" ${LJLIB_C})

SET(DEPS
	${LJ_VM_SRC}
	"${CMAKE_CURRENT_BINARY_DIR}/lj_ffdef.h"
	"${CMAKE_CURRENT_BINARY_DIR}/lj_bcdef.h"
	"${CMAKE_CURRENT_BINARY_DIR}/lj_libdef.h"
	"${CMAKE_CURRENT_BINARY_DIR}/lj_recdef.h"
	"${CMAKE_CURRENT_BINARY_DIR}/lj_folddef.h"
)

group_sources(DEPS)

# TODO: use lj_asm_ lj_emit lj_target?

set(LJCORE_C
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

group_sources(LJCORE_C)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(LIB_NAME ${PROJECT_NAME}-debug)
else()
	set(LIB_NAME ${PROJECT_NAME})
endif()

# TODO: check windows supports same target name for static and shared lib
if (BUILD_STATIC_LIB)
	add_library(${LIB_NAME} STATIC
		${LJCORE_C}
		${DEPS}
	)
	target_link_libraries(${LIB_NAME} ${LIBS})
endif()

if (BUILD_DYNAMIC_LIB)
	add_library(${LIB_NAME} SHARED
		${LJCORE_C}
		${DEPS}
	)
endif()

target_include_directories(${LIB_NAME}
	PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${LIB_NAME}
	PRIVATE
	$<$<BOOL:LUA_USE_POSIX>:m>
	$<$<BOOL:LUA_USE_DLOPEN>:dl>
)

target_compile_options(${LIB_NAME}
	PRIVATE
	${CCOPTIONS}
	${TARGET_XCFLAGS}
)

target_link_options(${LIB_NAME}
	PRIVATE
	${TARGET_XSHLDFLAGS}
)

set_target_properties(${LIB_NAME} PROPERTIES
	PREFIX ""
)

install(TARGETS ${LIB_NAME} LIBRARY
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

if (NOT ${BUILD_LIB_ONLY})
	add_executable(luajit
		"${LUAJIT_DIR}/luajit.c"
	)

	target_link_libraries(luajit
		PRIVATE
		${LIB_NAME}
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
			${LIB_NAME}
		)
	endif()
endif()
