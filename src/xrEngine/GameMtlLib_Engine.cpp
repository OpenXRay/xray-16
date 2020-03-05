#include "stdafx.h"
#include "GameMtlLib.h"

#ifndef GM_NON_GAME

static void DestroySounds(SoundVec& lst)
{
    for (auto it : lst)
        it.destroy();
}

static void DestroyPSs(PSVec& lst) {}
static void CreateSounds(SoundVec& lst, pcstr buf)
{
    string128 tmp;
    int cnt = _GetItemCount(buf);
    R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT + 2);
    lst.resize(cnt);
    for (int k = 0; k < cnt; ++k)
        lst[k].create(_GetItem(buf, k, tmp), st_Effect, sg_SourceType);
}

static void CreateMarks(IWallMarkArray* pMarks, pcstr buf)
{
    string256 tmp;
    int cnt = _GetItemCount(buf);
    R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT);
    for (int k = 0; k < cnt; ++k)
        pMarks->AppendMark(_GetItem(buf, k, tmp));
}

static void CreatePSs(PSVec& lst, pcstr buf)
{
    string256 tmp;
    int cnt = _GetItemCount(buf);
    R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT);
    for (int k = 0; k < cnt; ++k)
        lst.push_back(_GetItem(buf, k, tmp));
}
#endif

SGameMtlPair::~SGameMtlPair()
{
#ifndef GM_NON_GAME
    // destroy all media
    DestroySounds(BreakingSounds);
    DestroySounds(StepSounds);
    DestroySounds(CollideSounds);
    DestroyPSs(CollideParticles);
#endif
}

void SGameMtlPair::Load(IReader& fs)
{
    shared_str buf;
    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_PAIR));
    mtl0 = fs.r_u32();
    mtl1 = fs.r_u32();
    ID = fs.r_u32();
    ID_parent = fs.r_u32();
    u32 own_mask = fs.r_u32();
#ifdef GM_NON_GAME
    if (GAMEMTL_NONE_ID == ID_parent)
        OwnProps.one();
    else
        OwnProps.assign(own_mask);
#else
    OwnProps.assign(own_mask);
#endif
    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_BREAKING));
    fs.r_stringZ(buf);
#ifdef GM_NON_GAME
    BreakingSounds = *buf ? *buf : "";
#else
    CreateSounds(BreakingSounds, *buf);
#endif
    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_STEP));
    fs.r_stringZ(buf);
#ifdef GM_NON_GAME
    StepSounds = *buf ? *buf : "";
#else
    CreateSounds(StepSounds, *buf);
#endif
    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_COLLIDE));
    fs.r_stringZ(buf);
#ifdef GM_NON_GAME
    CollideSounds = *buf ? *buf : "";
#else
    CreateSounds(CollideSounds, *buf);
#endif
    fs.r_stringZ(buf);
#ifdef GM_NON_GAME
    CollideParticles = *buf ? *buf : "";
#else
    CreatePSs(CollideParticles, *buf);
#endif
    fs.r_stringZ(buf);
#ifdef GM_NON_GAME
    CollideMarks = *buf ? *buf : "";
#else
    CreateMarks(&*CollideMarks, *buf);
#endif
}
