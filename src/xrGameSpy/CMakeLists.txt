project(xrGameSpy)

list(APPEND DIRS
    "."
    )

add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrServerEntities
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/cximage
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/GameSpy/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/luabind
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/ode/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/OpenAutomate/inc
    ${SDL_INCLUDE_DIRS}
    )

#list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./atlas_stalkercs_v1.c")

#list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./atlas_stalkercs_v1.h")

add_definitions(-DXRGAMESPY_EXPORTS -DXRAY_DISABLE_GAMESPY_WARNINGS)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "stdafx.h")
cotire(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrMiscMath GameSpy ${SDL_LIBRARIES})
xr_install(${PROJECT_NAME})
