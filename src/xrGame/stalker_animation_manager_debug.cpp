////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_manager_debug.cpp
//	Created 	: 25.02.2003
//  Modified 	: 13.12.2006
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation manager debug functions
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef DEBUG
#include "stalker_animation_manager.h"
#include "ai/stalker/ai_stalker.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

typedef std::pair<shared_str, shared_str> ANIMATION_ID;

struct animation_id_predicate
{
    IC bool operator()(const ANIMATION_ID& _1, const ANIMATION_ID& _2) const
    {
        if (_1.first._get() < _2.first._get())
            return (true);

        if (_2.first._get() < _1.first._get())
            return (false);

        return (_1.second._get() < _2.second._get());
    }
};

// IC	bool shared_str_predicate	(const shared_str &_1, const shared_str &_2)
//{
//	return		(_1._get() < _2._get());
//}
//
// typedef xr_set<shared_str,shared_str_predicate>	VISUALS;

struct animation_stats
{
    //	shared_str	m_visual_id;
    u32 m_frame_count;
    u32 m_start_count;

    IC animation_stats(const shared_str& visual_id, const u32& frame_count, const u32& start_count)
        : //		m_visual_id		(visual_id),
          m_frame_count(frame_count),
          m_start_count(start_count)
    {
    }
};

typedef std::pair<ANIMATION_ID, animation_stats> ANIMATION_STATS_PAIR;
typedef xr_map<ANIMATION_ID, animation_stats, animation_id_predicate> ANIMATION_STATS;
static ANIMATION_STATS g_animation_stats;

typedef std::pair<ANIMATION_ID, ANIMATION_ID> BLEND_ID;

struct blend_id_predicate
{
    IC bool less(const shared_str& _1, const shared_str& _2) const { return (_1._get() < _2._get()); }
    template <typename T>
    IC bool less(const std::pair<T, T>& _1, const std::pair<T, T>& _2) const
    {
        if (less(_1.first, _2.first))
            return (true);

        if (less(_2.first, _1.first))
            return (false);

        return (less(_1.second, _2.second));
    }

    IC bool operator()(const BLEND_ID& _1, const BLEND_ID& _2) const { return (less(_1, _2)); }
};

typedef std::pair<BLEND_ID, u32> BLEND_STATS_PAIR;
typedef xr_map<BLEND_ID, u32, blend_id_predicate> BLEND_STATS;
static BLEND_STATS g_blend_stats;

void show_animations()
{
    u32 animation_count = g_animation_stats.size();
    const ANIMATION_STATS_PAIR** const animations =
        (ANIMATION_STATS_PAIR const**)_alloca(animation_count * sizeof(ANIMATION_STATS_PAIR*));
    const ANIMATION_STATS_PAIR** i = animations;
    ANIMATION_STATS::const_iterator I = g_animation_stats.begin();
    ANIMATION_STATS::const_iterator E = g_animation_stats.end();
    for (; I != E; ++I, ++i)
        *i = (const ANIMATION_STATS_PAIR*)&(*I).first;

    struct predicate
    {
        static IC bool frame_count(const ANIMATION_STATS_PAIR* const& _1, const ANIMATION_STATS_PAIR* const& _2)
        {
            return (_1->second.m_frame_count < _2->second.m_frame_count);
        }
    };

    const ANIMATION_STATS_PAIR** const e = animations + animation_count;
    std::sort(animations, e, &predicate::frame_count);

    Msg("frames starts animation                        animation_set");
    for (i = animations; i != e; ++i)
        Msg("%6d %6d %-32s %s", (*i)->second.m_frame_count, (*i)->second.m_start_count, *(*i)->first.first,
            *(*i)->first.second);
}

