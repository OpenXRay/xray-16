project(OPCODE)
add_dir (
    "."
    )

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${SDL_INCLUDE_DIRS}
    )

add_definitions(-DOPCODE_EXPORTS)

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore)
xr_install(${PROJECT_NAME})
