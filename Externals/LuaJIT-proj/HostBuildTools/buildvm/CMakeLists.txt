cmake_minimum_required(VERSION 3.22)

project(buildvm C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_VERBOSE_MAKEFILE)
    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)
endif()

separate_arguments(HOST_ACFLAGS)
separate_arguments(HOST_ALDFLAGS)

add_executable(buildvm)

target_sources(buildvm PRIVATE
    "${LUAJIT_DIR}/host/buildvm_asm.c"
	"${LUAJIT_DIR}/host/buildvm_fold.c"
	"${LUAJIT_DIR}/host/buildvm_lib.c"
	#"${LUAJIT_DIR}/host/buildvm_libbc.h"
	"${LUAJIT_DIR}/host/buildvm_peobj.c"
	"${LUAJIT_DIR}/host/buildvm.c"
	"${LUAJIT_DIR}/host/buildvm.h"
	"${BUILDVM_ARCH}"
)

target_include_directories(buildvm
	PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}/../..
	${LUAJIT_DIR}
)

target_compile_options(buildvm
	PRIVATE
	${HOST_ACFLAGS}
    -Wno-discarded-qualifiers
)

target_link_options(buildvm
    PRIVATE
    ${HOST_ALDFLAGS}
)
