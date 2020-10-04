//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SkeletonAnimated.h"

#include "AnimationKeyCalculate.h"
#include "SkeletonX.h"
#include "xrCore/FMesh.hpp"
#include "xrCore/xr_token.h"
#ifdef DEBUG
#include "xrCore/dump_string.h"
#endif
extern int psSkeletonUpdate;
using namespace animation;

//////////////////////////////////////////////////////////////////////////
// BoneInstance methods
void CBlendInstance::construct() { Blend.clear(); }
void CBlendInstance::blend_add(CBlend* H)
{
    if (Blend.size() == MAX_BLENDED)
    {
        if (H->fall_at_end)
            return;
        BlendSVecIt _d = Blend.begin();
        for (BlendSVecIt it = Blend.begin() + 1; it != Blend.end(); it++)
            if ((*it)->blendAmount < (*_d)->blendAmount)
                _d = it;
        Blend.erase(_d);
    }
    VERIFY(Blend.size() < MAX_BLENDED);
    Blend.push_back(H);
}
void CBlendInstance::blend_remove(CBlend* H)
{
    CBlend** I = std::find(Blend.begin(), Blend.end(), H);
    if (I != Blend.end())
        Blend.erase(I);
}

// Motion control
void CKinematicsAnimated::Bone_Motion_Start(CBoneData* bd, CBlend* handle)
{
    LL_GetBlendInstance(bd->GetSelfID()).blend_add(handle);
    for (auto &it : bd->children)
        Bone_Motion_Start(it, handle);
}
void CKinematicsAnimated::Bone_Motion_Stop(CBoneData* bd, CBlend* handle)
{
    LL_GetBlendInstance(bd->GetSelfID()).blend_remove(handle);
    for (auto &it : bd->children)
        Bone_Motion_Stop(it, handle);
}
void CKinematicsAnimated::Bone_Motion_Start_IM(CBoneData* bd, CBlend* handle)
{
    LL_GetBlendInstance(bd->GetSelfID()).blend_add(handle);
}
void CKinematicsAnimated::Bone_Motion_Stop_IM(CBoneData* bd, CBlend* handle)
{
    LL_GetBlendInstance(bd->GetSelfID()).blend_remove(handle);
}

#if (defined DEBUG || defined _EDITOR)

std::pair<LPCSTR, LPCSTR> CKinematicsAnimated::LL_MotionDefName_dbg(MotionID ID)
{
    shared_motions& s_mots = m_Motions[ID.slot].motions;
    for (auto &it : *s_mots.motion_map())
        if (it.second == ID.idx)
            return std::make_pair(*it.first, *s_mots.id());
    return std::make_pair((LPCSTR)nullptr, (LPCSTR)nullptr);
}

static LPCSTR name_bool(BOOL v)
{
    static const xr_token token_bool[] = {{"false", 0}, {"true", 1}};
    return get_token_name(token_bool, v);
}

static LPCSTR name_blend_type(CBlend::ECurvature blend)
{
    static const xr_token token_blend[] = {{"eFREE_SLOT", CBlend::eFREE_SLOT}, {"eAccrue", CBlend::eAccrue},
        {"eFalloff", CBlend::eFalloff}, {"eFORCEDWORD", CBlend::eFORCEDWORD}};
    return get_token_name(token_blend, blend);
}

static void dump_blend(CKinematicsAnimated* K, CBlend& B, u32 index)
{
    VERIFY(K);
    Msg("----------------------------------------------------------");
    Msg("blend index: %d, poiter: %p ", index, &B);
    Msg("time total: %f, speed: %f , power: %f ", B.timeTotal, B.speed, B.blendPower);
    Msg("ammount: %f, time current: %f, frame %d ", B.blendAmount, B.timeCurrent, B.dwFrame);
    Msg("accrue: %f, fallof: %f ", B.blendAccrue, B.blendFalloff);

    Msg("bonepart: %d, channel: %d, stop_at_end: %s, fall_at_end: %s ", B.bone_or_part, B.channel,
        name_bool(B.stop_at_end), name_bool(B.fall_at_end));
    Msg("state: %s, playing: %s, stop_at_end_callback: %s ", name_blend_type(B.blend_state()), name_bool(B.playing),
        name_bool(B.stop_at_end_callback));
    Msg("callback: %p callback param: %p", B.Callback, B.CallbackParam);

    if (B.blend_state() != CBlend::eFREE_SLOT)
    {
        Msg("motion : name %s, set: %s ", K->LL_MotionDefName_dbg(B.motionID).first,
            K->LL_MotionDefName_dbg(B.motionID).second);
    }
    Msg("----------------------------------------------------------");
}

void CKinematicsAnimated::LL_DumpBlends_dbg()
{
    Msg("==================dump blends=================================================");
    for (auto &it : blend_pool)
        dump_blend(this, it, u32(&it - blend_pool.begin()));
}

#endif

