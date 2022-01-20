#pragma once

class DiscordRPC final
{
    int64_t start_time{};
    string64 build_name{};
    string256 current_level_name{};
    string256 current_task_name{};

public:
    enum DiscordStatusType
    {
        UpdateLevel,
        UpdateTask,
    };

    void Init();

    void Update(DiscordStatusType updateType, pcstr updateData);
};

extern DiscordRPC g_DiscordRPC;
