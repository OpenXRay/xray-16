#discord_game_sdk.dll.lib

add_library(discord_game_sdk_discord_game_sdk STATIC)

add_library(DiscordGameSDK::DiscordGameSDK ALIAS discord_game_sdk_discord_game_sdk)

target_sources_grouped(
    TARGET discord_game_sdk_discord_game_sdk
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
    TARGET discord_game_sdk_discord_game_sdk
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

target_include_directories(discord_game_sdk_discord_game_sdk
    PUBLIC
    #${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Externals
)

#find_library(GAME_SDK discord_game_sdk)

target_link_libraries(discord_game_sdk_discord_game_sdk
    PUBLIC
    #${GAME_SDK}
    ${CMAKE_SOURCE_DIR}/sdk/libraries/x64/discord_game_sdk.dll.lib
)

target_link_directories(discord_game_sdk_discord_game_sdk
    PRIVATE
    #${CMAKE_SOURCE_DIR}/sdk/binaries/${ARCH_TYPE}
)

install(
    TARGETS discord_game_sdk_discord_game_sdk LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
