project(xrSound)

set(CACHE_SRC
    "SoundRender_Cache.cpp"
    "SoundRender_Cache.h"
)

set(CORE_SRC
    "SoundRender_Core.cpp"
    "SoundRender_Core.h"
    "SoundRender_Core_Processor.cpp"
    "SoundRender_Core_SourceManager.cpp"
    "SoundRender_Core_StartStop.cpp"
)

set(CORE_OPENAL_SRC
    "OpenALDeviceList.cpp"
    "OpenALDeviceList.h"
    "SoundRender_CoreA.cpp"
    "SoundRender_CoreA.h"
)

set(EFFECTS_SRC
    "SoundRender_Effects.h"
)

set(EFFECTS_OPENAL_SRC
    "SoundRender_EffectsA_EAX.cpp"
    "SoundRender_EffectsA_EAX.h"
    "SoundRender_EffectsA_EFX.cpp"
    "SoundRender_EffectsA_EFX.h"
)

set(EMITTER_SRC
    "SoundRender_Emitter.cpp"
    "SoundRender_Emitter.h"
    "SoundRender_Emitter_FSM.cpp"
    "SoundRender_Emitter_StartStop.cpp"
    "SoundRender_Emitter_streamer.cpp"
)

set(ENVIRONMENT_SRC
    "SoundRender_Environment.cpp"
    "SoundRender_Environment.h"
)

set(KERNEL_SRC
    "guids.cpp"
    "Sound.cpp"
    "Sound.h"
    "SoundRender.h"
    "stdafx.cpp"
    "stdafx.h"
)

set(SOURCE_SRC
    "SoundRender_Source.cpp"
    "SoundRender_Source.h"
    "SoundRender_Source_loader.cpp"
)

set(TARGET_SRC
    "SoundRender_Target.cpp"
    "SoundRender_Target.h"
)

set(TARGET_OPENAL_SRC
    "SoundRender_TargetA.cpp"
    "SoundRender_TargetA.h"
)

#set(STREAMING_SRC
#    "MusicStream.cpp"
#    "MusicStream.h"
#    "xr_streamsnd.cpp"
#    "xr_streamsnd.h"
#)

#set(CDAUDIO_SRC
#    "xr_cda.cpp"
#    "xr_cda.h"
#)

source_group("Cache" FILES ${CACHE_SRC})
source_group("Core" FILES ${CORE_SRC})
source_group("Core\\OpenAL" FILES ${CORE_OPENAL_SRC})
source_group("Effects" FILES ${EFFECTS_SRC})
source_group("Effects\\OpenAL" FILES ${EFFECTS_OPENAL_SRC})
source_group("Emitter" FILES ${EMITTER_SRC})
source_group("Environment" FILES ${ENVIRONMENT_SRC})
source_group("Kernel" FILES ${KERNEL_SRC})
source_group("Source" FILES ${SOURCE_SRC})
source_group("Target" FILES ${TARGET_SRC})
source_group("Target\\OpenAL" FILES ${TARGET_OPENAL_SRC})

add_library(${PROJECT_NAME} ${LIBS_LINK_TYPE}
    ${CACHE_SRC}
    ${CORE_SRC}
    ${CORE_OPENAL_SRC}
    ${EFFECTS_SRC}
    ${EFFECTS_OPENAL_SRC}
    ${EMITTER_SRC}
    ${ENVIRONMENT_SRC}
    ${KERNEL_SRC}
    ${SOURCE_SRC}
    ${TARGET_SRC}
    ${TARGET_OPENAL_SRC}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/xrEngine
    ${CMAKE_SOURCE_DIR}/Externals/libvorbis/include
    ${CMAKE_SOURCE_DIR}/Externals/libogg/include
    ${OPENAL_INCLUDE_DIR}
    ${OGG_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
    xrMiscMath
    xrAPI
    xrCDB
    ${OPENAL_LIBRARY}
    ${OGG_LIBRARIES}
    Vorbis::Vorbis
    Vorbis::VorbisFile
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXRSOUND_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "stdafx.h"
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
