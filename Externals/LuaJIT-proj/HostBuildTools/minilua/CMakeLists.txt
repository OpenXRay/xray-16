cmake_minimum_required(VERSION 3.13)

project(minilua)

if (CMAKE_VERBOSE_MAKEFILE)
    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)
endif()

string(REPLACE " " ";" HOST_ACFLAGS "${HOST_ACFLAGS}")
string(REPLACE " " ";" HOST_ALDFLAGS "${HOST_ALDFLAGS}")

add_executable(minilua "${LUAJIT_DIR}/host/minilua.c")

target_compile_options(minilua
	PRIVATE
	${HOST_ACFLAGS}
)

target_link_libraries(minilua
	PRIVATE
	$<$<BOOL:LUA_USE_POSIX>:m>
)

target_link_options(minilua
    PRIVATE
    ${HOST_ALDFLAGS}
)
