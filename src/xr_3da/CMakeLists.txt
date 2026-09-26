add_executable(xr_3da)

target_sources(xr_3da PRIVATE
    entry_point.cpp
    stdafx.h

    resource.h
    resource.rc

    splash_oxr.bmp
    stalker_cop.ico
    stalker_cs.ico
    stalker_soc.ico
)

set_property(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    PROPERTY
    VS_STARTUP_PROJECT xr_3da
)

target_include_directories(xr_3da
    PRIVATE
    "${XRAY_SOURCE_DIR}/src"
)

target_link_libraries(xr_3da
    PRIVATE
    xrCore
    xrAPI
    xrEngine
    xrRender_GL
    xrGame
)

set_target_properties(xr_3da PROPERTIES
    PREFIX ""
)

if (EMSCRIPTEN)
    target_link_options(xr_3da PRIVATE ${XRAY_WEB_EXECUTABLE_LINK_OPTIONS})
    set_property(TARGET xr_3da APPEND PROPERTY LINK_DEPENDS "${XRAY_SOURCE_DIR}/misc/web/pre.js")
    add_custom_target(web_dist ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${XRAY_SOURCE_DIR}/misc/web/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:xr_3da>/xr_3da.js"
            "$<TARGET_FILE_DIR:xr_3da>/xr_3da.wasm"
            "${XRAY_SOURCE_DIR}/misc/web/dist"
        COMMAND node "${XRAY_SOURCE_DIR}/misc/web/tools/build_dist.mjs"
        VERBATIM
    )
    add_dependencies(web_dist xr_3da)
endif()

install(TARGETS xr_3da RUNTIME
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
