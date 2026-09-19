//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "SkeletonMotions.hpp"


u16 CPartition::part_id(const shared_str& name) const
{
    for (u16 i = 0; i < MAX_PARTS; ++i)
    {
        const CPartDef& pd = part(i);
        if (pd.Name == name)
            return i;
    }
    Msg("! there is no part named [%s]", name.c_str());
    return u16(-1);
}


bool CMotionDef::StopAtEnd() const { return !!(flags & esmStopAtEnd); }

const motion_marks::interval* motion_marks::pick_mark(const float& t) const
{
    C_ITERATOR it = intervals.begin();
    const C_ITERATOR it_e = intervals.end();

    for (; it != it_e; ++it)
    {
        const interval& I = (*it);
        if (I.first <= t && I.second >= t)
            return &I;

        if (I.first > t)
            break;
    }
    return nullptr;
}

bool motion_marks::is_mark_between(float const& t0, float const& t1) const
{
    VERIFY(t0 <= t1);

    C_ITERATOR i = intervals.begin();
    const C_ITERATOR e = intervals.end();
    for (; i != e; ++i)
    {
        VERIFY((*i).first <= (*i).second);

        if ((*i).first == t0)
            return (true);

        if ((*i).first > t0)
        {
            if ((*i).second <= t1)
                return (true);

            if ((*i).first <= t1)
                return (true);

            return (false);
        }

        if ((*i).second < t0)
            continue;

        if ((*i).second == t0)
            return (true);

        return (true);
    }

    return (false);
}

float motion_marks::time_to_next_mark(float time) const
{
    C_ITERATOR i = intervals.begin();
    const C_ITERATOR e = intervals.end();
    float result_dist = FLT_MAX;
    for (; i != e; ++i)
    {
        const float dist = (*i).first - time;
        if (dist > 0.f && dist < result_dist)
            result_dist = dist;
    }
    return result_dist;
}

void motion_marks::Load(IReader* R)
{
    xr_string tmp;
    R->r_string(tmp);
    name = tmp.c_str();
    const u32 cnt = R->r_u32();
    intervals.resize(cnt);
    for (u32 i = 0; i < cnt; ++i)
    {
        interval& item = intervals[i];
        item.first = R->r_float();
        item.second = R->r_float();
    }
}

void motion_marks::Save(IWriter* W) const
{
    W->w_string(name.c_str());
    const u32 cnt = intervals.size();
    W->w_u32(cnt);
    for (u32 i = 0; i < cnt; ++i)
    {
        const interval& item = intervals[i];
        W->w_float(item.first);
        W->w_float(item.second);
    }
}
