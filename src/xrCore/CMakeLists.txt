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
execute_process(COMMAND git rev-parse --verify HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_SHA1
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)

message( "commit: " ${GIT_SHA1})

execute_process(COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_BRANCH
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)

message( "branch: " ${GIT_BRANCH} )

add_definitions(-DXRCORE_EXPORTS -DGIT_INFO_CURRENT_COMMIT=${GIT_SHA1} -DGIT_INFO_CURRENT_BRANCH=${GIT_BRANCH})
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h")
cotire(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_link_libraries(${PROJECT_NAME} xrMiscMath dl pthread ${SDL_LIBRARIES} ${LZO_LIBRARIES} ${CRYPTO++_LIBRARIES} ${PUGIXML_LIBRARY} ${TBB_LIBRARIES})
xr_install(${PROJECT_NAME})
