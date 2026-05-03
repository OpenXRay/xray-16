add_compile_definitions(
    $<$<CONFIG:ReleaseMasterGold>:MASTER_GOLD>
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:XRAY_STATIC_BUILD>
    $<$<CONFIG:ReleaseMasterGold>:XRAY_EXCEPTIONS=0>
    $<$<NOT:$<CONFIG:ReleaseMasterGold>>:XRAY_EXCEPTIONS=1>
    $<$<CONFIG:ReleaseMasterGold>:_HAS_EXCEPTIONS=0>
    $<$<CONFIG:ReleaseMasterGold>:dNODEBUG>
    PURE_DYNAMIC_CAST
)

add_compile_options(${XRAY_ENABLE_WARNINGS})

if(MSVC)
    function(xray_set_per_target_pdb dir)
        get_property(_subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
        foreach(_sub IN LISTS _subdirs)
            xray_set_per_target_pdb(${_sub})
        endforeach()
        get_property(_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
        foreach(_tgt IN LISTS _targets)
            get_target_property(_type ${_tgt} TYPE)
            if(NOT _type STREQUAL "INTERFACE_LIBRARY" AND NOT _type STREQUAL "UTILITY")
                set_target_properties(${_tgt} PROPERTIES
                    COMPILE_PDB_NAME ${_tgt}
                    COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pdb/$<CONFIG>
                    PDB_NAME ${_tgt}
                    PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pdb/$<CONFIG>)
            endif()
        endforeach()
    endfunction()
endif()

add_subdirectory(utils)
add_subdirectory(Layers)
add_subdirectory(xr_3da)
add_subdirectory(xrAICore)
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

if(MSVC)
    xray_set_per_target_pdb(${CMAKE_CURRENT_SOURCE_DIR})
endif()
