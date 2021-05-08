#pragma once

class R_sync_point
{
    void* q_sync_point[CHWCaps::MAX_GPUS]{};
    u32 q_sync_count;

public:
    R_sync_point() = default;

    void Create();
    void Destroy();

    bool Wait(u32 wait_sleep, u64 timeout);
    void End();
};
