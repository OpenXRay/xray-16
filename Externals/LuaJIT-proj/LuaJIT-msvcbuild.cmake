# Generate buildvm arch header
set(DASM_DASC "${LUAJIT_DIR}/vm_x86.dasc")
set(DASM "${LUAJIT_DIR}/../dynasm/dynasm.lua")
set(BUILDVM_ARCH "${LUAJIT_DIR}/host/buildvm_arch.h")

add_executable(minilua "${LUAJIT_DIR}/host/minilua.c")

target_compile_options(minilua
    PRIVATE
    #/O2 move to Debug
    /W3
    /D
)

target_compile_definitions(minilua
    PRIVATE
    _CRT_SECURE_NO_DEPRECATE
    "_CRT_STDIO_INLINE=__declspec(dllexport)__inline"
)

add_custom_command(OUTPUT ${BUILDVM_ARCH}
    COMMAND $<TARGET_FILE:minilua> ${DASM} -LN -o ${BUILDVM_ARCH} ${DASM_DASC}
    DEPENDS minilua
)

add_custom_target(buildvm_arch
    DEPENDS ${BUILDVM_ARCH}
)

set_target_properties(buildvm_arch PROPERTIES
        ADDITIONAL_CLEAN_FILES "${MINILUA_BINARY_DIR}"
)

# buildvm
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
	"${CMAKE_CURRENT_BINARY_DIR}/../.."
	"${LUAJIT_DIR}"
)

add_dependencies(buildvm buildvm_arch)

set(BUILDVM_FILE "$<TARGET_FILE:buildvm>")
