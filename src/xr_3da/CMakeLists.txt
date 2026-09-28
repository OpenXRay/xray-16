if (ANDROID AND XRAY_ANDROID_SHARED)
    add_library(xr_3da SHARED)
else()
    add_executable(xr_3da)
endif()

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
    "${CMAKE_SOURCE_DIR}/sdk/include"
)

target_link_libraries(xr_3da
    PRIVATE
    xrCore
    xrAPI
    xrEngine
    xrRender_GL
    xrGame
)

if (ANDROID AND XRAY_ANDROID_SHARED)
    # SDLActivity loads libmain.so and resolves the C-linkage SDL_main symbol.
    target_compile_definitions(xr_3da PRIVATE XRAY_ANDROID_SHARED)
endif()

set_target_properties(xr_3da PROPERTIES
    PREFIX ""
)

if (ANDROID AND XRAY_ANDROID_SHARED)
    set_target_properties(xr_3da PROPERTIES
        PREFIX "lib"
        OUTPUT_NAME main
    )
endif()

install(TARGETS xr_3da
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
