#pragma once

class DiscordRPC
{
public:
    DiscordRPC(){};
    ~DiscordRPC(){};

    void Init();
    void Update(pcstr level_name);
    void Deinit();
};

extern DiscordRPC* g_DiscordRPC;
