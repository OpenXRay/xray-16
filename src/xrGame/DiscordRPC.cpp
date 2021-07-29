#include "stdafx.h"
#include "DiscordRPC.h"
#include <DiscordRPC/discord_register.h>
#include <DiscordRPC/discord_rpc.h>

DiscordRPC* g_DiscordRPC;
const pcstr g_discordAppID = "869824972464488510";

void DiscordRPC::Init()
{
    DiscordEventHandlers handle{};
    Discord_Initialize(g_discordAppID, &handle, 1, nullptr);
}

void DiscordRPC::Update(pcstr level_name)
{
    DiscordRichPresence discordPresence{};
    int64_t start_time              = time(nullptr);

    discordPresence.state           = level_name;
    discordPresence.startTimestamp  = start_time;
    discordPresence.largeImageKey   = "xraylogo";
    discordPresence.largeImageText  = "OpenXRay";
    discordPresence.smallImageKey   = "xraysmalllogo";
    discordPresence.smallImageText  = level_name;

    Discord_UpdatePresence(&discordPresence);
}

void DiscordRPC::Deinit()
{
    Discord_ClearPresence();
    Discord_Shutdown();
}