u32 CKinematicsAnimated::LL_PartBlendsCount(u32 bone_part_id) { return blend_cycle(bone_part_id).size(); }
CBlend* CKinematicsAnimated::LL_PartBlend(u32 bone_part_id, u32 n)
{
    if (LL_PartBlendsCount(bone_part_id) <= n)
        return nullptr;
    return blend_cycle(bone_part_id)[n];
}
void CKinematicsAnimated::LL_IterateBlends(IterateBlendsCallback& callback)
{
    for (auto &it : blend_pool)
        if (it.blend_state() != CBlend::eFREE_SLOT)
            callback(it);
}
/*
LPCSTR CKinematicsAnimated::LL_MotionDefName_dbg	(LPVOID ptr)
{
//.
    // cycles
    mdef::const_iterator I,E;
    I = motions.cycle()->begin();
    E = motions.cycle()->end();
    for ( ; I != E; ++I) if (&(*I).second == ptr) return *(*I).first;
    // fxs
    I = motions.fx()->begin();
    E = motions.fx()->end();
    for ( ; I != E; ++I) if (&(*I).second == ptr) return *(*I).first;
    return 0;
}
*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
MotionID CKinematicsAnimated::LL_MotionID(LPCSTR B)
{
    MotionID motion_ID;
    for (int k = int(m_Motions.size()) - 1; k >= 0; --k)
    {
        shared_motions* s_mots = &m_Motions[k].motions;
        accel_map::iterator I = s_mots->motion_map()->find(pstr(B));
        if (I != s_mots->motion_map()->end())
        {
            motion_ID.set(u16(k), I->second);
            break;
        }
    }
    return motion_ID;
}
u16 CKinematicsAnimated::LL_PartID(LPCSTR B)
{
    if (nullptr == m_Partition)
        return BI_NONE;
    for (u16 id = 0; id < MAX_PARTS; id++)
    {
        CPartDef& P = (*m_Partition)[id];
        if (nullptr == P.Name)
            continue;
        if (0 == xr_stricmp(B, *P.Name))
            return id;
    }
    return BI_NONE;
}

// cycles
MotionID CKinematicsAnimated::ID_Cycle_Safe(LPCSTR N)
{
    MotionID motion_ID;
    for (int k = int(m_Motions.size()) - 1; k >= 0; --k)
    {
        shared_motions* s_mots = &m_Motions[k].motions;
        accel_map::const_iterator I = s_mots->cycle()->find(pstr(N));
        if (I != s_mots->cycle()->end())
        {
            motion_ID.set(u16(k), I->second);
            break;
        }
    }
    return motion_ID;
}
MotionID CKinematicsAnimated::ID_Cycle(shared_str N)
{
    MotionID motion_ID = ID_Cycle_Safe(N);
    R_ASSERT3(motion_ID.valid(), "! MODEL: can't find cycle: ", N.c_str());
    return motion_ID;
}
MotionID CKinematicsAnimated::ID_Cycle_Safe(shared_str N)
{
    MotionID motion_ID;
    for (int k = int(m_Motions.size()) - 1; k >= 0; --k)
    {
        shared_motions* s_mots = &m_Motions[k].motions;
        accel_map::const_iterator I = s_mots->cycle()->find(N);
        if (I != s_mots->cycle()->end())
        {
            motion_ID.set(u16(k), I->second);
            break;
        }
    }
    return motion_ID;
}
MotionID CKinematicsAnimated::ID_Cycle(LPCSTR N)
{
    MotionID motion_ID = ID_Cycle_Safe(N);
    R_ASSERT3(motion_ID.valid(), "! MODEL: can't find cycle: ", N);
    return motion_ID;
}
void CKinematicsAnimated::LL_FadeCycle(u16 part, float falloff, u8 mask_channel /*= (1<<0)*/)
{
    BlendSVec& Blend = blend_cycles[part];

    for (u32 I = 0; I < Blend.size(); I++)
    {
        CBlend& B = *Blend[I];
        if (!(mask_channel & (1 << B.channel)))
            continue;
        // B.blend				= CBlend::eFalloff;
        B.set_falloff_state();
        B.blendFalloff = falloff;
        // B.blendAccrue		= B.timeCurrent;
        if (B.stop_at_end)
            B.stop_at_end_callback = FALSE; // callback не должен приходить!
    }
}
void CKinematicsAnimated::LL_CloseCycle(u16 part, u8 mask_channel /*= (1<<0)*/)
{
    if (BI_NONE == part)
        return;
    if (part >= MAX_PARTS)
        return;

    // destroy cycle(s)
    BlendSVecIt I = blend_cycles[part].begin(), E = blend_cycles[part].end();
    for (; I != E; I++)
    {
        CBlend& B = *(*I);
        if (!(mask_channel & (1 << B.channel)))
            continue;
        // B.blend = CBlend::eFREE_SLOT;
        B.set_free_state();

        CPartDef& P = (*m_Partition)[B.bone_or_part];
        for (u32 i = 0; i < P.bones.size(); i++)
            Bone_Motion_Stop_IM((*bones)[P.bones[i]], *I);

        blend_cycles[part].erase(I); // ?
        E = blend_cycles[part].end();
        I--;
    }
    // blend_cycles[part].clear	(); // ?
}

