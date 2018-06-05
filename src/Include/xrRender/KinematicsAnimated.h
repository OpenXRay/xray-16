#ifndef KinematicsAnimated_included
#define KinematicsAnimated_included
#pragma once

#include "xrCore/Animation/SkeletonMotions.hpp"
#include "animation_blend.h"
#include "Layers/xrRender/KinematicAnimatedDefs.h"
#include "Layers/xrRender/KinematicsAddBoneTransform.hpp" //--#SM+#--

class IKinematics;
class CBlend;
class CKinematicsAnimated;
class CBoneInstanceAnimated;
struct CKey;
class CInifile;
class IKinematicsAnimated;
class IRenderVisual;
struct IterateBlendsCallback
{
    virtual void operator()(CBlend& B) = 0;
};
struct IUpdateTracksCallback
{
    virtual bool operator()(float dt, IKinematicsAnimated& k) = 0;
};

struct SKeyTable
{
    CKey keys[MAX_CHANNELS][MAX_BLENDED]; // all keys
    CBlend* blends[MAX_CHANNELS][MAX_BLENDED]; // blend pointers
    int chanel_blend_conts[MAX_CHANNELS]; // channel counts
    SKeyTable() { std::fill_n(chanel_blend_conts, MAX_CHANNELS, 0); }
};

class IKinematicsAnimated
{
public:
    virtual ~IKinematicsAnimated() { ; }
    // Calculation
public:
    virtual void OnCalculateBones() = 0;

#ifdef DEBUG
    virtual std::pair<LPCSTR, LPCSTR> LL_MotionDefName_dbg(MotionID ID) = 0;
    virtual void LL_DumpBlends_dbg() = 0;
#endif

    virtual u32 LL_PartBlendsCount(u32 bone_part_id) = 0;
    virtual CBlend* LL_PartBlend(u32 bone_part_id, u32 n) = 0;
    virtual void LL_IterateBlends(IterateBlendsCallback& callback) = 0;

    virtual u16 LL_MotionsSlotCount() = 0;
    virtual const shared_motions& LL_MotionsSlot(u16 idx) = 0;

    //IC CMotionDef* LL_GetMotionDef(MotionID id) { return m_Motions[id.slot].motions.motion_def(id.idx); }
    //IC CMotion* LL_GetRootMotion(MotionID id) { return &m_Motions[id.slot].bone_motions[iRoot]->at(id.idx); }
    //IC CMotion* LL_GetMotion(MotionID id, u16 bone_id) {return &m_Motions[id.slot].bone_motions[bone_id]->at(id.idx); }
    virtual CMotionDef* LL_GetMotionDef(MotionID id) = 0;
    virtual CMotion* LL_GetRootMotion(MotionID id) = 0;
    virtual CMotion* LL_GetMotion(MotionID id, u16 bone_id) = 0;
    // interface for procedural animations :)
    virtual void LL_BuldBoneMatrixDequatize(const CBoneData* bd, u8 channel_mask, SKeyTable& keys) = 0;
    virtual void LL_BoneMatrixBuild(CBoneInstance& bi, const Fmatrix* parent, const SKeyTable& keys) = 0;

    virtual void LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset) = 0; //--#SM+#--
    virtual void LL_ClearAdditionalTransform(u16 bone_id) = 0; //--#SM+#--

    virtual IBlendDestroyCallback* GetBlendDestroyCallback() = 0;
    virtual void SetBlendDestroyCallback(IBlendDestroyCallback* cb) = 0;
    virtual void SetUpdateTracksCalback(IUpdateTracksCallback* callback) = 0;
    virtual IUpdateTracksCallback* GetUpdateTracksCalback() = 0;

    // Low level interface
    virtual MotionID LL_MotionID(LPCSTR B) = 0;
    virtual u16 LL_PartID(LPCSTR B) = 0;

    //CBlend* LL_PlayFX(u16 bone, MotionID motion, float blendAccrue, float blendFalloff, float Speed, float Power);
    virtual CBlend* LL_PlayCycle(u16 partition, MotionID motion, BOOL bMixing, float blendAccrue, float blendFalloff,
        float Speed, BOOL noloop, PlayCallback Callback, LPVOID CallbackParam, u8 channel = 0) = 0;
    virtual CBlend* LL_PlayCycle(
        u16 partition, MotionID motion, BOOL bMixIn, PlayCallback Callback, LPVOID CallbackParam, u8 channel = 0) = 0;
    //void LL_FadeCycle(u16 partition, float falloff, u8 mask_channel = (1 << 0));
    virtual void LL_CloseCycle(u16 partition, u8 mask_channel = (1 << 0)) = 0;
    virtual void LL_SetChannelFactor(u16 channel, float factor) = 0;
    //virtual CBlendInstance& LL_GetBlendInstance(u16 bone_id) = 0;

    // Main functionality
    virtual void UpdateTracks() = 0; // Update motions
    virtual void LL_UpdateTracks(float dt, bool b_force, bool leave_blends) = 0; // Update motions
    //void DestroyCycle(CBlend& B);

    // cycles
    virtual MotionID ID_Cycle(LPCSTR N) = 0;
    virtual MotionID ID_Cycle_Safe(LPCSTR N) = 0;
    virtual MotionID ID_Cycle(shared_str N) = 0;
    virtual MotionID ID_Cycle_Safe(shared_str N) = 0;
    virtual CBlend* PlayCycle(
        LPCSTR N, BOOL bMixIn = TRUE, PlayCallback Callback = nullptr, LPVOID CallbackParam = nullptr, u8 channel = 0) = 0;
    virtual CBlend* PlayCycle(
        MotionID M, BOOL bMixIn = TRUE, PlayCallback Callback = nullptr, LPVOID CallbackParam = nullptr, u8 channel = 0) = 0;
    virtual CBlend* PlayCycle(u16 partition, MotionID M, BOOL bMixIn = TRUE, PlayCallback Callback = nullptr,
        LPVOID CallbackParam = nullptr, u8 channel = 0) = 0;
    // fx'es
    virtual MotionID ID_FX(LPCSTR N) = 0;
    virtual MotionID ID_FX_Safe(LPCSTR N) = 0;
    virtual CBlend* PlayFX(LPCSTR N, float power_scale) = 0;
    virtual CBlend* PlayFX(MotionID M, float power_scale) = 0;

    virtual const CPartition& partitions() const = 0;

    virtual IRenderVisual* dcast_RenderVisual() = 0;
    virtual IKinematics* dcast_PKinematics() = 0;

    virtual float get_animation_length(MotionID motion_ID) = 0;
    //#ifdef DEBUG
    //virtual const BlendSVec& blend_cycle(const u32& bone_part_id) const = 0;
    //#endif
};

#endif // KinematicsAnimated_included
