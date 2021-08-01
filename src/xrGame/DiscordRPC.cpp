#include "stdafx.h"

#include "DiscordRPC.h"
#include "xrCore/Text/StringConversion.hpp"

#include <DiscordRPC/discord_register.h>
#include <DiscordRPC/discord_rpc.h>

constexpr pcstr DISCORD_APP_ID = "869824972464488510";

DiscordRPC g_DiscordRPC;

void DiscordRPC::Init()
{
    xr_sprintf(build_name, "build %u", Core.GetBuildId());
    start_time = time(nullptr);
    DiscordEventHandlers handle{};
    Discord_Initialize(DISCORD_APP_ID, &handle, 1, nullptr);
}

void DiscordRPC::Deinit()
{
    Discord_ClearPresence();
    Discord_Shutdown();
}

void DiscordRPC::Update(DiscordStatusType updateType, pcstr updateData)
{
    DiscordRichPresence discordPresence{};
    static std::locale locale("");
    xr_string tempData = StringToUTF8(updateData, locale);

    if (updateType == DiscordRPC::DiscordStatusType::UPDATE_LEVEL)
        xr_strcpy(current_level_name, tempData.c_str());
    else if (updateType == DiscordRPC::DiscordStatusType::UPDATE_TASK)
        xr_strcpy(current_task_name, tempData.c_str());

    discordPresence.startTimestamp  = start_time;
    discordPresence.largeImageKey   = "current_level_name";
    discordPresence.smallImageKey   = "build_name";
    discordPresence.largeImageText  = current_level_name;
    discordPresence.smallImageText  = build_name;
    discordPresence.state           = current_level_name;
    discordPresence.details         = current_task_name;

    Discord_UpdatePresence(&discordPresence);
}
