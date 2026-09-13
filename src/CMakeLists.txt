add_compile_definitions(
    $<$<CONFIG:ReleaseMasterGold>:MASTER_GOLD>
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:XRAY_STATIC_BUILD>
    $<$<CONFIG:ReleaseMasterGold>:XRAY_EXCEPTIONS=0>
    $<$<NOT:$<CONFIG:ReleaseMasterGold>>:XRAY_EXCEPTIONS=1>
)

add_compile_options(${XRAY_ENABLE_WARNINGS})

add_subdirectory(Layers)
add_subdirectory(utils)
add_subdirectory(xrAICore)
add_subdirectory(xrCDB)
add_subdirectory(xrCore)
add_subdirectory(xrEngine)
add_subdirectory(xrGame)
add_subdirectory(xrGameSpy)
add_subdirectory(xrMaterialSystem)
add_subdirectory(xrNetServer)
add_subdirectory(xrParticles)
add_subdirectory(xrPhysics)
add_subdirectory(xrScriptEngine)
add_subdirectory(xrSound)
add_subdirectory(xrUICore)
add_subdirectory(xr_3da)

install(
    TARGETS
    xr_3da
    xrAICore
    xrAPI
    xrCDB
    xrCore
    xrEngine
    xrGame
    xrGameSpy
    xrLuabind
    xrMaterialSystem
    xrNetServer
    xrODE
    xrOPCODE
    xrParticles
    xrPhysics
    xrRender_GL
    $<$<PLATFORM_ID:Windows>:xrRender_R4>
    xrScriptEngine
    xrSound
    xrUICore
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT OpenXRay
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT OpenXRay
)

install(
    FILES
    $<TARGET_PDB_FILE:xr_3da>
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Debug
    OPTIONAL
)

if (BUILD_SHARED_LIBS)
    install(
        FILES
        $<TARGET_PDB_FILE:xrAICore>
        $<TARGET_PDB_FILE:xrAPI>
        $<TARGET_PDB_FILE:xrCDB>
        $<TARGET_PDB_FILE:xrCore>
        $<TARGET_PDB_FILE:xrEngine>
        $<TARGET_PDB_FILE:xrGame>
        $<TARGET_PDB_FILE:xrGameSpy>
        $<TARGET_PDB_FILE:xrLuabind>
        $<TARGET_PDB_FILE:xrMaterialSystem>
        $<TARGET_PDB_FILE:xrNetServer>
        $<TARGET_PDB_FILE:xrODE>
        $<TARGET_PDB_FILE:xrOPCODE>
        $<TARGET_PDB_FILE:xrParticles>
        $<TARGET_PDB_FILE:xrPhysics>
        $<TARGET_PDB_FILE:xrRender_GL>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PDB_FILE:xrRender_R4>>
        $<TARGET_PDB_FILE:xrScriptEngine>
        $<TARGET_PDB_FILE:xrSound>
        $<TARGET_PDB_FILE:xrUICore>
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Debug
        OPTIONAL
    )
endif()