float CKinematicsAnimated::get_animation_length(MotionID motion_ID)
{
    VERIFY(motion_ID.slot < m_Motions.size());

    SMotionsSlot& slot = m_Motions[motion_ID.slot];

    VERIFY(LL_GetBoneRoot() < slot.bone_motions.size());

    MotionVec* bone_motions = slot.bone_motions[LL_GetBoneRoot()];

    VERIFY(motion_ID.idx < bone_motions->size());

    CMotionDef* const m_def = slot.motions.motion_def(motion_ID.idx);

    float const anim_speed = m_def ? m_def->Speed() : 1.f;

    return bone_motions->at(motion_ID.idx).GetLength() / anim_speed;
}

void CKinematicsAnimated::IBlendSetup(CBlend& B, u16 part, u8 channel, MotionID motion_ID, BOOL bMixing,
    float blendAccrue, float /*blendFalloff*/, float Speed, BOOL noloop, PlayCallback Callback, LPVOID CallbackParam)
{
    VERIFY(B.channel < MAX_CHANNELS);
    // Setup blend params
    if (bMixing)
    {
        // B.blend		= CBlend::eAccrue;
        B.set_accrue_state();
        B.blendAmount = EPS_S;
    }
    else
    {
        // B.blend		= CBlend::eFixed;
        // B.blend		= CBlend::eAccrue;
        B.set_accrue_state();
        B.blendAmount = 1;
    }
    B.blendAccrue = blendAccrue;
    B.blendFalloff = 0; // blendFalloff used for previous cycles
    B.blendPower = 1;
    B.speed = Speed;
    B.motionID = motion_ID;
    B.timeCurrent = 0;
    B.timeTotal = m_Motions[B.motionID.slot].bone_motions[LL_GetBoneRoot()]->at(motion_ID.idx).GetLength();
    B.bone_or_part = part;
    B.stop_at_end = noloop;
    B.playing = TRUE;
    B.stop_at_end_callback = TRUE;
    B.Callback = Callback;
    B.CallbackParam = CallbackParam;

    B.channel = channel;
    B.fall_at_end = B.stop_at_end && (channel > 1);
}
void CKinematicsAnimated::IFXBlendSetup(
    CBlend& B, MotionID motion_ID, float blendAccrue, float blendFalloff, float Power, float Speed, u16 bone)
{
    // B.blend			= CBlend::eAccrue;
    B.set_accrue_state();
    B.blendAmount = EPS_S;
    B.blendAccrue = blendAccrue;
    B.blendFalloff = blendFalloff;
    B.blendPower = Power;
    B.speed = Speed;
    B.motionID = motion_ID;
    B.timeCurrent = 0;
    B.timeTotal = m_Motions[B.motionID.slot].bone_motions[bone]->at(motion_ID.idx).GetLength();
    B.bone_or_part = bone;

    B.playing = TRUE;
    B.stop_at_end_callback = TRUE;
    B.stop_at_end = FALSE;
    //
    B.Callback = nullptr;
    B.CallbackParam = nullptr;

    B.channel = 0;
    B.fall_at_end = FALSE;
}
CBlend* CKinematicsAnimated::LL_PlayCycle(u16 part, MotionID motion_ID, BOOL bMixing, float blendAccrue,
    float blendFalloff, float Speed, BOOL noloop, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*=0*/)
{
    // validate and unroll
    if (!motion_ID.valid())
        return nullptr;
    if (BI_NONE == part)
    {
        for (u16 i = 0; i < MAX_PARTS; i++)
            LL_PlayCycle(
                i, motion_ID, bMixing, blendAccrue, blendFalloff, Speed, noloop, Callback, CallbackParam, channel);
        return nullptr;
    }
    if (part >= MAX_PARTS)
        return nullptr;
    if (nullptr == m_Partition->part(part).Name)
        return nullptr;

    //	shared_motions* s_mots	= &m_Motions[motion.slot];
    //	CMotionDef* m_def		= s_mots->motion_def(motion.idx);

    // Process old cycles and create _new_
    if (channel == 0)
    {
        _DBG_SINGLE_USE_MARKER;
        if (bMixing)
            LL_FadeCycle(part, blendFalloff, 1 << channel);
        else
            LL_CloseCycle(part, 1 << channel);
    }
    CPartDef& P = (*m_Partition)[part];
    CBlend* B = IBlend_Create();

    _DBG_SINGLE_USE_MARKER;
    IBlendSetup(
        *B, part, channel, motion_ID, bMixing, blendAccrue, blendFalloff, Speed, noloop, Callback, CallbackParam);
    for (u32 i = 0; i < P.bones.size(); i++)
        Bone_Motion_Start_IM((*bones)[P.bones[i]], B);
    blend_cycles[part].push_back(B);
    return B;
}
CBlend* CKinematicsAnimated::LL_PlayCycle(
    u16 part, MotionID motion_ID, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*=0*/)
{
    VERIFY(motion_ID.valid());
    CMotionDef* m_def = m_Motions[motion_ID.slot].motions.motion_def(motion_ID.idx);
    VERIFY(m_def);
    return LL_PlayCycle(part, motion_ID, bMixIn, m_def->Accrue(), m_def->Falloff(), m_def->Speed(), m_def->StopAtEnd(),
        Callback, CallbackParam, channel);
}
CBlend* CKinematicsAnimated::PlayCycle(
    LPCSTR N, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*= 0*/)
{
    MotionID motion_ID = ID_Cycle(N);
    if (motion_ID.valid())
        return PlayCycle(motion_ID, bMixIn, Callback, CallbackParam, channel);
    else
    {
        xrDebug::Fatal(DEBUG_INFO, "! MODEL: can't find cycle: %s", N);
        return nullptr;
    }
}
CBlend* CKinematicsAnimated::PlayCycle(
    MotionID motion_ID, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*= 0*/)
{
    VERIFY(motion_ID.valid());
    CMotionDef* m_def = m_Motions[motion_ID.slot].motions.motion_def(motion_ID.idx);
    VERIFY(m_def);
    return LL_PlayCycle(m_def->bone_or_part, motion_ID, bMixIn, m_def->Accrue(), m_def->Falloff(), m_def->Speed(),
        m_def->StopAtEnd(), Callback, CallbackParam, channel);
}

