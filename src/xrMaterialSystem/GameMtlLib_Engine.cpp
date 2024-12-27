#include "stdafx.h"
#include "GameMtlLib.h"

namespace
{
void DestroySounds(xr_vector<ref_sound>& lst)
{
    for (auto it : lst)
        it.destroy();
}

void DestroyPSs(xr_vector<shared_str>& lst) {}
void CreateSounds(xr_vector<ref_sound>& lst, pcstr buf)
{
    string128 tmp;
    const int cnt = _GetItemCount(buf);
    R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT + 2);
    lst.resize(cnt);
    for (int k = 0; k < cnt; ++k)
        lst[k].create(_GetItem(buf, k, tmp), st_Effect, sg_SourceType);
}

void CreateMarks(IWallMarkArray* pMarks, pcstr buf)
{
    string256 tmp;
    const int cnt = _GetItemCount(buf);
    R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT);
    for (int k = 0; k < cnt; ++k)
        pMarks->AppendMark(_GetItem(buf, k, tmp));
}

void CreatePSs(xr_vector<shared_str>& lst, pcstr buf)
{
    string256 tmp;
    const int cnt = _GetItemCount(buf);
    R_ASSERT(cnt <= GAMEMTL_SUBITEM_COUNT);
    for (int k = 0; k < cnt; ++k)
        lst.emplace_back(_GetItem(buf, k, tmp));
}
}

SGameMtlPair::~SGameMtlPair()
{
    // destroy all media
    DestroySounds(BreakingSounds);
    DestroySounds(StepSounds);
    DestroySounds(CollideSounds);
    DestroyPSs(CollideParticles);
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
    // XXX: recheck below when we will be integrating EDITOR
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
    CreateSounds(BreakingSounds, *buf);

    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_STEP));
    fs.r_stringZ(buf);
    CreateSounds(StepSounds, *buf);

    R_ASSERT(fs.find_chunk(GAMEMTLPAIR_CHUNK_COLLIDE));
    fs.r_stringZ(buf);
    CreateSounds(CollideSounds, *buf);

    fs.r_stringZ(buf);
    CreatePSs(CollideParticles, *buf);

    fs.r_stringZ(buf);
    CreateMarks(&*CollideMarks, *buf);
}
