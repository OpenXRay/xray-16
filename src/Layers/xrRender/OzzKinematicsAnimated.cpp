#include "stdafx.h"

#include "OzzKinematicsAnimated.h"

#include "AnimationKeyCalculate.h"

#include <ozz/base/maths/simd_math.h>
#include <ozz/base/span.h>

namespace XRay
{
namespace Animation
{
namespace
{
void ExtractKey(const ozz::vector<ozz::math::SoaTransform>& locals, u16 joint, CKey& key)
{
    const ozz::math::SoaTransform& soa = locals[size_t(joint) >> 2];
    const size_t lane = size_t(joint) & 3;

    float tx[4], ty[4], tz[4], qx[4], qy[4], qz[4], qw[4];
    ozz::math::StorePtrU(soa.translation.x, tx);
    ozz::math::StorePtrU(soa.translation.y, ty);
    ozz::math::StorePtrU(soa.translation.z, tz);
    ozz::math::StorePtrU(soa.rotation.x, qx);
    ozz::math::StorePtrU(soa.rotation.y, qy);
    ozz::math::StorePtrU(soa.rotation.z, qz);
    ozz::math::StorePtrU(soa.rotation.w, qw);

    key.T.set(tx[lane], ty[lane], tz[lane]);
    key.Q.x = qx[lane];
    key.Q.y = qy[lane];
    key.Q.z = qz[lane];
    key.Q.w = qw[lane];
    key.Q.normalize();
}
}

OzzKinematicsAnimated::OzzKinematicsAnimated() : samples(MAX_BLENDED_POOL) {}

OzzKinematicsAnimated::~OzzKinematicsAnimated()
{
    if (!g_pOzzMotionLibraries)
        return;
    for (OzzMotionLibrary* library : libraries)
        if (library)
            g_pOzzMotionLibraries->undock(library);
}

void OzzKinematicsAnimated::Load(const char* N, IReader* data, u32 dwFlags)
{
    xray::render::fg::CKinematicsAnimated::Load(N, data, dwFlags);

    const u16 boneCount = LL_BoneCount();
    xr_vector<OzzBoneDesc> descs(boneCount);
    for (u16 i = 0; i < boneCount; ++i)
    {
        const CBoneData& bd = LL_GetData(i);
        descs[i].name = bd.name;
        descs[i].parent = bd.GetParentID();
        descs[i].bind_local = bd.bind_transform;
    }

    mirror = BuildOzzSkeletonMirror(ozz::span<const OzzBoneDesc>(descs.data(), descs.size()));

    libraries.assign(m_Motions.size(), nullptr);
    if (!mirror)
    {
        xrDebug::Fatal(DEBUG_INFO, "[ozz] can't build skeleton mirror for model '%s'", N);
        return;
    }

    string_path embedded;
    strconcat(sizeof(embedded), embedded, N, ".ogf");

    for (size_t k = 0; k < m_Motions.size(); ++k)
    {
        const shared_str& key = m_Motions[k].motions.id();
        if (0 == xr_strcmp(key.c_str(), embedded))
        {
            libraries[k] = g_pOzzMotionLibraries->dock(key, data, *mirror);
        }
        else
        {
            string_path fn;
            if (!FS.exist(fn, "$level$", key.c_str()) && !FS.exist(fn, "$game_meshes$", key.c_str()))
                xrDebug::Fatal(DEBUG_INFO, "[ozz] can't find motion file '%s' for model '%s'", key.c_str(), N);

            IReader* MS = FS.r_open(fn);
            if (!MS)
                xrDebug::Fatal(DEBUG_INFO, "[ozz] can't open motion file '%s' for model '%s'", fn, N);

            libraries[k] = g_pOzzMotionLibraries->dock(key, MS, *mirror);
            FS.r_close(MS);
        }

        if (!libraries[k])
            xrDebug::Fatal(DEBUG_INFO, "[ozz] can't bake motion library '%s' for model '%s'", key.c_str(), N);

        ValidateLibrary(N, key, k);
    }
}

void OzzKinematicsAnimated::ValidateLibrary(const char* N, const shared_str& key, size_t slot)
{
    const OzzMotionLibrary& library = *libraries[slot];
    const MotionVec* motions = m_Motions[slot].motions.bone_motions(LL_GetData(u16(0)).name);
    const size_t expected = motions ? motions->size() : 0;

    if (expected != library.animations.size())
        xrDebug::Fatal(DEBUG_INFO, "[ozz] motion count mismatch in '%s' for model '%s': %u legacy, %u baked",
            key.c_str(), N, u32(expected), u32(library.animations.size()));

    const int joints = mirror->skeleton.num_joints();
    for (size_t i = 0; i < library.animations.size(); ++i)
    {
        const ozz::animation::Animation* animation = library.animations[i].get();
        if (!animation)
            xrDebug::Fatal(
                DEBUG_INFO, "[ozz] missing baked motion %u in '%s' for model '%s'", u32(i), key.c_str(), N);
        if (animation->num_tracks() != joints)
            xrDebug::Fatal(DEBUG_INFO, "[ozz] motion %u in '%s' has %d tracks, model '%s' has %d joints", u32(i),
                key.c_str(), animation->num_tracks(), N, joints);
    }
}

void OzzKinematicsAnimated::Copy(xray::render::fg::dxRender_Visual* P)
{
    xray::render::fg::CKinematicsAnimated::Copy(P);

    OzzKinematicsAnimated* pFrom = static_cast<OzzKinematicsAnimated*>(P);
    mirror = pFrom->mirror;
    libraries = pFrom->libraries;
    for (OzzMotionLibrary* library : libraries)
        if (library)
            ++library->refs;
}

const OzzKinematicsAnimated::BlendSample& OzzKinematicsAnimated::EnsureSampled(CBlend& B)
{
#ifdef DEBUG
    const std::thread::id caller = std::this_thread::get_id();
    if (sampleOwner == std::thread::id())
        sampleOwner = caller;
    VERIFY2(sampleOwner == caller, "[ozz] sampling contexts touched from two threads");
#endif

    if (sampleFrame != Device.dwFrame)
    {
        live.clear();
        sampleFrame = Device.dwFrame;
    }

    const size_t slot = size_t(&B - blend_pool.begin());
    BlendSample& S = samples[slot];

    if (S.frame == Device.dwFrame && S.time == B.timeCurrent && S.motion == B.motionID)
        return S;

    for (u16 other : live)
    {
        const BlendSample& O = samples[other];
        if (O.motion == B.motionID && O.time == B.timeCurrent)
            return O;
    }

    const ozz::animation::Animation* animation = libraries[B.motionID.slot]->animations[B.motionID.idx].get();
    const int joints = mirror->skeleton.num_joints();

    if (S.ctx.max_tracks() < joints)
        S.ctx.Resize(joints);
    if (S.motion != B.motionID)
    {
        S.ctx.Invalidate();
        S.motion = B.motionID;
    }

    const size_t soa = size_t(mirror->skeleton.num_soa_joints());
    if (locals.size() != soa)
        locals.resize(soa);
    if (S.keys.size() != size_t(joints))
        S.keys.resize(size_t(joints));

    const float duration = animation->duration();
    float ratio = duration > 0.f ? B.timeCurrent / duration : 0.f;
    clamp(ratio, 0.f, 1.f);

    ozz::animation::SamplingJob job;
    job.animation = animation;
    job.context = &S.ctx;
    job.ratio = ratio;
    job.output = ozz::make_span(locals);
    R_ASSERT2(job.Run(), "[ozz] motion sampling failed");

    for (u16 j = 0; j < u16(joints); ++j)
        ExtractKey(locals, j, S.keys[j]);

    S.frame = Device.dwFrame;
    S.time = B.timeCurrent;
    live.push_back(u16(slot));
    return S;
}

void OzzKinematicsAnimated::LL_BuldBoneMatrixDequatize(const CBoneData* bd, u8 channel_mask, SKeyTable& keys)
{
    const u16 SelfID = bd->GetSelfID();
    VERIFY(mirror);

    const u16 joint = mirror->boneToJoint[SelfID];
    if (BI_NONE == joint)
        return;

    xray::render::fg::CBlendInstance& BLEND_INST = LL_GetBlendInstance(SelfID);
    CKey BK[MAX_CHANNELS][MAX_BLENDED];

    for (auto& it : BLEND_INST.blend_vector())
    {
        CBlend* B = it;
        if (!(channel_mask & (1 << B->channel)))
            continue;

        OzzMotionLibrary* library = libraries[B->motionID.slot];
        VERIFY(library && B->motionID.idx < library->animations.size() && library->animations[B->motionID.idx]);

        const u8 channel = B->channel;
        int& b_count = keys.chanel_blend_conts[channel];
        keys.blends[channel][b_count] = B;
        keys.keys[channel][b_count] = EnsureSampled(*B).keys[joint];

        if (channels.rule(channel).extern_ == xray::render::fg::animation::add)
            ExtractKey(FirstFrame(*library, B->motionID.idx), joint, BK[channel][b_count]);

        ++b_count;
    }

    for (u16 j = 0; MAX_CHANNELS > j; ++j)
        if (channels.rule(j).extern_ == xray::render::fg::animation::add)
            xray::render::fg::keys_substruct(keys.keys[j], BK[j], keys.chanel_blend_conts[j]);
}
}
}
