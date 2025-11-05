add_library(xrNetServer)

set(XR_NETSERVER_PLATFORM_SOURCES
    empty/NET_Client.cpp
    empty/NET_Client.h
    empty/NET_Server.cpp
    empty/NET_Server.h
)

if (WIN32)
    set(XR_NETSERVER_PLATFORM_SOURCES
        NET_Client.cpp
        NET_Client.h
        NET_Server.cpp
        NET_Server.h
    )
endif()

target_sources(xrNetServer PRIVATE
    guids.cpp
    ip_filter.cpp
    ip_filter.h
    NET_AuthCheck.cpp
    NET_AuthCheck.h
    NET_Common.cpp
    NET_Common.h
    NET_Compressor.cpp
    NET_Compressor.h
    NET_Log.cpp
    NET_Log.h
    NET_Messages.h
    NET_PlayersMonitor.h
    NET_Shared.h
    stdafx.cpp
    stdafx.h
    ${XR_NETSERVER_PLATFORM_SOURCES}
)

target_include_directories(xrNetServer
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(xrNetServer
    PRIVATE
    xrCore
)

if (WIN32)
    target_link_libraries(xrNetServer
        PRIVATE
        Ws2_32
        dxguid
    )
endif()

target_compile_definitions(xrNetServer
    PRIVATE
    XR_NETSERVER_EXPORTS
)

set_target_properties(xrNetServer PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrNetServer
    PRIVATE
    stdafx.h
)

if (BUILD_SHARED_LIBS)
    install(TARGETS xrNetServer LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
