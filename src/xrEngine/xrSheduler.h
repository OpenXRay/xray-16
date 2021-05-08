#pragma once

#include "xrCore/xrstring.h"
#include "xrCore/FTimer.h"

class ISheduled;

class ENGINE_API CSheduler
{
private:
    struct Item
    {
        u32 dwTimeForExecute;
        u32 dwTimeOfLastExecute;
        shared_str scheduled_name;
        ISheduled* Object;
        u32 dwPadding; // for align-issues

        IC bool operator<(Item& I) { return dwTimeForExecute > I.dwTimeForExecute; }
    };
    struct ItemReg
    {
        BOOL OP;
        BOOL RT;
        ISheduled* Object;
    };

    struct SchedulerStatistics
    {
        float Load;
        CStatTimer Update;

        SchedulerStatistics() { FrameStart(); }
        void FrameStart()
        {
            Load = 0.0f;
            Update.FrameStart();
        }

        void FrameEnd() { Update.FrameEnd(); }
    };

private:
    xr_vector<Item> ItemsRT;
    xr_vector<Item> Items;
    xr_vector<Item> ItemsProcessed;
    xr_vector<ItemReg> Registration;
    ISheduled* m_current_step_obj;
    bool m_processing_now;
    SchedulerStatistics stats;

    IC void Push(Item& I);
    IC void Pop();
    IC Item& Top() { return Items.front(); }
    void internal_Register(ISheduled* A, bool RT = false);
    bool internal_Unregister(ISheduled* A, bool RT, bool warn_on_not_found = true);
    void internal_Registration();

public:
    u64 cycles_start;
    u64 cycles_limit;

public:
    void ProcessStep();
    void Process();
    void Update();

#ifdef DEBUG
    bool Registered(ISheduled* object) const;
#endif // DEBUG
    void Register(ISheduled* A, bool RT = false);
    void Unregister(ISheduled* A);
    void EnsureOrder(ISheduled* Before, ISheduled* After);

    void Initialize();
    void Destroy();
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);
    const CStatTimer& GetUpdateTime()
    {
        stats.FrameEnd();
        return stats.Update;
    }
};

namespace XRay
{
class ENGINE_API Scheduler
{
    struct ItemReg
    {
        bool Operational;
        bool RealtimePriority;
        ISheduled* Object;
    };

    struct Item
    {
        ISheduled* Object;
        shared_str ScheduledName;

        u32 TimeForExecute;
        u32 TimeOfLastExecute;

        bool operator<(const Item& rhs) const { return TimeForExecute > rhs.TimeForExecute; }

    private:
        u32 padding; // This makes Item to be 4 bytes
    };

    struct SchedulerStatistics
    {
        float Load;
        CStatTimer Update;

        SchedulerStatistics() { FrameStart(); }
        void FrameStart()
        {
            Load = 0.0f;
            Update.FrameStart();
        }

        void FrameEnd() { Update.FrameEnd(); }
    };

    xr_vector<Item> RealtimeUpdateQueue;
    xr_vector<Item> UpdateQueue;
    xr_vector<ItemReg> RegistrationQueue;
    bool processingNow;
    SchedulerStatistics stats;

    u64 cyclesStart;
    u64 cyclesLimit;

    void internalRegister(ItemReg&& item);
    bool internalUnregister(ItemReg&& item, const bool warnWhenNotFound = true);
    void processRegistrationQueue();

public:
    Scheduler() : processingNow(false), cyclesStart(0), cyclesLimit(0) {}

    void Initialize();
    void Destroy();
    void DumpStatistics(class ::IGameFont& font, class ::IPerformanceAlert* alert);

    bool Registered(ISheduled* object) const;

    void Register(ISheduled* object, const bool realtime = false);
    void Unregister(ISheduled* object);
    void EnsureOrder(ISheduled* before, ISheduled* after);

    void ProcessStep();
    void ProcessRealtimeQueueQueue();
    void ProcessUpdateQueue();

    const CStatTimer& GetUpdateTime()
    {
        stats.FrameEnd();
        return stats.Update;
    }
};
}
