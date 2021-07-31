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

void DiscordRPC::Update(pcstr level_name)
{
    DiscordRichPresence discordPresence{};

    discordPresence.startTimestamp  = start_time;
    discordPresence.smallImageText  = build_name;
    discordPresence.largeImageKey   = "current_level_name";
    discordPresence.smallImageKey   = "build_name";

    if (level_name)
    {
        std::locale myLocale("");
        xr_string temp = StringToUTF8(level_name, myLocale);
        xr_sprintf(current_level_name, "%s", temp);
    }

    discordPresence.state           = current_level_name;
    discordPresence.largeImageText  = current_level_name;
    discordPresence.details         = current_task_name;

    Discord_UpdatePresence(&discordPresence);
}

void DiscordRPC::SetTask(pcstr task_name)
{
    std::locale myLocale("");
    xr_string temp = StringToUTF8(task_name, myLocale);
    current_task_name = temp.c_str();
    Update();
}
