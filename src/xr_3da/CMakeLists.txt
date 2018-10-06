project(xr_3da)

list(APPEND DIRS
    "."
    )

add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/ode/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    ${SDL_INCLUDE_DIRS}
    )

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/./StickyKeyFilter.hpp")

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrAPI xrEngine)
xr_install(${PROJECT_NAME})
if(UNIX) # Yes, we have shell
    install(FILES xr_3da.sh DESTINATION "."
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)
endif()
