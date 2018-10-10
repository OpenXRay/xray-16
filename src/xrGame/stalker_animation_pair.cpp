////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_pair.cpp
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation pair
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_animation_pair.h"
#include "stalker_animation_manager.h"
#include "xrCore/Animation/Motion.hpp"
#include "ai_debug.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/ai_monsters_anims.h"
#include "animation_movement_controller.h"
#include "xrCore/buffer_vector.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

void CStalkerAnimationPair::synchronize(
    IKinematicsAnimated* skeleton_animated, const CStalkerAnimationPair& stalker_animation) const
{
    if (!blend())
        return;

    CMotionDef* motion0 = skeleton_animated->LL_GetMotionDef(animation());
    VERIFY(motion0);
    if (!(motion0->flags & esmSyncPart))
        return;

    if (!stalker_animation.blend())
        return;

    CMotionDef* motion1 = skeleton_animated->LL_GetMotionDef(stalker_animation.animation());
    VERIFY(motion1);
    if (!(motion1->flags & esmSyncPart))
        return;

    blend()->timeCurrent = stalker_animation.blend()->timeCurrent;
}

#ifndef USE_HEAD_BONE_PART_FAKE
void CStalkerAnimationPair::play_global_animation(
    IKinematicsAnimated* skeleton_animated, PlayCallback callback, const bool& use_animation_movement_control)
#else

void CStalkerAnimationPair::play_global_animation(IKinematicsAnimated* skeleton_animated, PlayCallback callback,
    const u32& bone_part, const bool& use_animation_movement_control, const bool& local_animation, bool mix_animations)
#endif
{
    // DBG_OpenCashedDraw();
    // DBG_DrawBones( *m_object );
    // DBG_ClosedCashedDraw( 50000 );

    m_blend = 0;
    for (u16 i = 0; i < MAX_PARTS; ++i)
    {
#ifdef USE_HEAD_BONE_PART_FAKE
        if (!(bone_part & (1 << i)))
            continue;
#endif

        CBlend* blend = 0;
        if (!m_blend)
        {
            blend = skeleton_animated->LL_PlayCycle(i, animation(), mix_animations ? TRUE : FALSE, callback, m_object);

            if (blend && !m_blend)
                m_blend = blend;

            if (use_animation_movement_control || this->use_animation_movement_control(skeleton_animated, animation()))
            {
                m_object->create_anim_mov_ctrl(blend, m_target_matrix, local_animation);
            }
            else
            {
                if (m_object->animation_movement() && m_global_animation)
                    m_object->animation_movement()->stop();
            }
        }
        else
            skeleton_animated->LL_PlayCycle(i, animation(), mix_animations ? TRUE : FALSE, 0, 0);
    }
    // DBG_OpenCashedDraw();
    // DBG_DrawBones( *m_object );
    // DBG_ClosedCashedDraw( 50000 );
}

#ifndef USE_HEAD_BONE_PART_FAKE
void CStalkerAnimationPair::play(IKinematicsAnimated* skeleton_animated, PlayCallback callback,
    const bool& use_animation_movement_control, const bool& local_animation, bool continue_interrupted_animation,
    bool mix_animations)
#else
void CStalkerAnimationPair::play(IKinematicsAnimated* skeleton_animated, PlayCallback callback,
    const bool& use_animation_movement_control, const bool& local_animation, bool continue_interrupted_animation,
    const u32& bone_part, bool mix_animations)
#endif
{
    VERIFY(animation());
    if (actual())
    {
#if 0
#ifdef DEBUG
        if (psAI_Flags.is(aiAnimation) && blend())
            Msg				("%6d [%s][%s][%s][%f]",Device.dwTimeGlobal,m_object_name,m_animation_type_name,*animation()->name(),blend()->timeCurrent);
#endif
#endif

#ifdef DEBUG
        m_just_started = false;
#endif // DEBUG
        return;
    }

    if (animation() != m_array_animation)
    {
        m_array_animation.invalidate();
        m_array = 0;
    }

#ifdef DEBUG
    m_just_started = true;
#endif // DEBUG

    if (!global_animation())
    {
        // here we should know if it is a head
        // ugly way to find this out :-(
        // fix it in the future
        if (m_step_dependence && m_object->animation_movement())
            m_object->animation_movement()->stop();

        float pos = 0.f;
        if (m_step_dependence && continue_interrupted_animation)
        {
            VERIFY(!m_blend || !fis_zero(m_blend->timeTotal));
            if (m_step_dependence && m_blend)
                pos = fmod(m_blend->timeCurrent, m_blend->timeTotal) / m_blend->timeTotal;
        }
        // DBG_OpenCashedDraw();
        // DBG_DrawBones( *m_object );
        // DBG_ClosedCashedDraw( 50000 );
        m_blend = skeleton_animated->PlayCycle(animation(), TRUE, callback, m_object);

        if (m_step_dependence && continue_interrupted_animation)
        {
            if (m_object->animation().standing())
                pos = 0.5f;

            if (m_blend)
                m_blend->timeCurrent = m_blend->timeTotal * pos;
        }
        // DBG_OpenCashedDraw();
        // DBG_DrawBones( *m_object );
        // DBG_ClosedCashedDraw( 50000 );
    }
    else
#ifndef USE_HEAD_BONE_PART_FAKE
        play_global_animation(
            skeleton_animated, callback, use_animation_movement_control, local_animation, mix_animations);
#else
        play_global_animation(
            skeleton_animated, callback, bone_part, use_animation_movement_control, local_animation, mix_animations);
#endif
    m_actual = true;

    if (m_step_dependence)
        m_object->CStepManager::on_animation_start(animation(), blend());

#ifdef DEBUG
    if (psAI_Flags.is(aiAnimation))
    {
        CMotionDef* motion = skeleton_animated->LL_GetMotionDef(animation());
        VERIFY(motion);
        LPCSTR name = skeleton_animated->LL_MotionDefName_dbg(animation()).first;
        Msg("%6d [%s][%s][%s][%d][%c][%c][%c][%f][%f][%f]", Device.dwTimeGlobal, m_object_name, m_animation_type_name,
            name, motion->bone_or_part, (!(motion->flags & esmStopAtEnd)) ? '+' : '-',
            use_animation_movement_control ? '+' : '-', local_animation ? '+' : '-',
            VPUSH((m_target_matrix ? m_target_matrix->c : m_object->XFORM().c)));
    }
#endif
}

