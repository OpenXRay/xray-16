project(xrAPI)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../.. 
    )

add_definitions(-DXRAPI_EXPORTS)

add_library(${PROJECT_NAME} SHARED "xrAPI.cpp" "stdafx.h")
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
xr_install(${PROJECT_NAME})