CBlend* CKinematicsAnimated::PlayCycle(
    u16 partition, MotionID motion_ID, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*= 0*/)
{
    VERIFY(motion_ID.valid());
    CMotionDef* m_def = m_Motions[motion_ID.slot].motions.motion_def(motion_ID.idx);
    VERIFY(m_def);
    return LL_PlayCycle(partition, motion_ID, bMixIn, m_def->Accrue(), m_def->Falloff(), m_def->Speed(),
        m_def->StopAtEnd(), Callback, CallbackParam, channel);
}

// fx'es
MotionID CKinematicsAnimated::ID_FX_Safe(LPCSTR N)
{
    MotionID motion_ID;
    for (int k = int(m_Motions.size()) - 1; k >= 0; --k)
    {
        shared_motions* s_mots = &m_Motions[k].motions;
        accel_map::iterator I = s_mots->fx()->find(pstr(N));
        if (I != s_mots->fx()->end())
        {
            motion_ID.set(u16(k), I->second);
            break;
        }
    }
    return motion_ID;
}
MotionID CKinematicsAnimated::ID_FX(LPCSTR N)
{
    MotionID motion_ID = ID_FX_Safe(N);
    R_ASSERT3(motion_ID.valid(), "! MODEL: can't find FX: ", N);
    return motion_ID;
}
CBlend* CKinematicsAnimated::PlayFX(MotionID motion_ID, float power_scale)
{
    VERIFY(motion_ID.valid());
    CMotionDef* m_def = m_Motions[motion_ID.slot].motions.motion_def(motion_ID.idx);
    VERIFY(m_def);
    return LL_PlayFX(m_def->bone_or_part, motion_ID, m_def->Accrue(), m_def->Falloff(), m_def->Speed(),
        m_def->Power() * power_scale);
}
CBlend* CKinematicsAnimated::PlayFX(LPCSTR N, float power_scale)
{
    MotionID motion_ID = ID_FX(N);
    return PlayFX(motion_ID, power_scale);
}

CBlend* CKinematicsAnimated::PlayFX_Safe(cpcstr N, float power_scale)
{
    MotionID motion_ID = ID_FX_Safe(N);
    if (motion_ID.valid())
        return PlayFX(motion_ID, power_scale);
    return nullptr;
}

// u16 part,u8 channel, MotionID motion_ID, BOOL  bMixing, float blendAccrue, float blendFalloff, float Speed, BOOL
// noloop, PlayCallback callback(), LPVOID CallbackParam)

CBlend* CKinematicsAnimated::LL_PlayFX(
    u16 bone, MotionID motion_ID, float blendAccrue, float blendFalloff, float Speed, float Power)
{
    if (!motion_ID.valid())
        return nullptr;
    if (blend_fx.size() >= MAX_BLENDED)
        return nullptr;
    if (BI_NONE == bone)
        bone = iRoot;

    CBlend* B = IBlend_Create();
    _DBG_SINGLE_USE_MARKER;
    IFXBlendSetup(*B, motion_ID, blendAccrue, blendFalloff, Power, Speed, bone);
    Bone_Motion_Start((*bones)[bone], B);

    blend_fx.push_back(B);
    return B;
}

