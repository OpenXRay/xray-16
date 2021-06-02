project(xrNetServer)

set(SRC_FILES
    "ip_filter.cpp"
    "ip_filter.h"
    "NET_AuthCheck.cpp"
    "NET_AuthCheck.h"
    #"NET_Client.cpp"
    #"NET_Client.h"
    "NET_Common.cpp"
    "NET_Common.h"
    "NET_Compressor.cpp"
    "NET_Compressor.h"
    "NET_Log.cpp"
    "NET_Log.h"
    "NET_Messages.h"
    "NET_PlayersMonitor.h"
    #"NET_Server.cpp"
    #"NET_Server.h"
    "NET_Shared.h"
    "stdafx.cpp"
    "stdafx.h"
    "empty/NET_Client.cpp"
    "empty/NET_Client.h"
    "empty/NET_Server.cpp"
    "empty/NET_Server.h"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DXR_NETSERVER_EXPORTS
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "stdafx.h"
)

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