void show_blends()
{
    u32 blend_count = g_blend_stats.size();
    const BLEND_STATS_PAIR** const blends = (BLEND_STATS_PAIR const**)_alloca(blend_count * sizeof(BLEND_STATS_PAIR*));
    const BLEND_STATS_PAIR** i = blends;
    BLEND_STATS::const_iterator I = g_blend_stats.begin();
    BLEND_STATS::const_iterator E = g_blend_stats.end();
    for (; I != E; ++I, ++i)
        *i = (const BLEND_STATS_PAIR*)&(*I).first;

    struct predicate
    {
        static IC bool blend_count(const BLEND_STATS_PAIR* const& _1, const BLEND_STATS_PAIR* const& _2)
        {
            return (_1->second < _2->second);
        }
    };

    const BLEND_STATS_PAIR** const e = blends + blend_count;
    std::sort(blends, e, &predicate::blend_count);

    Msg("       animation_set1                                  animation1    count     animation2                     "
        "    "
        "         animation_set2");
    for (i = blends; i != e; ++i)
        Msg("%-32s %32s ->[%6d]-> %-32s %32s", *(*i)->first.second.second, *(*i)->first.second.first, (*i)->second,
            *(*i)->first.first.first, *(*i)->first.first.second);
}

void show_animation_stats()
{
#ifdef DEBUG
    if (g_animation_stats.empty())
        return;

    show_animations();
    Msg("--------------------------------------------------");
    show_blends();
#endif
}

void add_animation(
    const shared_str& animation_id, const shared_str& animation_set_id, const shared_str& visual_id, bool just_started)
{
    ANIMATION_ID query(animation_id, animation_set_id);
    ANIMATION_STATS::iterator I = g_animation_stats.find(query);
    if (I != g_animation_stats.end())
    {
        ++((*I).second.m_frame_count);
        if (just_started)
            ++((*I).second.m_start_count);

        return;
    }

    g_animation_stats.insert(std::make_pair(query, animation_stats(visual_id, 1, just_started ? 1 : 0)));
}

void add_blend(const shared_str& animation_id, const shared_str& animation_set_id, const shared_str& visual_id,
    const std::pair<LPCSTR, LPCSTR>* blend_id)
{
    if (!blend_id)
        return;

    BLEND_ID query = std::make_pair(std::make_pair(animation_id, animation_set_id),
        std::make_pair(shared_str(blend_id->first), shared_str(blend_id->second)));

    BLEND_STATS::iterator I = g_blend_stats.find(query);
    if (I != g_blend_stats.end())
    {
        ++((*I).second);
        return;
    }

    g_blend_stats.insert(std::make_pair(query, 1));
}

void add_animation_stats(const shared_str& animation_id, const shared_str& animation_set_id,
    const shared_str& visual_id, const std::pair<LPCSTR, LPCSTR>* blend_id, bool just_started)
{
    add_animation(animation_id, animation_set_id, visual_id, just_started);
    add_blend(animation_id, animation_set_id, visual_id, blend_id);
}

void CStalkerAnimationManager::add_animation_stats(
    const ANIMATION_ID& animation_id, const BLEND_ID* blend_id, bool just_started)
{
    ::add_animation_stats(
        animation_id.first, animation_id.second, *object().Visual()->getDebugName(), blend_id, just_started);
}

void CStalkerAnimationManager::add_animation_stats()
{
    std::pair<LPCSTR, LPCSTR> blend;

    if (script().animation())
    {
        add_animation_stats(m_skeleton_animated->LL_MotionDefName_dbg(script().animation()),
            script().blend_id(m_skeleton_animated, blend), script().m_just_started);
        return;
    }

    if (global().animation())
    {
        add_animation_stats(m_skeleton_animated->LL_MotionDefName_dbg(global().animation()),
            global().blend_id(m_skeleton_animated, blend), global().m_just_started);
        return;
    }

    //	add_animation_stats
    //(m_skeleton_animated->LL_MotionDefName_dbg(head().animation()),head().blend_id(m_skeleton_animated,blend),head().m_just_started);
    add_animation_stats(m_skeleton_animated->LL_MotionDefName_dbg(torso().animation()),
        torso().blend_id(m_skeleton_animated, blend), torso().m_just_started);
    add_animation_stats(m_skeleton_animated->LL_MotionDefName_dbg(legs().animation()),
        legs().blend_id(m_skeleton_animated, blend), legs().m_just_started);
}
#endif // DEBUG
