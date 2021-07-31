#pragma once

class DiscordRPC
{
    int64_t start_time{};
    string64 build_name{};
    string64 current_level_name{};
    pcstr current_task_name{};

public:
    void Init();
    void Deinit();

    void Update(pcstr level_name = nullptr);
    void SetTask(pcstr task_name);
};

extern DiscordRPC g_DiscordRPC;
