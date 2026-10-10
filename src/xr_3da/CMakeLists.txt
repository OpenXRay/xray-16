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

if(APPLE AND NVRHI_WITH_METAL3)
    target_compile_definitions(xr_3da PRIVATE XRAY_USE_METAL)
    add_dependencies(xr_3da copy-dxcompiler copy-dxil)
    add_custom_command(TARGET xr_3da POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${slang_BINARY_DIR}/$<CONFIG>/lib/libdxcompiler.dylib"
            "${slang_BINARY_DIR}/$<CONFIG>/lib/libdxil.dylib"
            "${NVRHI_METAL_IRCONVERTER_LIBRARY}"
            "$<TARGET_FILE_DIR:xr_3da>"
        VERBATIM
    )
    install(FILES
        "${slang_BINARY_DIR}/$<CONFIG>/lib/libdxcompiler.dylib"
        "${slang_BINARY_DIR}/$<CONFIG>/lib/libdxil.dylib"
        "${NVRHI_METAL_IRCONVERTER_LIBRARY}"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

if (WIN32)
    target_link_options(xr_3da PRIVATE "LINKER:/ERRORLIMIT:0")
    add_custom_command(TARGET xr_3da POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${XRAY_SDK_BIN_DIR}/${XRAY_ANSEL_NAME}.dll"
            "${XRAY_SDK_BIN_DIR}/BugTrap.dll"
            "${XRAY_SDK_BIN_DIR}/discord_game_sdk.dll"
            "$<TARGET_FILE_DIR:xr_3da>"
        VERBATIM
    )
endif()

set_target_properties(xr_3da PROPERTIES
    PREFIX ""
)

install(TARGETS xr_3da RUNTIME
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
