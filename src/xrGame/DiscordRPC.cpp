#include "StdAfx.h"

#include "DiscordRPC.h"
#include "xrCore/Text/StringConversion.hpp"

#include <DiscordGameSDK/discord.h>

constexpr discord::ClientId DISCORD_APP_ID = 869824972464488510;

discord::Core* g_discordCore{};
DiscordRPC g_DiscordRPC;

void DiscordRPC::Init()
{
    xr_sprintf(build_name, "build %u", Core.GetBuildId());
    start_time = time(nullptr);

    discord::Core::Create(DISCORD_APP_ID, DiscordCreateFlags_Default, &g_discordCore);
}

void DiscordRPC::Update(DiscordStatusType updateType, pcstr updateData)
{
    discord::Activity discordActivity{};
    xr_string tempData = StringToUTF8(updateData, std::locale(""));
    pcstr updateName = tempData.c_str();

    if (updateType == DiscordStatusType::UpdateLevel)
        xr_strcpy(current_level_name, updateName);
    else if (updateType == DiscordStatusType::UpdateTask)
        xr_strcpy(current_task_name, updateName);

    discordActivity.GetTimestamps().SetStart(start_time);
    discordActivity.GetAssets().SetLargeImage("current_level_name");
    discordActivity.GetAssets().SetSmallImage("build_name");
    discordActivity.GetAssets().SetLargeText(current_level_name);
    discordActivity.GetAssets().SetSmallText(build_name);
    discordActivity.SetState(current_level_name);
    discordActivity.SetDetails(current_task_name);

    g_discordCore->ActivityManager().UpdateActivity(discordActivity, nullptr);
    g_discordCore->RunCallbacks();
}
