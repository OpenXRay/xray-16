project(xrCore)

list(APPEND DIRS
    "."
    "Animation"
    "Compression"
    "Containers"
    "Crypto"
    "Math"
    "Media"
    "PostProcess"
    "Text"
    "Threading"
    "XML"
    )

add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/pugixml/src
    #${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    ${SDL_INCLUDE_DIRS}
    ${TBB_INCLUDE_DIRS}
)

add_definitions(-DXRCORE_EXPORTS)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_link_libraries(${PROJECT_NAME} xrMiscMath ${SDL_LIBRARIES} ${LZO_LIBRARIES} ${CRYPTO++_LIBRARIES} ${PUGIXML_LIBRARY} ${TBB_LIBRARIES})
