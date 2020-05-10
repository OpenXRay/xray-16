////////////////////////////////////////////////////////////////////////////
//	Module 		: profiler.h
//	Created 	: 23.07.2004
//  Modified 	: 23.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Profiler
////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(DEBUG) && !defined(USE_PROFILER)
#define USE_PROFILER
#endif

#ifdef USE_PROFILER

#include "xrCore/xrCore.h"
#include "xrEngine/Engine.h"
#include "xrEngine/defines.h"

class IGameFont;

#ifdef CONFIG_PROFILE_LOCKS
extern void add_profile_portion(pcstr id, const u64 &time);
#endif

#pragma pack(push, 4)
struct CProfileResultPortion
{
    u64 m_time;
    pcstr m_timer_id;
};
#pragma pack(pop)

struct CProfilePortion : public CProfileResultPortion
{
private:
    bool enabled = false;
public:
    inline CProfilePortion(const char *id, bool enableIf = true);
    inline ~CProfilePortion();
};

struct CProfileStats
{
    u32 m_update_time;
    shared_str m_name;
    float m_time;
    float m_min_time;
    float m_max_time;
    float m_total_time;
    u32 m_count;
    u32 m_call_count;

    inline CProfileStats();
};

class ENGINE_API CProfiler
{
private:
    struct pred_rstr
    {
        bool operator()(const shared_str &lhs, const shared_str &rhs) const
        { return xr_strcmp(*lhs, *rhs)<0; }
    };

protected:
    typedef xr_vector<CProfileResultPortion> PORTIONS;
    typedef xr_map<shared_str, CProfileStats, pred_rstr> TIMERS;

protected:
    PORTIONS m_portions;
    TIMERS m_timers;
    bool m_actual;
    Lock m_section;
    u32 m_call_count;

protected:
    void setup_timer(pcstr timer_id, const u64 &timer_time, const u32 &call_count);
    inline void convert_string(pcstr str, shared_str &out, u32 max_string_size);

public:
    CProfiler();
    ~CProfiler();
    void show_stats(IGameFont &font, bool show);
    void clear();
    void add_profile_portion(const CProfileResultPortion &profile_portion);
};

extern ENGINE_API CProfiler* g_profiler;

inline CProfiler& profiler();

#include "profiler_inline.h"

#define START_PROFILE(...) \
    {                      \
        CProfilePortion __profile_portion__(__VA_ARGS__);
#define STOP_PROFILE }

#else // !USE_PROFILER
#define START_PROFILE(a) {
#define STOP_PROFILE }
#endif
