#pragma once

enum KILL_TYPE : u32
{
    KT_HIT = 0,
    KT_BLEEDING,
    KT_RADIATION,
};

enum SPECIAL_KILL_TYPE : u32
{
    SKT_NONE = 0,
    SKT_HEADSHOT,
    SKT_BACKSTAB,

    SKT_KNIFEKILL,
    SKT_PDA,

    SKT_KIR, // Kill in Row

    SKT_NEWRANK,
    SKT_EYESHOT,
};

enum KILL_RES : u32
{
    KR_NONE = 0,
    KR_SELF,
    KR_TEAMMATE,
    KR_TEAMMATE_CRITICAL,
    KR_RIVAL,
    KR_RIVAL_CRITICAL,
};
