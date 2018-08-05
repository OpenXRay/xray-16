project(xrPhysics)

list(APPEND DIRS
	"."
	"tri-colliderknoopc"
	"dcylinder"
	)

add_dir("${DIRS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/.. 
	${CMAKE_CURRENT_SOURCE_DIR}/../../Externals/ode/include
	${CMAKE_CURRENT_SOURCE_DIR}/../../sdk/include
    /usr/include/SDL2
	)

list(REMOVE_ITEM ${PROJECT_NAME}__SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tri-colliderknoopc/dcTriListCollider.cpp")
#list(REMOVE_ITEM ${PROJECT_NAME}__INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/./xr_cda.h")

add_definitions(-DXRPHYSICS_EXPORTS -DdSINGLE -D_SECURE_SCL=0)
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} xrCore ODE xrScriptEngine xrAPI xrCDB xrSound)
