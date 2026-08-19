add_executable(xr_3da WIN32)

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
    xrGame
    xrRender
)

if (WIN32)
    target_link_options(xr_3da PRIVATE "LINKER:/ERRORLIMIT:0")
endif()

set_target_properties(xr_3da PROPERTIES
    PREFIX ""
)

install(TARGETS xr_3da RUNTIME
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(XRAY_USE_DLSS AND XRAY_DLSS_RUNTIME_DIR AND EXISTS "${XRAY_DLSS_RUNTIME_DIR}")
    add_custom_command(TARGET xr_3da POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${XRAY_DLSS_RUNTIME_DIR}"
            "$<TARGET_FILE_DIR:xr_3da>"
        COMMAND ${CMAKE_COMMAND} -E chdir "$<TARGET_FILE_DIR:xr_3da>"
            ${CMAKE_COMMAND} -E create_symlink libnvidia-ngx-dlss.so.310.5.3 libnvidia-ngx-dlss.so
        COMMAND ${CMAKE_COMMAND} -E chdir "$<TARGET_FILE_DIR:xr_3da>"
            ${CMAKE_COMMAND} -E create_symlink libnvidia-ngx-dlssd.so.310.5.3 libnvidia-ngx-dlssd.so
        COMMAND ${CMAKE_COMMAND} -E chdir "$<TARGET_FILE_DIR:xr_3da>"
            ${CMAKE_COMMAND} -E create_symlink libnvidia-ngx-dlssg.so.310.5.3 libnvidia-ngx-dlssg.so
        COMMENT "Copy DLSS runtime libraries next to xr_3da")
endif()

