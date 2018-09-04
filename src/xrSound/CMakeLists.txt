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
	)

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./xr_cda.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./MusicStream.cpp")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./xr_cda.h")
list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./MusicStream.h")

add_definitions(-DXRSOUND_EXPORTS -DdSINGLE -D_SECURE_SCL=0)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrAPI xrCDB ${OGG_LIBRARIES})
