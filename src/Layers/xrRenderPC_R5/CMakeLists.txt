project(xrRender_R5)

list(APPEND DIRS
    "."
    "Engine"
    "Engine/Factory"
    )


add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../xrRender
    ${SDL_INCLUDE_DIRS}
    )

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

cotire(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrEngine)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION /usr/lib PERMISSIONS OWNER_READ OWNER_WRITE 
    GROUP_READ 
    WORLD_READ )

