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
        return;

    string_path embedded;
    strconcat(sizeof(embedded), embedded, N, ".ogf");

    for (size_t k = 0; k < m_Motions.size(); ++k)
    {
        const shared_str& key = m_Motions[k].motions.id();
        if (0 == xr_strcmp(key.c_str(), embedded))
        {
            libraries[k] = g_pOzzMotionLibraries->dock(key, data, *mirror);
            continue;
        }

        string_path fn;
        if (!FS.exist(fn, "$level$", key.c_str()) && !FS.exist(fn, "$game_meshes$", key.c_str()))
        {
            Msg("! [ozz] can't find motion file [%s] for model [%s]", key.c_str(), N);
            continue;
        }

        IReader* MS = FS.r_open(fn);
        if (!MS)
        {
            Msg("! [ozz] can't open motion file [%s] for model [%s]", fn, N);
            continue;
        }
        libraries[k] = g_pOzzMotionLibraries->dock(key, MS, *mirror);
        FS.r_close(MS);
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
    BlendSample& S = samples[size_t(&B - blend_pool.begin())];

    if (S.frame == Device.dwFrame && S.time == B.timeCurrent && S.motion == B.motionID)
        return S;

    const ozz::animation::Animation* animation = libraries[B.motionID.slot]->animations[B.motionID.idx].get();

    if (S.motion != B.motionID)
    {
        S.ctx.Invalidate();
        S.ctx.Resize(mirror->skeleton.num_joints());
        S.motion = B.motionID;
    }
    if (S.locals.size() != size_t(mirror->skeleton.num_soa_joints()))
        S.locals.resize(size_t(mirror->skeleton.num_soa_joints()));

    const float duration = animation->duration();
    float ratio = duration > 0.f ? B.timeCurrent / duration : 0.f;
    clamp(ratio, 0.f, 1.f);

    ozz::animation::SamplingJob job;
    job.animation = animation;
    job.context = &S.ctx;
    job.ratio = ratio;
    job.output = ozz::make_span(S.locals);
    job.Run();

    S.frame = Device.dwFrame;
    S.time = B.timeCurrent;
    return S;
}

void OzzKinematicsAnimated::LL_BuldBoneMatrixDequatize(const CBoneData* bd, u8 channel_mask, SKeyTable& keys)
{
    const u16 SelfID = bd->GetSelfID();
    if (!mirror)
        return;

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

        OzzMotionLibrary* library = B->motionID.slot < libraries.size() ? libraries[B->motionID.slot] : nullptr;
        if (!library || B->motionID.idx >= library->animations.size() || !library->animations[B->motionID.idx])
            continue;

        const u8 channel = B->channel;
        int& b_count = keys.chanel_blend_conts[channel];
        keys.blends[channel][b_count] = B;

        ExtractKey(EnsureSampled(*B).locals, joint, keys.keys[channel][b_count]);

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
