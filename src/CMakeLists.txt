add_compile_definitions(
    $<$<CONFIG:ReleaseMasterGold>:MASTER_GOLD>
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:XRAY_STATIC_BUILD>
    $<$<CONFIG:ReleaseMasterGold>:XRAY_EXCEPTIONS=0>
    $<$<NOT:$<CONFIG:ReleaseMasterGold>>:XRAY_EXCEPTIONS=1>
)

add_compile_options(${XRAY_ENABLE_WARNINGS})

add_subdirectory(utils)
add_subdirectory(Layers)
add_subdirectory(xr_3da)
add_subdirectory(xrAICore)
add_subdirectory(xrAnimation)
add_subdirectory(xrUICore)
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
