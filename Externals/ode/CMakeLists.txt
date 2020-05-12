project(ODE)

list(APPEND DIRS
	"ode/src"
	"include/ode"
	"contrib/msvc7/ode_default"
	)

add_dir("${DIRS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_definitions(-DdSINGLE -DdNODEBUG -D_SECURE_SCL=0 -DLINUX -D_cdecl= -D__forceinline=inline)

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}__SOURCES} ${${PROJECT_NAME}__INCLUDES})

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PERMISSIONS OWNER_READ OWNER_WRITE 
    GROUP_READ 
    WORLD_READ )
