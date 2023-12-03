add_library(xrImGui STATIC)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/Externals/imgui")

target_sources(xrImGui PRIVATE
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

target_include_directories(xrImGui
    PUBLIC
    "${IMGUI_DIR}"
)

target_link_libraries(xrImGui
    PRIVATE
    dl
)

target_compile_definitions(xrImGui
    PRIVATE
    IMGUI_EXPORTS
)

set_target_properties(xrImGui PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)
