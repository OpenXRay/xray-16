add_library(xrSound SHARED)

target_sources_grouped(
    TARGET xrSound
    NAME "Core"
    FILES
    SoundRender_Core.cpp
    SoundRender_Core.h
    SoundRender_Core_Processor.cpp
    SoundRender_Core_SourceManager.cpp
    SoundRender_Core_StartStop.cpp
)

target_sources_grouped(
    TARGET xrSound
    NAME "Core\\OpenAL"
    FILES
    OpenALDeviceList.cpp
    OpenALDeviceList.h
    SoundRender_CoreA.cpp
    SoundRender_CoreA.h
)

target_sources_grouped(
    TARGET xrSound
    NAME "Emitter"
    FILES
    SoundRender_Emitter.cpp
    SoundRender_Emitter.h
    SoundRender_Emitter_FSM.cpp
    SoundRender_Emitter_StartStop.cpp
)

target_sources_grouped(
    TARGET xrSound
    NAME "Scene"
    FILES
    SoundRender_Scene.cpp
    SoundRender_Scene.h
)

target_sources_grouped(
    TARGET xrSound
    NAME "Kernel"
    FILES
    Sound.cpp
    Sound.h
    SoundRender.h
    stdafx.cpp
    stdafx.h
)

target_sources_grouped(
    TARGET xrSound
    NAME "Source"
    FILES
    SoundRender_Source.cpp
    SoundRender_Source.h
)

target_sources_grouped(
    TARGET xrSound
    NAME "Target"
    FILES
    SoundRender_Target.cpp
    SoundRender_Target.h
)

target_sources_grouped(
    TARGET xrSound
    NAME "Target\\OpenAL"
    FILES
    SoundRender_TargetA.cpp
    SoundRender_TargetA.h
)

#target_sources_grouped(
#    TARGET xrSound
#    NAME "Stream"
#    FILES
#    MusicStream.cpp
#    MusicStream.h
#    xr_streamsnd.cpp
#    xr_streamsnd.h
#)

#target_sources_grouped(
#    TARGET xrSound
#    NAME "CDA"
#    FILES
#    xr_cda.cpp
#    xr_cda.h
#)

target_include_directories(xrSound
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/xrEngine"
    "${OPENAL_INCLUDE_DIR}"
)

target_link_libraries(xrSound
    PRIVATE
    xrCore
    xrMiscMath
    xrAPI
    xrCDB
    xrMaterialSystem
    ${OPENAL_LIBRARY}
    Ogg::Ogg
    Vorbis::Vorbis
    Vorbis::VorbisFile
)

target_compile_definitions(xrSound
    PRIVATE
    XRSOUND_EXPORTS
)

set_target_properties(xrSound PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrSound
    PRIVATE
    stdafx.h
)

install(TARGETS xrSound LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
