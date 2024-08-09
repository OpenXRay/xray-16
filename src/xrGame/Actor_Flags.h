#pragma once

enum
{
    AF_GODMODE = (1 << 0),
    AF_NO_CLIP = (1 << 1),
    AF_UNLIMITEDAMMO = (1 << 3),
    AF_RUN_BACKWARD = (1 << 4),
    AF_AUTOPICKUP = (1 << 5),
    AF_PSP = (1 << 6),
    AF_DYNAMIC_MUSIC = (1 << 7),
    AF_GODMODE_RT = (1 << 8),
    AF_IMPORTANT_SAVE = (1 << 9),
    AF_CROUCH_TOGGLE = (1 << 10),
    AF_MULTI_ITEM_PICKUP = (1 << 11),
    AF_LOADING_STAGES = (1 << 12),
    AF_ALWAYS_USE_ATTITUDE_SENSORS = (1 << 13), // or only when zooming if false
    AF_USE_TRACERS = (1 << 14)
};

extern Flags32 psActorFlags;
extern BOOL GodMode();

extern int psActorSleepTime;