void CKinematicsAnimated::DestroyCycle(CBlend& B)
{
    if (GetBlendDestroyCallback())
        GetBlendDestroyCallback()->BlendDestroy(B);
    // B.blend 		= CBlend::eFREE_SLOT;
    B.set_free_state();
    const CPartDef& P = m_Partition->part(B.bone_or_part);
    for (u32 i = 0; i < P.bones.size(); i++)
        Bone_Motion_Stop_IM((*bones)[P.bones[i]], &B);
}

// returns true if play time out

void CKinematicsAnimated::LL_UpdateTracks(float dt, bool b_force, bool leave_blends)
{
    BlendSVecIt I, E;
    // Cycles
    for (u16 part = 0; part < MAX_PARTS; part++)
    {
        if (nullptr == m_Partition->part(part).Name)
            continue;
        I = blend_cycles[part].begin();
        E = blend_cycles[part].end();
        for (; I != E; I++)
        {
            CBlend& B = *(*I);
            if (!b_force && B.dwFrame == RDEVICE.dwFrame)
                continue;
            B.dwFrame = RDEVICE.dwFrame;
            if (B.update(dt, B.Callback) && !leave_blends)
            {
                DestroyCycle(B);
                blend_cycles[part].erase(I);
                E = blend_cycles[part].end();
                I--;
            }
            // else{
            //	CMotionDef* m_def						= m_Motions[B.motionID.slot].motions.motion_def(B.motionID.idx);
            //	float timeCurrent						= B.timeCurrent;
            //	xr_vector<motion_marks>::iterator it	= m_def->marks.begin();
            //	xr_vector<motion_marks>::iterator it_e	= m_def->marks.end();
            //	for(;it!=it_e; ++it)
            //	{
            //		if( (*it).pick_mark(timeCurrent) )
            //	}
            //}
        }
    }

    LL_UpdateFxTracks(dt);
}
void CKinematicsAnimated::LL_UpdateFxTracks(float dt)
{
    // FX
    BlendSVecIt I, E;
    I = blend_fx.begin();
    E = blend_fx.end();
    for (; I != E; I++)
    {
        CBlend& B = *(*I);
        if (!B.stop_at_end_callback)
        {
            B.playing = FALSE;
            continue;
        }
        // B.timeCurrent += dt*B.speed;
        B.update_time(dt);
        switch (B.blend_state())
        {
        case CBlend::eFREE_SLOT: NODEFAULT;

        case CBlend::eAccrue:
            B.blendAmount += dt * B.blendAccrue * B.blendPower * B.speed;
            if (B.blendAmount >= B.blendPower)
            {
                // switch to fixed
                B.blendAmount = B.blendPower;
                // B.blend			= CBlend::eFalloff;//CBlend::eFixed;
                B.set_falloff_state();
            }
            break;
        case CBlend::eFalloff:
            B.blendAmount -= dt * B.blendFalloff * B.blendPower * B.speed;
            if (B.blendAmount <= 0)
            {
                // destroy fx
                // B.blend = CBlend::eFREE_SLOT;
                B.set_free_state();
                Bone_Motion_Stop((*bones)[B.bone_or_part], *I);
                blend_fx.erase(I);
                E = blend_fx.end();
                I--;
            }
            break;
        default: NODEFAULT;
        }
    }
}
void CKinematicsAnimated::UpdateTracks()
{
    _DBG_SINGLE_USE_MARKER;
    if (Update_LastTime == RDEVICE.dwTimeGlobal)
        return;
    u32 DT = RDEVICE.dwTimeGlobal - Update_LastTime;
    if (DT > 66)
        DT = 66;
    float dt = float(DT) / 1000.f;

    if (GetUpdateTracksCalback())
    {
        if ((*GetUpdateTracksCalback())(float(RDEVICE.dwTimeGlobal - Update_LastTime) / 1000.f, *this))
            Update_LastTime = RDEVICE.dwTimeGlobal;
        return;
    }
    Update_LastTime = RDEVICE.dwTimeGlobal;
    LL_UpdateTracks(dt, false, false);
}

void CKinematicsAnimated::Release()
{
    // xr_free bones
    //.	for (u32 i=0; i<bones->size(); i++)
    //.	{
    //.		CBoneDataAnimated* B	= (CBoneDataAnimated*)(*bones)[i];
    //.		B->Motions.clear		();
    //.	}

    // destroy shared data
    //.	xr_delete(partition);
    //.	xr_delete(motion_map);
    //.	xr_delete(m_cycle);
    //.	xr_delete(m_fx);

    inherited::Release();
}

CKinematicsAnimated::~CKinematicsAnimated() { IBoneInstances_Destroy(); }
CKinematicsAnimated::CKinematicsAnimated()
    : CKinematics(), IKinematicsAnimated(), blend_instances(nullptr), m_Partition(nullptr), m_blend_destroy_callback(nullptr),
      m_update_tracks_callback(nullptr), Update_LastTime(0)
{
}

