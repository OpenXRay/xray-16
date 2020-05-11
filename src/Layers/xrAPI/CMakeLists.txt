project(xrAPI)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../.. 
    )

add_definitions(-DXRAPI_EXPORTS)

add_library(${PROJECT_NAME} SHARED "xrAPI.cpp" "stdafx.h")
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PERMISSIONS OWNER_READ OWNER_WRITE 
    GROUP_READ 
    WORLD_READ )

