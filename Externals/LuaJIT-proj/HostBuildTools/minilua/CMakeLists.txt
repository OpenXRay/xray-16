cmake_minimum_required(VERSION 3.22)

project(minilua C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_VERBOSE_MAKEFILE)
    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)
endif()

separate_arguments(HOST_ACFLAGS)
separate_arguments(HOST_ALDFLAGS)

add_executable(minilua)

target_sources(minilua PRIVATE
	"${LUAJIT_DIR}/host/minilua.c"
)

target_compile_options(minilua
	PRIVATE
	${HOST_ACFLAGS}
)

target_link_libraries(minilua
	PRIVATE
	$<$<BOOL:${LUA_USE_POSIX}>:m>
)

target_link_options(minilua
    PRIVATE
    ${HOST_ALDFLAGS}
)
