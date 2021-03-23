project(xrSound)

set(SRC_FILES
    "guids.cpp"
    #"MusicStream.cpp"
    #"MusicStream.h"
    "OpenALDeviceList.cpp"
    "OpenALDeviceList.h"
    "Sound.cpp"
    "Sound.h"
    "SoundRender_Cache.cpp"
    "SoundRender_Cache.h"
    "SoundRender_CoreA.cpp"
    "SoundRender_CoreA.h"
    "SoundRender_Core.cpp"
    "SoundRender_CoreD.cpp"
    "SoundRender_CoreD.h"
    "SoundRender_Core.h"
    "SoundRender_Core_Processor.cpp"
    "SoundRender_Core_SourceManager.cpp"
    "SoundRender_Core_StartStop.cpp"
    "SoundRender_Emitter.cpp"
    "SoundRender_Emitter_FSM.cpp"
    "SoundRender_Emitter.h"
    "SoundRender_Emitter_StartStop.cpp"
    "SoundRender_Emitter_streamer.cpp"
    "SoundRender_Environment.cpp"
    "SoundRender_Environment.h"
    "SoundRender.h"
    "SoundRender_Source.cpp"
    "SoundRender_Source.h"
    "SoundRender_Source_loader.cpp"
    "SoundRender_TargetA.cpp"
    "SoundRender_TargetA.h"
    "SoundRender_Target.cpp"
    "SoundRender_TargetD.cpp"
    "SoundRender_TargetD.h"
    "SoundRender_Target.h"
    "stdafx.cpp"
    "stdafx.h"
    #"xr_cda.cpp"
    #"xr_cda.h"
    #"xr_streamsnd.cpp"
    #"xr_streamsnd.h"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/xrEngine
    ${CMAKE_SOURCE_DIR}/Externals/libvorbis/include
    ${CMAKE_SOURCE_DIR}/Externals/libogg/include
    ${CMAKE_SOURCE_DIR}/Externals/openal/include
    ${OGG_INCLUDE_DIRS}
    ${SDL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
    xrMiscMath
    xrAPI
    xrCDB
    ${OGG_LIBRARIES}
    openal
    vorbis
    vorbisfile
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