void CKinematicsAnimated::IBoneInstances_Create()
{
    inherited::IBoneInstances_Create();
    u32 size = bones->size();
    blend_instances = xr_alloc<CBlendInstance>(size);
    for (u32 i = 0; i < size; i++)
        blend_instances[i].construct();
}

void CKinematicsAnimated::IBoneInstances_Destroy()
{
    inherited::IBoneInstances_Destroy();
    if (blend_instances)
    {
        xr_free(blend_instances);
        blend_instances = nullptr;
    }
}

#define PCOPY(a) a = pFrom->a
void CKinematicsAnimated::Copy(dxRender_Visual* P)
{
    inherited::Copy(P);

    CKinematicsAnimated* pFrom = (CKinematicsAnimated*)P;
    PCOPY(m_Motions);
    PCOPY(m_Partition);

    IBlend_Startup();
}

void CKinematicsAnimated::Spawn()
{
    inherited::Spawn();

    IBlend_Startup();

    for (u32 i = 0; i < bones->size(); i++)
        blend_instances[i].construct();
    m_update_tracks_callback = nullptr;
    channels.init();
}
void CKinematicsAnimated::ChannelFactorsStartup() { channels.init(); }
void CKinematicsAnimated::LL_SetChannelFactor(u16 channel, float factor) { channels.set_factor(channel, factor); }
void CKinematicsAnimated::IBlend_Startup()
{
    _DBG_SINGLE_USE_MARKER;
    CBlend B;
    // B.blend				= CBlend::eFREE_SLOT;

    B.set_free_state();

#ifdef DEBUG
    B.set_falloff_state();
#endif

    blend_pool.clear();
    for (u32 i = 0; i < MAX_BLENDED_POOL; i++)
    {
        blend_pool.push_back(B);
#ifdef DEBUG
        blend_pool.back().set_free_state();
#endif
    }
    // cycles+fx clear
    for (u32 i = 0; i < MAX_PARTS; i++)
        blend_cycles[i].clear();
    blend_fx.clear();
    ChannelFactorsStartup();
}

CBlend* CKinematicsAnimated::IBlend_Create()
{
    UpdateTracks();
    _DBG_SINGLE_USE_MARKER;
    for (auto &it : blend_pool)
        if (it.blend_state() == CBlend::eFREE_SLOT)
            return &it;
    FATAL("Too many blended motions requisted");
    return nullptr;
}
void CKinematicsAnimated::Load(const char* N, IReader* data, u32 dwFlags)
{
    inherited::Load(N, data, dwFlags);

    // Globals
    blend_instances = nullptr;
    m_Partition = nullptr;
    Update_LastTime = 0;

    // Load animation
    if (data->find_chunk(OGF_S_MOTION_REFS))
    {
        string_path items_nm;
        data->r_stringZ(items_nm, sizeof(items_nm));
        u32 set_cnt = _GetItemCount(items_nm);
        R_ASSERT(set_cnt < MAX_ANIM_SLOT);
        m_Motions.reserve(set_cnt);
        string_path nm;
        for (u32 k = 0; k < set_cnt; ++k)
        {
            _GetItem(items_nm, k, nm);
            xr_strcat(nm, ".omf");
            string_path fn;
            if (!FS.exist(fn, "$level$", nm))
            {
                if (!FS.exist(fn, "$game_meshes$", nm))
                {
#ifdef _EDITOR
                    Msg("!Can't find motion file '%s'.", nm);
                    return;
#else
                    xrDebug::Fatal(DEBUG_INFO, "Can't find motion file '%s'.", nm);
#endif
                }
            }
            // Check compatibility
            m_Motions.push_back(SMotionsSlot());
            bool create_res = true;
            if (!g_pMotionsContainer->has(nm)) // optimize fs operations
            {
                IReader* MS = FS.r_open(fn);
                create_res = m_Motions.back().motions.create(nm, MS, bones);
                FS.r_close(MS);
            }
            if (create_res)
                m_Motions.back().motions.create(nm, nullptr, bones);
            else
            {
                m_Motions.pop_back();
                Msg("! error in model [%s]. Unable to load motion file '%s'.", N, nm);
            }
        }
    }
    else if (data->find_chunk(OGF_S_MOTION_REFS2))
    {
        u32 set_cnt = data->r_u32();
        m_Motions.reserve(set_cnt);
        string_path nm;
        for (u32 k = 0; k < set_cnt; ++k)
        {
            data->r_stringZ(nm, sizeof(nm));
            xr_strcat(nm, ".omf");
            string_path fn;
            if (!FS.exist(fn, "$level$", nm))
            {
                if (!FS.exist(fn, "$game_meshes$", nm))
                {
#ifdef _EDITOR
                    Msg("!Can't find motion file '%s'.", nm);
                    return;
#else
                    xrDebug::Fatal(DEBUG_INFO, "Can't find motion file '%s'.", nm);
#endif
                }
            }
            // Check compatibility
            m_Motions.push_back(SMotionsSlot());
            bool create_res = true;
            if (!g_pMotionsContainer->has(nm)) // optimize fs operations
            {
                IReader* MS = FS.r_open(fn);
                create_res = m_Motions.back().motions.create(nm, MS, bones);
                FS.r_close(MS);
            }
            if (create_res)
                m_Motions.back().motions.create(nm, nullptr, bones);
            else
            {
                m_Motions.pop_back();
                Msg("! error in model [%s]. Unable to load motion file '%s'.", N, nm);
            }
        }
    }
    else
    {
        string_path nm;
        strconcat(sizeof(nm), nm, N, ".ogf");
        m_Motions.push_back(SMotionsSlot());
        m_Motions.back().motions.create(nm, data, bones);
    }

    R_ASSERT(m_Motions.size());

    m_Partition = m_Motions[0].motions.partition();
    m_Partition->load(this, N);

    // initialize motions
    for (auto &m_it : m_Motions)
    {
        SMotionsSlot& MS = m_it;
        MS.bone_motions.resize(bones->size());
        for (u32 i = 0; i < bones->size(); i++)
        {
            CBoneData* BD = (*bones)[i];
            MS.bone_motions[i] = MS.motions.bone_motions(BD->name);
        }
    }

    // Init blend pool
    IBlend_Startup();

    //.	if (motions.cycle()->size()<2)
    //.		Msg("* WARNING: model '%s' has only one motion. Candidate for SkeletonRigid???",N);
}

