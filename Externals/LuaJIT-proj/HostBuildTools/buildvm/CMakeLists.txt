cmake_minimum_required(VERSION 3.13)

project(buildvm)

if (CMAKE_VERBOSE_MAKEFILE)
    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)
endif()

string(REPLACE " " ";" HOST_ACFLAGS "${HOST_ACFLAGS}")
string(REPLACE " " ";" HOST_ALDFLAGS "${HOST_ALDFLAGS}")
string(REPLACE " " ";" BUILDVM_SRC "${BUILDVM_SRC}")
string(REPLACE " " ";" BUILDVM_ARCH "${BUILDVM_ARCH}")

add_executable(buildvm ${BUILDVM_SRC} ${BUILDVM_ARCH})

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
