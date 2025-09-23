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
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(xr_3da
    PRIVATE
    xrCore
    xrAPI
    xrEngine
    xrRender_GL
    xrGame
)

if (WIN32)
    target_link_libraries(xr_3da
        PRIVATE
        xrRenderPC_R4
    )

    set_target_properties(xr_3da PROPERTIES
        WIN32_EXECUTABLE ON
    )

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_xr_runtime_dir "${CMAKE_SOURCE_DIR}/sdk/binaries/x64")
    else()
        set(_xr_runtime_dir "${CMAKE_SOURCE_DIR}/sdk/binaries/x86")
    endif()

    add_custom_command(TARGET xr_3da POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_xr_runtime_dir}"
            "$<TARGET_FILE_DIR:xr_3da>"
        COMMENT "Copying Windows runtime dependencies"
        VERBATIM
    )
endif()

set_target_properties(xr_3da PROPERTIES
    PREFIX ""
)

install(TARGETS xr_3da RUNTIME
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
