////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cast_stats.cpp
//	Created 	: 17.09.2004
//  Modified 	: 17.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Smart dynamic cast statistics
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef DEBUG
#define SMART_CAST_STATS

class CSmartCastStats
{
private:
    static CSmartCastStats* m_instance;

private:
    struct CStats
    {
        LPCSTR m_from;
        LPCSTR m_to;
        u32 m_count;

        IC CStats(LPCSTR from, LPCSTR to, u32 count) : m_from(from), m_to(to), m_count(count) {}
        IC bool operator<(const CStats& stats) const
        {
            return ((m_from < stats.m_from) || ((m_from == stats.m_from) && (m_to < stats.m_to)));
        }
    };

    struct CStatsPredicate
    {
        IC bool operator()(const CStats& _1, const CStats& _2) const { return (_1.m_count < _2.m_count); }
    };

private:
    typedef xr_set<CStats> STATS;

private:
    STATS m_stats;
    xr_vector<CStats> m_temp;

public:
    IC static CSmartCastStats* instance();
    IC static void _release();

public:
    IC void add(LPCSTR from, LPCSTR to);
    IC void clear();
    IC void show();
};

CSmartCastStats* CSmartCastStats::m_instance = 0;

IC CSmartCastStats* CSmartCastStats::instance()
{
    if (!m_instance)
        m_instance = new CSmartCastStats();
    return (m_instance);
}

IC void CSmartCastStats::_release() { xr_delete(m_instance); }
IC CSmartCastStats& stats() { return (*CSmartCastStats::instance()); }
#ifdef SMART_CAST_STATS_ALL
CSmartCastStats* g_smart_cast_opt = 0;

IC CSmartCastStats& stats_all()
{
    if (!g_smart_cast_opt)
        g_smart_cast_opt = new CSmartCastStats();
    return (*g_smart_cast_opt);
}
#endif

IC void CSmartCastStats::add(LPCSTR from, LPCSTR to)
{
    CStats temp(from, to, 1);
    STATS::iterator I = m_stats.find(temp);
    if (I == m_stats.end())
        m_stats.insert(temp);
    else
        ++(const_cast<CStats&>(*I).m_count);
}

IC void CSmartCastStats::clear() { m_stats.clear(); }
IC void CSmartCastStats::show()
{
    if (m_stats.empty())
    {
        Msg("CONGRATULATIONS : SmartCast stats is empty!!!");
        return;
    }

    m_temp.clear();
    m_temp.insert(m_temp.begin(), m_stats.begin(), m_stats.end());
    std::sort(m_temp.begin(), m_temp.end(), CStatsPredicate());
    u32 total = 0;

    xr_vector<CStats>::const_iterator I = m_temp.begin();
    xr_vector<CStats>::const_iterator E = m_temp.end();
    for (; I != E; ++I)
        total += (*I).m_count;

    Msg("SmartCast stats (different %d, total %d) : ", (u32)m_stats.size(), total);

    I = m_temp.begin();
    for (; I != E; ++I)
        Msg("%8d %6.2f% : smart_cast<%s>(%s)", (*I).m_count, float((*I).m_count) * 100.f / float(total), (*I).m_to,
            (*I).m_from);
}

void add_smart_cast_stats(LPCSTR from, LPCSTR to)
{
#ifdef SMART_CAST_STATS
    stats().add(from, to);
#endif
}

void add_smart_cast_stats_all(LPCSTR from, LPCSTR to)
{
#ifdef SMART_CAST_STATS
#ifdef SMART_CAST_STATS_ALL
    stats_all().add(from, to);
#endif
#endif
}

void show_smart_cast_stats()
{
#ifdef SMART_CAST_STATS
    stats().show();
#ifdef SMART_CAST_STATS_ALL
    stats_all().show();
#endif
#else
    Msg("! SMART_CAST_STATS macros is not defined, stats is disabled");
#endif
}

void clear_smart_cast_stats()
{
#ifdef SMART_CAST_STATS
    stats().clear();
#ifdef SMART_CAST_STATS_ALL
    stats_all().clear();
#endif
#else
    Msg("! SMART_CAST_STATS macros is not defined, stats is disabled");
#endif
}

void release_smart_cast_stats()
{
#ifdef SMART_CAST_STATS
    CSmartCastStats::_release();
#ifdef SMART_CAST_STATS_ALL
    xr_delete(g_smart_cast_opt);
#endif
#endif
}

#endif // DEBUG
