project(xrSound)

list(APPEND DIRS
    "."
    )

add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrEngine
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/libvorbis/include
#	${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/libogg/include
    ${OGG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/openal/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    ${SDL_INCLUDE_DIRS}
    )

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./xr_cda.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./xr_streamsnd.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./MusicStream.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./xr_cda.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./xr_streamsnd.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./MusicStream.h")

add_definitions(-DXRSOUND_EXPORTS -DdSINGLE -D_SECURE_SCL=0)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h")
cotire(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrMiscMath xrAPI xrCDB ${OGG_LIBRARIES} openal vorbis vorbisfile)
xr_install(${PROJECT_NAME})

