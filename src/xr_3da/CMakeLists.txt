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
    "${XRAY_SOURCE_DIR}/src"
)

target_link_libraries(xr_3da
    PRIVATE
    xrCore
    xrAPI
    xrEngine
    xrGame
    xrRender
)

add_custom_command(TARGET xr_3da POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        ${XRAY_SLANG_RUNTIME_FILES}
        "$<TARGET_FILE_DIR:xr_3da>"
    VERBATIM
)

if(APPLE AND NVRHI_WITH_METAL3)
    target_compile_definitions(xr_3da PRIVATE XRAY_USE_METAL)
    add_custom_command(TARGET xr_3da POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${NVRHI_METAL_IRCONVERTER_LIBRARY}"
            "$<TARGET_FILE_DIR:xr_3da>"
        VERBATIM
    )
    install(FILES
        "${XRAY_SLANG_ROOT}/lib/libdxcompiler.dylib"
        "${XRAY_SLANG_ROOT}/lib/libdxil.dylib"
        "${NVRHI_METAL_IRCONVERTER_LIBRARY}"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

if (WIN32)
    target_link_options(xr_3da PRIVATE "LINKER:/ERRORLIMIT:0")
    set(XRAY_WINDOWS_RUNTIME_DLLS
        "${XRAY_SDK_BIN_DIR}/${XRAY_ANSEL_NAME}.dll"
        "${XRAY_SDK_BIN_DIR}/BugTrap.dll"
        "${XRAY_SDK_BIN_DIR}/discord_game_sdk.dll"
        "${XRAY_SDK_BIN_DIR}/OpenAL32.dll"
        "${XRAY_SDK_BIN_DIR}/soft_oal.dll"
    )
    add_custom_command(TARGET xr_3da POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${XRAY_WINDOWS_RUNTIME_DLLS}
            "$<TARGET_FILE_DIR:xr_3da>"
        VERBATIM
    )
    install(FILES ${XRAY_WINDOWS_RUNTIME_DLLS}
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

set_target_properties(xr_3da PROPERTIES
    PREFIX ""
)

install(TARGETS xr_3da RUNTIME
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
