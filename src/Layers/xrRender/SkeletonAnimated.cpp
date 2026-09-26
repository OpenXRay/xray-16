//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SkeletonAnimated.h"
#include "xrAnimation/OzzSkeletonMirror.h"

#include "SkeletonX.h"
#include "xrCore/FMesh.hpp"
#include "xrCore/xr_token.h"
#ifdef DEBUG
#include "xrCore/dump_string.h"
#endif


namespace xray::render::fg
{
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
    const auto& metadata = LL_MotionsSlot(ID.slot);
    for (const auto& it : metadata.motions)
        if (it.second == ID.idx)
            return std::make_pair(it.first.c_str(), metadata.source.c_str());
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
        {"eFalloff", CBlend::eFalloff}};
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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
MotionID CKinematicsAnimated::LL_MotionID(LPCSTR B)
{
    MotionID motion_ID;
    for (int k = int(LL_MotionsSlotCount()) - 1; k >= 0; --k)
    {
        const auto& motions = LL_MotionsSlot(u16(k)).motions;
        const auto I = motions.find(pstr(B));
        if (I != motions.end())
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
        const CPartDef& P = m_Partition->part(id);
        if (!P.Name)
            continue;
        if (0 == xr_stricmp(B, P.Name.c_str()))
            return id;
    }
    return BI_NONE;
}

