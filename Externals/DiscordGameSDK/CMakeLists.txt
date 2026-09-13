add_library(DiscordGameSDK STATIC)

target_sources_grouped(
    TARGET DiscordGameSDK
    NAME "Managers"
    FILES
    activity_manager.cpp
    activity_manager.h
    overlay_manager.cpp
    overlay_manager.h
    relationship_manager.cpp
    relationship_manager.h
    user_manager.cpp
    user_manager.h
)

target_sources_grouped(
    TARGET DiscordGameSDK
    NAME "Core"
    FILES
    core.cpp
    core.h
    discord.h
    event.h
    ffi.h
    #pch.hpp
    types.cpp
    types.h
)

target_include_directories(DiscordGameSDK
    PUBLIC
    "${CMAKE_SOURCE_DIR}/Externals"
)

target_link_libraries(DiscordGameSDK
    PUBLIC
    ${XRAY_SDK_LIBRARY_DIR}/discord_game_sdk.dll.lib
)

set_target_properties(DiscordGameSDK PROPERTIES
    PREFIX ""
)
