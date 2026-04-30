add_library(xrNetServer)

target_sources(xrNetServer PRIVATE
    BaseClient.cpp
    BaseClient.h
    BaseServer.cpp
    BaseServer.h
    ClientConnectionOptions.h
    ServerConnectionOptions.h
    IBannedClient.cpp
    IBannedClient.h
    IClient.cpp
    IClient.h
    IClientStatistic.cpp
    IClientStatistic.h
    INetQueue.cpp
    INetQueue.h
    SteamNetClient.cpp
    SteamNetClient.h
    SteamNetServer.cpp
    SteamNetServer.h
    PlayersMonitor.h
    ip_address.cpp
    ip_address.h
    ip_filter.cpp
    ip_filter.h
    NET_AuthCheck.cpp
    NET_AuthCheck.h
    NET_Client.h
    NET_Common.cpp
    NET_Common.h
    NET_Compressor.cpp
    NET_Compressor.h
    NET_Log.cpp
    NET_Log.h
    NET_Messages.h
    NET_Server.h
    NET_Shared.h
    stdafx.cpp
    stdafx.h
)

if (WIN32)
    target_sources(xrNetServer PRIVATE
        WinsocksHelper.cpp
        WinsocksHelper.h
    )
endif()

target_include_directories(xrNetServer
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
)

target_include_directories(xrNetServer
    PUBLIC
    "${CMAKE_SOURCE_DIR}/Externals/GameNetworkingSockets/include"
)

target_link_libraries(xrNetServer
    PRIVATE
    xrCore
    GameNetworkingSockets_s
)

if (WIN32)
    target_link_libraries(xrNetServer PRIVATE Ws2_32)
endif()

target_compile_definitions(xrNetServer
    PRIVATE
    XR_NETSERVER_EXPORTS
    STEAMNETWORKINGSOCKETS_STATIC_LINK
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
