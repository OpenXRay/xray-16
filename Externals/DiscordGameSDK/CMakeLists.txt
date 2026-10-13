add_library(DiscordGameSDK STATIC
    activity_manager.cpp
    activity_manager.h
    core.cpp
    core.h
    discord.h
    event.h
    ffi.h
    overlay_manager.cpp
    overlay_manager.h
    relationship_manager.cpp
    relationship_manager.h
    types.cpp
    types.h
    user_manager.cpp
    user_manager.h
)

target_include_directories(DiscordGameSDK
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(DiscordGameSDK
    PUBLIC
    xray_discord
)

set_target_properties(DiscordGameSDK PROPERTIES PREFIX "")
