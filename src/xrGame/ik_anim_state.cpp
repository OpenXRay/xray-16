#include "StdAfx.h"

#include "ik_anim_state.h"

#include "Include/xrRender/KinematicsAnimated.h"
#include "xrCore/Animation/Motion.hpp"

IC bool is_in(const motion_marks::interval& i, float v)
{
    if (i.first < i.second)
        return i.first < v && i.second > v;
    else
        return i.first < v || i.second > v;
}

float blend_time(const CBlend& b)
{
    VERIFY(b.timeTotal > 0.f);
    VERIFY(b.timeCurrent >= 0.f);
    VERIFY(!fis_zero(b.timeTotal));

    float t = (b.timeCurrent / b.timeTotal);
    t -= floor(t);

    VERIFY(t <= 1.f);
    VERIFY(t >= 0.f);

    return t * b.timeTotal;
}

float time_to_next_mark(const CBlend& b, const motion_marks& marks)
{
    VERIFY(!marks.is_empty());
    const float l_blend_time = blend_time(b);
    float time = marks.time_to_next_mark(l_blend_time);
    if (time < FLT_MAX)
        return time;
    time = marks.time_to_next_mark(EPS_S);
    if (time < FLT_MAX)
        return time + b.timeTotal - l_blend_time;
    return b.timeTotal - l_blend_time;
}

bool blend_in(const CBlend& b, const motion_marks& marks) { return NULL != marks.pick_mark(blend_time(b)); }
IC bool b_is_blending(const CBlend* current_blend, const CBlend* b)
{
    return current_blend && current_blend->blend_state() != CBlend::eFREE_SLOT && current_blend != b &&
        b->blendAmount < b->blendPower - EPS_L;
}

void ik_anim_state::update(IKinematicsAnimated* K, const CBlend* b, u16 i)
{
    // Andy	is_step = m && b && blend_in( *b, m->get_interval( i ) );
    VERIFY(K);
    is_step = false;
    is_idle = false;
    do_glue = false;
    is_blending = false;
    if (!b)
    {
        current_blend = 0;
        return;
    }
    CMotionDef& m_def_new = *K->LL_GetMotionDef(b->motionID);

    if (m_def_new.marks.size() <= i)
        return;

    if (b_is_blending(current_blend, b))
    {
        is_blending = true;
        CMotionDef& m_def_cur = *K->LL_GetMotionDef(current_blend->motionID);
        bool is_cur_step = (m_def_cur.marks.size() > i) && blend_in(*current_blend, m_def_cur.marks[i]);
        is_idle = !!(m_def_new.flags & esmIdle) && !!(m_def_cur.flags & esmIdle);
        bool any_idle = (m_def_cur.flags & esmIdle) || (m_def_new.flags & esmIdle);
        bool is_new_step = blend_in(*b, m_def_new.marks[i]);
        bool any_step = is_cur_step || is_new_step;
        bool step_all = is_cur_step && is_new_step;

        // is_step = step_all || any_idle && is_cur_step || is_new_step;

        do_glue = step_all || any_idle && is_new_step;
        is_step = (!any_idle && any_step) || any_idle && do_glue;
        // do_glue =true;
    }
    else
    {
        is_step = blend_in(*b, m_def_new.marks[i]);
        current_blend = b;
        is_idle = !!(m_def_new.flags & esmIdle);
        do_glue = true;
    }
}

bool ik_anim_state::time_step_begin(IKinematicsAnimated* K, const CBlend& B, u16 limb_id, float& time)
{
    time = 0;
    CMotionDef& m_def_cur = *K->LL_GetMotionDef(B.motionID);
    if (m_def_cur.marks.size() <= limb_id || !!(m_def_cur.flags & esmIdle))
        return false;
    motion_marks& marks = m_def_cur.marks[limb_id];
    if (marks.is_empty())
        return false;
    // if( blend_in( *current_blend, marks ) )
    //	time = 0;
    time = time_to_next_mark(B, marks);
    VERIFY(time < FLT_MAX);
    return true;
}
