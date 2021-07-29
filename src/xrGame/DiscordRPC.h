#pragma once

#include <DiscordRPC/discord_register.h>
#include <DiscordRPC/discord_rpc.h>
#include <Windows.h>

class DiscordRPC
{
public:
    DiscordRPC() = default;
    ~DiscordRPC();

    void Init();
    void Update(const char* level_name);
};

extern DiscordRPC* g_DiscordRPC;