void CKinematicsAnimated::LL_BuldBoneMatrixDequatize(const CBoneData* bd, u8 channel_mask, SKeyTable& keys)
{
    u16 SelfID = bd->GetSelfID();
    CBlendInstance& BLEND_INST = LL_GetBlendInstance(SelfID);
    CKey BK[MAX_CHANNELS][MAX_BLENDED]; // base keys

    for (auto &it : BLEND_INST.blend_vector())
    {
        CBlend* B = it;
        int& b_count = keys.chanel_blend_conts[B->channel];
        CKey* D = &keys.keys[B->channel][b_count];
        if (!(channel_mask & (1 << B->channel)))
            continue;
        u8 channel = B->channel;
        // keys.blend_factors[channel][b_count]	=  B->blendAmount;
        keys.blends[channel][b_count] = B;
        CMotion& M = *LL_GetMotion(B->motionID, SelfID);
        Dequantize(*D, *B, M);
        QR2Quat(M._keysR[0], BK[channel][b_count].Q);
        if (M.test_flag(flTKeyPresent))
        {
            if (M.test_flag(flTKey16IsBit))
                QT16_2T(M._keysT16[0], M, BK[channel][b_count].T);
            else
                QT8_2T(M._keysT8[0], M, BK[channel][b_count].T);
        }
        else
            BK[channel][b_count].T.set(M._initT);
        ++b_count;
    }
    for (u16 j = 0; MAX_CHANNELS > j; ++j)
        if (channels.rule(j).extern_ == animation::add)
            keys_substruct(keys.keys[j], BK[j], keys.chanel_blend_conts[j]);
}

// calculate single bone with key blending
void CKinematicsAnimated::LL_BoneMatrixBuild(CBoneInstance& bi, const Fmatrix* parent, const SKeyTable& keys)
{
    // Blend them together
    CKey channel_keys[MAX_CHANNELS];
    animation::channel_def BC[MAX_CHANNELS];
    u16 ch_count = 0;

    for (u16 j = 0; MAX_CHANNELS > j; ++j)
    {
        if (j != 0 && keys.chanel_blend_conts[j] == 0)
            continue;
        // data for channel mix cycle based on ch_count
        channels.get_def(j, BC[ch_count]);
        process_single_channel(
            channel_keys[ch_count], BC[ch_count], keys.keys[j], keys.blends[j], keys.chanel_blend_conts[j]);
        ++ch_count;
    }
    CKey Result;
    // Mix channels
    MixChannels(Result, channel_keys, BC, ch_count);

    Fmatrix RES;
    RES.mk_xform(Result.Q, Result.T);
    bi.mTransform.mul_43(*parent, RES);
#ifdef DEBUG
#ifndef _EDITOR
    if (!check_scale(RES))
    {
        VERIFY(check_scale(bi.mTransform));
    }
    VERIFY(_valid(bi.mTransform));
    Fbox dbg_box;
    float box_size = 100000.f;
    dbg_box.set(-box_size, -box_size, -box_size, box_size, box_size, box_size);
    // VERIFY(dbg_box.contains(bi.mTransform.c));
    VERIFY2(dbg_box.contains(bi.mTransform.c),
        (make_string("model: %s has strange bone position, matrix : ", getDebugName().c_str()) +
            get_string(bi.mTransform))
            .c_str());

// if(!is_similar(PrevTransform,RES,0.3f))
//{
//	Msg("bone %s",*bd->name)	;
//}
// BONE_INST.mPrevTransform.set(RES);
#endif
#endif
}

