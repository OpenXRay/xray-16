cmake_minimum_required(VERSION 3.13)

project(buildvm)

string(REPLACE " " ";" HOST_ACFLAGS "${HOST_ACFLAGS}")
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
