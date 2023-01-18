project(xrImGui)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/Externals/imgui)

set(KERNEL_SRC
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h"
)

add_library(${PROJECT_NAME} STATIC ${KERNEL_SRC})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${IMGUI_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    dl
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DIMGUI_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)