// cycles
MotionID CKinematicsAnimated::ID_Cycle_Safe(LPCSTR N)
{
    MotionID motion_ID;
    for (int k = int(LL_MotionsSlotCount()) - 1; k >= 0; --k)
    {
        const auto& cycles = LL_MotionsSlot(u16(k)).cycles;
        const auto I = cycles.find(pstr(N));
        if (I != cycles.end())
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
    for (int k = int(LL_MotionsSlotCount()) - 1; k >= 0; --k)
    {
        const auto& cycles = LL_MotionsSlot(u16(k)).cycles;
        const auto I = cycles.find(N);
        if (I != cycles.end())
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
            B.stop_at_end_callback = false; // callback не должен приходить!
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

        const CPartDef& P = m_Partition->part(B.bone_or_part);
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
    return LL_MotionDuration(motion_ID) / LL_GetMotionDef(motion_ID)->Speed();
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
    B.timeTotal = LL_MotionDuration(motion_ID);
    B.bone_or_part = part;
    B.stop_at_end = noloop;
    B.playing = true;
    B.stop_at_end_callback = true;
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
    B.timeTotal = LL_MotionDuration(motion_ID);
    B.bone_or_part = bone;

    B.playing = true;
    B.stop_at_end_callback = true;
    B.stop_at_end = false;
    //
    B.Callback = nullptr;
    B.CallbackParam = nullptr;

    B.channel = 0;
    B.fall_at_end = false;
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
    if (!m_Partition->part(part).Name)
        return nullptr;


    // Process old cycles and create _new_
    if (channel == 0)
    {
        _DBG_SINGLE_USE_MARKER;
        if (bMixing)
            LL_FadeCycle(part, blendFalloff, 1 << channel);
        else
            LL_CloseCycle(part, 1 << channel);
    }
    const CPartDef& P = m_Partition->part(part);
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
    const CMotionDef* m_def = LL_GetMotionDef(motion_ID);
    VERIFY(m_def);
    return LL_PlayCycle(part, motion_ID, bMixIn, m_def->Accrue(), m_def->Falloff(), m_def->Speed(), m_def->StopAtEnd(),
        Callback, CallbackParam, channel);
}
CBlend* CKinematicsAnimated::PlayCycle(
    LPCSTR N, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*= 0*/)
{
    MotionID motion_ID = ID_Cycle(N);
    R_ASSERT3_CURE(motion_ID.valid(), "! MODEL: can't find cycle:", N, { return nullptr; });
    return PlayCycle(motion_ID, bMixIn, Callback, CallbackParam, channel);
}
CBlend* CKinematicsAnimated::PlayCycle(
    MotionID motion_ID, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*= 0*/)
{
    VERIFY(motion_ID.valid());
    const CMotionDef* m_def = LL_GetMotionDef(motion_ID);
    VERIFY(m_def);
    return LL_PlayCycle(m_def->bone_or_part, motion_ID, bMixIn, m_def->Accrue(), m_def->Falloff(), m_def->Speed(),
        m_def->StopAtEnd(), Callback, CallbackParam, channel);
}

CBlend* CKinematicsAnimated::PlayCycle(
    u16 partition, MotionID motion_ID, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel /*= 0*/)
{
    VERIFY(motion_ID.valid());
    const CMotionDef* m_def = LL_GetMotionDef(motion_ID);
    VERIFY(m_def);
    return LL_PlayCycle(partition, motion_ID, bMixIn, m_def->Accrue(), m_def->Falloff(), m_def->Speed(),
        m_def->StopAtEnd(), Callback, CallbackParam, channel);
}

// fx'es
MotionID CKinematicsAnimated::ID_FX_Safe(LPCSTR N)
{
    MotionID motion_ID;
    for (int k = int(LL_MotionsSlotCount()) - 1; k >= 0; --k)
    {
        const auto& effects = LL_MotionsSlot(u16(k)).effects;
        const auto I = effects.find(pstr(N));
        if (I != effects.end())
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
    const CMotionDef* m_def = LL_GetMotionDef(motion_ID);
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
        if (!m_Partition->part(part).Name)
            continue;
        I = blend_cycles[part].begin();
        E = blend_cycles[part].end();
        for (; I != E; I++)
        {
            CBlend& B = *(*I);
            if (!b_force && B.dwFrame == Device.dwFrame)
                continue;
            B.dwFrame = Device.dwFrame;
            if (B.update(dt, B.Callback) && !leave_blends)
            {
                DestroyCycle(B);
                blend_cycles[part].erase(I);
                E = blend_cycles[part].end();
                I--;
            }
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
            B.playing = false;
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
    if (Update_LastTime == Device.dwTimeGlobal)
        return;

    ZoneScopedN("Animation::TrackAdvance");

    u32 DT = Device.dwTimeGlobal - Update_LastTime;
    if (DT > 66)
        DT = 66;
    float dt = float(DT) / 1000.f;

    if (GetUpdateTracksCalback())
    {
        if ((*GetUpdateTracksCalback())(float(Device.dwTimeGlobal - Update_LastTime) / 1000.f, *this))
            Update_LastTime = Device.dwTimeGlobal;
        return;
    }
    Update_LastTime = Device.dwTimeGlobal;
    LL_UpdateTracks(dt, false, false);
}

void CKinematicsAnimated::Release()
{
    inherited::Release();
}

CKinematicsAnimated::~CKinematicsAnimated() { IBoneInstances_Destroy(); }

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
    PCOPY(m_animations);
    m_Partition = &m_animations->partition;
    m_pose.Reset(m_animations, MAX_BLENDED_POOL);

    IBlend_Startup();
}

void CKinematicsAnimated::Spawn()
{
    inherited::Spawn();
    LL_SetBoneRoot(m_animations->skeleton->jointToBone.front());
    m_pose.ClearBasePose();
    m_includePoseBase = true;
    m_poseDirty = true;

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
        {
            m_pose.ReserveBlend(u16(&it - blend_pool.begin()));
            return &it;
        }
    FATAL("Too many blended motions requisted");
    return nullptr;
}
void CKinematicsAnimated::Load(const char* N, IReader* data, u32 dwFlags)
{
    inherited::Load(N, data, dwFlags);
    m_animations = XRay::Animation::LoadOzzModelAnimations(N);
    R_ASSERT2(m_animations && m_animations->skeleton, N);
    R_ASSERT2(m_animations->skeleton->boneToJoint.size() == bones->size(), N);
    m_Partition = &m_animations->partition;
    m_pose.Reset(m_animations, MAX_BLENDED_POOL);
    Update_LastTime = 0;
    IBlend_Startup();
}

bool CKinematicsAnimated::PrepareBones()
{
    ZoneScopedN("Animation::PoseInputs");

    bool changed = m_poseDirty;
    for (u16 i = 0; i < blend_pool.size(); ++i)
    {
        const CBlend& blend = blend_pool[i];
        if (blend.blend_state() != CBlend::eFREE_SLOT)
            changed |= m_pose.SetBlend(i, blend.motionID, blend.timeCurrent, blend.blendAmount, blend.channel);
    }
    for (u16 bone = 0; bone < LL_BoneCount(); ++bone)
    {
        u16 slots[MAX_BLENDED];
        u16 count = 0;
        for (const CBlend* blend : blend_instances[bone].blend_vector())
            slots[count++] = u16(blend - blend_pool.begin());
        changed |= m_pose.SetBoneBlends(bone, ozz::span<const u16>(slots, count));
    }
    for (u16 channel = 0; channel < MAX_CHANNELS; ++channel)
    {
        animation::channel_def definition;
        channels.get_def(channel, definition);
        changed |= m_pose.SetChannelFactor(channel, definition.factor);
    }
    m_poseDirty = changed;
    return changed;
}

void CKinematicsAnimated::LL_EvaluateBonePose(Fmatrix& result, u16 bone, const Fmatrix& parent,
    const BonePoseQuery& query)
{
    UCalc_mtlock lock;
    ZoneScopedN("Animation::PoseQuery");

    PrepareBones();
    XRay::Animation::OzzPoseOverride controls;
    if (query.blend)
    {
        R_ASSERT(query.blend >= blend_pool.begin() && query.blend < blend_pool.end());
        if (query.isolated)
        {
            const auto& blends = blend_instances[bone].blend_vector();
            R_ASSERT(std::find(blends.begin(), blends.end(), query.blend) != blends.end());
        }
        controls.blend = u16(query.blend - blend_pool.begin());
    }
    controls.isolated = query.isolated;
    controls.time = query.time;
    controls.rotation = query.rotation;
    controls.translation = query.translation;
    m_pose.QueryBone(result, bone, parent, query.channels, controls);
}

void CKinematicsAnimated::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks)
{
    UCalc_mtlock lock;
    const bool includePoseBase = m_includePoseBase;
    m_includePoseBase = includePoseBase && !ignore_callbacks;
    inherited::Bone_GetAnimPos(pos, id, channel_mask, ignore_callbacks);
    m_includePoseBase = includePoseBase;
}

bool CKinematicsAnimated::LL_CapturePoseBase(float weight, XRay::Animation::PoseCaptureResult* result)
{
    UCalc_mtlock lock;
    if (result)
        *result = {};
    const auto reject = [result](pcstr reason, u16 bone, float value, float limit)
    {
        if (result)
            *result = {reason, bone, value, limit};
        return false;
    };

    if (!m_animations || !m_animations->skeleton)
        return reject("missing-skeleton", BI_NONE, 0.f, 0.f);
    if (!m_bones_offsets.empty())
        return reject("additional-bone-transform", m_bones_offsets.front().m_bone_id,
            float(m_bones_offsets.size()), 0.f);

    const u16 count = LL_BoneCount();
    if (!count)
        return reject("empty-skeleton", BI_NONE, 0.f, 1.f);

    xr_vector<Fmatrix> modelPose(count);
    xr_vector<u8> visibleBones(count);
    for (u16 bone = 0; bone < count; ++bone)
    {
        visibleBones[bone] = LL_GetBoneVisible(bone) ? 1 : 0;
        if (!visibleBones[bone])
        {
            modelPose[bone].identity();
            continue;
        }
        auto& instance = bone_instances[bone];
        if (instance.callback() && !instance.callback_overwrite())
            return reject("non-overwriting-callback", bone, float(instance.callback_type()), 0.f);
        const u16 parent = LL_GetData(bone).GetParentID();
        if (parent != BI_NONE && !LL_GetBoneVisible(parent) && !instance.callback_overwrite())
            return reject("hidden-parent-without-overwrite", bone, 0.f, 1.f);
        modelPose[bone].set(instance.mTransform);
    }

    if (!m_pose.SetBasePose(ozz::span<const Fmatrix>(modelPose.data(), modelPose.size()), weight,
            ozz::span<const u8>(visibleBones.data(), visibleBones.size()), result))
        return false;

    m_poseDirty = true;
    CalculateBones_Invalidate();
    return true;
}

bool CKinematicsAnimated::LL_SetPoseBaseWeight(float weight)
{
    UCalc_mtlock lock;
    const float previousWeight = m_pose.BasePoseWeight();
    if (!m_pose.SetBasePoseWeight(weight))
        return false;
    if (m_pose.BasePoseWeight() != previousWeight)
    {
        m_poseDirty = true;
        CalculateBones_Invalidate();
    }
    return true;
}

void CKinematicsAnimated::LL_ClearPoseBase()
{
    UCalc_mtlock lock;
    if (!m_pose.HasBasePose())
        return;

    m_pose.ClearBasePose();
    m_poseDirty = true;
    CalculateBones_Invalidate();
}

// Добавить скриптовое смещение для кости --#SM+#--
void CKinematicsAnimated::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    UCalc_mtlock lock;
    LL_ClearPoseBase();
    inherited::LL_AddTransformToBone(offset);
}

// Обнулить скриптовое смещение для конкретной кости или всех сразу (bone_id = BI_NONE) --#SM+#--
void CKinematicsAnimated::LL_ClearAdditionalTransform(u16 bone_id) { inherited::LL_ClearAdditionalTransform(bone_id); }

void CKinematicsAnimated::BuildBoneMatrix(
    const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 channel_mask)
{
    bi.mTransform.mul_43(*parent, m_pose.EvaluateLocalBone(bd->GetSelfID(), channel_mask, m_includePoseBase));
    CalculateBonesAdditionalTransforms(bd, bi, parent, channel_mask);
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
    if (slot < LL_MotionsSlotCount())
    {
        const auto& metadata = LL_MotionsSlot(slot);
        const auto cycle = metadata.cycles.find(pstr(N));
        if (cycle != metadata.cycles.end())
            motion_ID.set(slot, cycle->second);
        if (!motion_ID.valid())
        {
            const auto effect = metadata.effects.find(pstr(N));
            if (effect != metadata.effects.end())
                motion_ID.set(slot, effect->second);
        }
    }
    return motion_ID;
}
#endif
} // namespace xray::render::fg
