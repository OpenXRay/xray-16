#pragma once

#include "Engine.h"
#include "Common/Noncopyable.hpp"

class SchedulerData
{
public:
    u32 t_min : 14; // minimal bound of update time (sample: 20ms)
    u32 t_max : 14; // maximal bound of update time (sample: 200ms)
    u32 b_RT : 1;
    u32 b_locked : 1;
#ifdef DEBUG
    u32 dbg_startframe;
    u32 dbg_update_shedule;
#endif
};

class ISheduled
{
public:
    virtual ~ISheduled() = 0;
    virtual SchedulerData& GetSchedulerData() = 0;
    virtual float shedule_Scale() = 0;
    virtual void shedule_Update(u32 dt) = 0;
    // XXX nitrocaster: return (const char *) to reduce string pool spoiling
    virtual shared_str shedule_Name() const = 0;
    virtual bool shedule_Needed() = 0;
};

inline ISheduled::~ISheduled() {}

class ENGINE_API ScheduledBase : public virtual ISheduled, Noncopyable
{
public:
    SchedulerData shedule;

    ScheduledBase();
    virtual ~ScheduledBase();

    virtual SchedulerData& GetSchedulerData() override { return shedule; }
    virtual void shedule_Update(u32 dt) override;
    virtual shared_str shedule_Name() const override { return shared_str("unknown"); }

protected:
    virtual void shedule_register();
    virtual void shedule_unregister();
};
