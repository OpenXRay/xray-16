cmake_minimum_required(VERSION 3.13)

project(minilua)

string(REPLACE " " ";" HOST_ACFLAGS "${HOST_ACFLAGS}")

add_executable(minilua "${LUAJIT_DIR}/host/minilua.c")

target_compile_options(minilua
	PRIVATE
	${HOST_ACFLAGS}
)

target_link_libraries(minilua
	PRIVATE
	$<$<BOOL:LUA_USE_POSIX>:m>
)
