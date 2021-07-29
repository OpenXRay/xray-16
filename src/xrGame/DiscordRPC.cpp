#include "stdafx.h"
#include "DiscordRPC.h"
#include <ctime>

DiscordRPC* g_DiscordRPC;

void DiscordRPC::Init()
{
    DiscordEventHandlers Handle;
    memset(&Handle, 0, sizeof(Handle));
    Discord_Initialize("869824972464488510", &Handle, 1, NULL);
}

DiscordRPC::~DiscordRPC()
{
    Discord_Shutdown();
    Discord_ClearPresence();
}

void DiscordRPC::Update(const char* level_name)
{
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    int64_t start_time              = time(nullptr);

    discordPresence.state           = level_name;
    discordPresence.startTimestamp  = start_time;
    discordPresence.largeImageKey   = "xraylogo";
    discordPresence.largeImageText  = "OpenXRay";
    discordPresence.smallImageKey   = "xraysmalllogo";
    discordPresence.smallImageText  = level_name;

    Discord_UpdatePresence(&discordPresence);
}
