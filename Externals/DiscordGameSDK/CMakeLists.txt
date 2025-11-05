add_library(DiscordGameSDK STATIC
    activity_manager.cpp
    core.cpp
    overlay_manager.cpp
    relationship_manager.cpp
    types.cpp
    user_manager.cpp
)

target_include_directories(DiscordGameSDK
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

if (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_discord_sdk_lib "${CMAKE_SOURCE_DIR}/sdk/libraries/x64/discord_game_sdk.dll.lib")
    else()
        set(_discord_sdk_lib "${CMAKE_SOURCE_DIR}/sdk/libraries/x86/discord_game_sdk.dll.lib")
    endif()

    target_link_libraries(DiscordGameSDK PUBLIC "${_discord_sdk_lib}")
endif()

set_target_properties(DiscordGameSDK PROPERTIES
    PREFIX ""
)