#ifdef DEBUG
std::pair<LPCSTR, LPCSTR>* CStalkerAnimationPair::blend_id(
    IKinematicsAnimated* skeleton_animated, std::pair<LPCSTR, LPCSTR>& result) const
{
    if (!blend())
        return (0);

    u32 bone_part_id = 0;
    if (!global_animation())
        bone_part_id = blend()->bone_or_part;

    // const BlendSVec			&blends = skeleton_animated->blend_cycle(bone_part_id);
    const u32 part_blends_num = skeleton_animated->LL_PartBlendsCount(bone_part_id);
    if (part_blends_num < 2)
        return (0);
    const u32 part_blend = part_blends_num - 2;
    CBlend* b = skeleton_animated->LL_PartBlend(bone_part_id, part_blend);
#if 0
    VERIFY2					(
        b->motionID != animation(),
        make_string(
            "animation is blending with itself (%s)",
            skeleton_animated->LL_MotionDefName_dbg(animation()).first
        )
    );
#endif
    result = skeleton_animated->LL_MotionDefName_dbg(b->motionID);
    return (&result);
}
#endif // DEBUG

void CStalkerAnimationPair::select_animation(const ANIM_VECTOR& array, const ANIMATION_WEIGHTS* weights)
{
    if (!weights)
    {
        m_array_animation = array[::Random.randI(array.size())];
        VERIFY(m_array_animation);
        return;
    }

    float accumulator = 0.f;
    ANIMATION_WEIGHTS::const_iterator I = weights->begin(), B = I;
    ANIMATION_WEIGHTS::const_iterator E = weights->end();

    u32 array_size = array.size();
    if (array_size < weights->size())
        E = B + array_size;

    for (; I != E; ++I)
        accumulator += *I;

    float chosen = ::Random.randF() * accumulator;
    accumulator = 0.f;
    for (I = B; I != E; ++I)
    {
        if ((accumulator + *I) >= chosen)
            break;

        accumulator += *I;
        continue;
    }

    VERIFY(I != E);
    VERIFY(u32(I - B) < array.size());
    m_array_animation = array[I - B];
    VERIFY(m_array_animation);
}

MotionID CStalkerAnimationPair::select(const ANIM_VECTOR& array, const ANIMATION_WEIGHTS* weights)
{
    VERIFY(!array.empty());

    if (m_array == &array)
    {
        VERIFY(animation());
        return (animation());
    }

    m_array = &array;
    select_animation(array, weights);
    return (m_array_animation);
}

void CStalkerAnimationPair::on_animation_end()
{
    make_inactual();
    m_blend = 0;

    if (m_callbacks.empty())
        return;

    u32 callback_count = m_callbacks.size();
    typedef buffer_vector<CALLBACK_ID> Callbacks;
    Callbacks callbacks(_alloca(callback_count * sizeof(Callbacks::value_type)), callback_count, m_callbacks.begin(),
        m_callbacks.end());
    Callbacks::const_iterator i = callbacks.begin();
    Callbacks::const_iterator e = callbacks.end();
    for (; i != e; ++i)
        (*i)();
}

void CStalkerAnimationPair::target_matrix(Fvector const& position, Fvector const& direction)
{
    m_target_matrix_impl.rotation(direction, Fvector().set(0.f, 1.f, 0.f));
    m_target_matrix_impl.c = position;
    m_target_matrix = &m_target_matrix_impl;
}

bool CStalkerAnimationPair::use_animation_movement_control(
    IKinematicsAnimated* skeleton_animated, MotionID const& motion_id) const
{
    CMotionDef* motion_def = skeleton_animated->LL_GetMotionDef(motion_id);
    VERIFY(motion_def);
    return ((motion_def->flags & esmRootMover) == esmRootMover);
}

void CStalkerAnimationPair::reset()
{
#if 0 // def DEBUG
    if (m_animation)
        Msg						("animation [%s][%s] is reset",m_object_name,m_animation_type_name);
#endif // DEBUG

    m_animation.invalidate();
    m_blend = 0;
    m_actual = true;
    m_array = 0;
    m_array_animation.invalidate();
}
