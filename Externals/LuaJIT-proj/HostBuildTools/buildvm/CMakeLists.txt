cmake_minimum_required(VERSION 3.13)

project(buildvm)

if (CMAKE_VERBOSE_MAKEFILE)
    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)
endif()

separate_arguments(HOST_ACFLAGS)
separate_arguments(HOST_ALDFLAGS)
separate_arguments(BUILDVM_SRC NATIVE_COMMAND ${BUILDVM_SRC})
separate_arguments(BUILDVM_ARCH)

add_executable(buildvm)

target_sources(buildvm PRIVATE
	"${BUILDVM_SRC}"
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
)

target_link_options(buildvm
    PRIVATE
    ${HOST_ALDFLAGS}
)
