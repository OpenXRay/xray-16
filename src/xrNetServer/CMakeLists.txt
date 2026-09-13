add_library(xrNetServer)

target_sources(xrNetServer
    PRIVATE
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
)

if (WIN32)
    target_sources(xrNetServer PRIVATE
        NET_Client.cpp
        NET_Client.h
        NET_Server.cpp
        NET_Server.h
    )
else()
    target_sources(xrNetServer PRIVATE
        empty/NET_Client.cpp
        empty/NET_Client.h
        empty/NET_Server.cpp
        empty/NET_Server.h
    )
endif()

target_include_directories(xrNetServer
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(xrNetServer
    PRIVATE
    xrCore
    $<$<PLATFORM_ID:Windows>:Ws2_32>
    $<$<PLATFORM_ID:Windows>:dxguid>
)

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
