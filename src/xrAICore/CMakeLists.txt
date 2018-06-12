project(xrAICore)

list(APPEND DIRS
    "."
    "Components"
    "Navigation"
    "Navigation/PatrolPath"
    "Navigation/PathManagers"
    )

add_dir("${DIRS}")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/luabind
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    )

#foreach(header ${${PROJECT_NAME}__INCLUDES})
#	if (${header} MATCHES _inline.h$)
#		list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES ${header})
#	endif()
#endforeach()

add_definitions(-DXRAICORE_EXPORTS -DAI_COMPILER -D_SECURE_SCL=0)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore xrAPI xrScriptEngine luabind luajit)