// Добавить скриптовое смещение для кости --#SM+#--
void CKinematicsAnimated::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    inherited::LL_AddTransformToBone(offset);
}

// Обнулить скриптовое смещение для конкретной кости или всех сразу (bone_id = BI_NONE) --#SM+#--
void CKinematicsAnimated::LL_ClearAdditionalTransform(u16 bone_id) { inherited::LL_ClearAdditionalTransform(bone_id); }

void CKinematicsAnimated::BuildBoneMatrix(
    const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 channel_mask /*= (1<<0)*/)
{
    // CKey				R						[MAX_CHANNELS][MAX_BLENDED];	//all keys
    // float				BA						[MAX_CHANNELS][MAX_BLENDED];	//all factors
    // int				b_counts				[MAX_CHANNELS]	= {0,0,0,0}; //channel counts
    SKeyTable keys;
    LL_BuldBoneMatrixDequatize(bd, channel_mask, keys);

    LL_BoneMatrixBuild(bi, parent, keys);

    CalculateBonesAdditionalTransforms(bd, bi, parent, channel_mask); //--#SM+#--

    /*
    if(bi.mTransform.c.y>10000)
    {
    Log("BLEND_INST",BLEND_INST.Blend.size());
    Log("Bone",LL_BoneName_dbg(SelfID));
    Msg("Result.Q %f,%f,%f,%f",Result.Q.x,Result.Q.y,Result.Q.z,Result.Q.w);
    Log("Result.T",Result.T);
    Log("lp parent",(u32)parent);
    Log("parent",*parent);
    Log("RES",RES);
    Log("mT",bi.mTransform);

    CBlend*			B		=	*BI;
    CMotion&		M		=	*LL_GetMotion(B->motionID,SelfID);
    float			time	=	B->timeCurrent*float(SAMPLE_FPS);
    u32				frame	=	iFloor(time);
    u32				count	=	M.get_count();
    float			delta	=	time-float(frame);

    Log("flTKeyPresent",M.test_flag(flTKeyPresent));
    Log("M._initT",M._initT);
    Log("M._sizeT",M._sizeT);

    // translate
    if (M.test_flag(flTKeyPresent))
    {
    CKeyQT*	K1t	= &M._keysT[(frame+0)%count];
    CKeyQT*	K2t	= &M._keysT[(frame+1)%count];

    Fvector T1,T2,Dt;
    T1.x		= float(K1t->x)*M._sizeT.x+M._initT.x;
    T1.y		= float(K1t->y)*M._sizeT.y+M._initT.y;
    T1.z		= float(K1t->z)*M._sizeT.z+M._initT.z;
    T2.x		= float(K2t->x)*M._sizeT.x+M._initT.x;
    T2.y		= float(K2t->y)*M._sizeT.y+M._initT.y;
    T2.z		= float(K2t->z)*M._sizeT.z+M._initT.z;

    Dt.lerp	(T1,T2,delta);

    Msg("K1t %d,%d,%d",K1t->x,K1t->y,K1t->z);
    Msg("K2t %d,%d,%d",K2t->x,K2t->y,K2t->z);

    Log("count",count);
    Log("frame",frame);
    Log("T1",T1);
    Log("T2",T2);
    Log("delta",delta);
    Log("Dt",Dt);

    }else
    {
    D->T.set	(M._initT);
    }
    VERIFY(0);
    }
    */
}

void CKinematicsAnimated::OnCalculateBones() { UpdateTracks(); }
IBlendDestroyCallback* CKinematicsAnimated::GetBlendDestroyCallback() { return m_blend_destroy_callback; }
void CKinematicsAnimated::SetUpdateTracksCalback(IUpdateTracksCallback* callback)
{
    m_update_tracks_callback = callback;
}

void CKinematicsAnimated::SetBlendDestroyCallback(IBlendDestroyCallback* cb) { m_blend_destroy_callback = cb; }
#ifdef _EDITOR
MotionID CKinematicsAnimated::ID_Motion(LPCSTR N, u16 slot)
{
    MotionID motion_ID;
    if (slot < MAX_ANIM_SLOT)
    {
        shared_motions* s_mots = &m_Motions[slot].motions;
        // find in cycles
        accel_map::iterator I = s_mots->cycle()->find(pstr(N));
        if (I != s_mots->cycle()->end())
            motion_ID.set(slot, I->second);
        if (!motion_ID.valid())
        {
            // find in fx's
            accel_map::iterator I = s_mots->fx()->find(pstr(N));
            if (I != s_mots->fx()->end())
                motion_ID.set(slot, I->second);
        }
    }
    return motion_ID;
}
#endif
