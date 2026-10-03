#include "stdafx.h"
#include "xrSheduler.h"
#include "xr_object.h"
#include "GameFont.h"
#include "PerformanceAlert.hpp"

//#define DEBUG_SCHEDULER
//#define DEBUG_SCHEDULERMT

#if defined(XR_PLATFORM_ANDROID)
float psShedulerBudget = 10.f;
#else
float psShedulerBudget = 66.f;
#endif
float psShedulerCurrent = 10.f;
float psShedulerTarget = 10.f;
const float psShedulerReaction = 0.1f;
bool isSheduleInProgress = false;

//-------------------------------------------------------------------------------------
void CSheduler::Initialize()
{
    m_current_step_obj = nullptr;
    m_processing_now = false;
}

void CSheduler::Destroy()
{
    internal_Registration();

    for (u32 it = 0; it < Items.size(); it++)
    {
        if (nullptr == Items[it].Object)
        {
            Items.erase(Items.begin() + it);
            it--;
        }
    }
#ifdef DEBUG
    if (!Items.empty())
    {
        Msg("! Sheduler work-list is not empty");
        for (const auto& item : Items)
            Log(item.Object->shedule_Name().c_str());
    }
#endif

    ItemsRT.clear();
    Items.clear();
    ItemsProcessed.clear();
    Registration.clear();
}

void CSheduler::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    stats.FrameEnd();
    const float percentage = 100.f * stats.Update.result / Device.GetStats().EngineTotal.result;
    font.OutNext("Object Scheduler:");
    font.OutNext("- update:     %2.2fms, %2.1f%%", stats.Update.result, percentage);
    font.OutNext("- load:       %2.2fms", stats.Load);
    if (alert && stats.Update.result > 3.0f)
        alert->Print(font, "Update    > 3ms:  %3.1f", stats.Update.result);
    stats.FrameStart();
}

void CSheduler::internal_Registration()
{
    for (u32 it = 0; it < Registration.size(); it++)
    {
        ItemReg& R = Registration[it];
        if (R.OP)
        {
            // register
            // search for paired "unregister"
            bool foundAndErased = false;
            for (u32 pair = it + 1; pair < Registration.size(); pair++)
            {
                ItemReg& R_pair = Registration[pair];
                if (!R_pair.OP && R_pair.Object == R.Object)
                {
                    foundAndErased = true;
                    Registration.erase(Registration.begin() + pair);
                    break;
                }
            }

            // register if non-paired
            if (!foundAndErased)
            {
#ifdef DEBUG_SCHEDULER
                Msg("SCHEDULER: internal register [%s][%x][%s]", R.Object->shedule_Name().c_str(), R.Object,
                    R.RT ? "true" : "false");
#endif
                internal_Register(R.Object, R.RT);
            }
#ifdef DEBUG_SCHEDULER
            else
                Msg("SCHEDULER: internal register skipped, because unregister found [%s][%x][%s]", "unknown", R.Object,
                    R.RT ? "true" : "false");
#endif
        }
        else
        {
            // unregister
            internal_Unregister(R.Object, R.RT);
        }
    }
    Registration.clear();
}

void CSheduler::internal_Register(ISheduled* object, bool realTime)
{
    VERIFY(!object->GetSchedulerData().b_locked);

    // Fill item structure
    Item item;
    item.dwTimeForExecute = Device.dwTimeGlobal;
    item.dwTimeOfLastExecute = Device.dwTimeGlobal;
    item.scheduled_name = object->shedule_Name();
    item.Object = object;

    if (realTime)
    {
        object->GetSchedulerData().b_RT = true;
        ItemsRT.emplace_back(std::move(item));
    }
    else
    {
        object->GetSchedulerData().b_RT = false;

        // Insert into priority Queue
        Push(item);
    }
}

bool CSheduler::internal_Unregister(ISheduled* object, bool realTime, bool warn_on_not_found)
{
    // the object may be already dead
    // VERIFY (!O->shedule.b_locked);
    if (realTime)
    {
        for (u32 i = 0; i < ItemsRT.size(); i++)
        {
            if (ItemsRT[i].Object == object)
            {
#ifdef DEBUG_SCHEDULER
                Msg("SCHEDULER: internal unregister [%s][%x][%s]", "unknown", object, "true");
#endif
                ItemsRT.erase(ItemsRT.begin() + i);
                return true;
            }
        }
    }
    else
    {
        for (auto& item : Items)
        {
            if (item.Object == object)
            {
#ifdef DEBUG_SCHEDULER
                Msg("SCHEDULER: internal unregister [%s][%x][%s]", item.scheduled_name.c_str(), object, "false");
#endif
                item.Object = nullptr;
                return true;
            }
        }
    }
    if (m_current_step_obj == object)
    {
#ifdef DEBUG_SCHEDULER
        Msg("SCHEDULER: internal unregister (self unregistering) [%x][%s]", object, "false");
#endif

        m_current_step_obj = nullptr;
        return true;
    }

#ifdef DEBUG
    if (warn_on_not_found)
        Msg("! scheduled object %s tries to unregister but is not registered", object->shedule_Name().c_str());
#endif

    return false;
}

#ifndef MASTER_GOLD
bool CSheduler::Registered(ISheduled* object) const
{
    u32 count = 0;

    for (const auto& it : ItemsRT)
    {
        if (it.Object == object)
        {
            // Msg ("0x%8x found in RT",object);
            count = 1;
            break;
        }
    }

    for (const auto& it : Items)
    {
        if (it.Object == object)
        {
            // Msg ("0x%8x found in non-RT",object);
            VERIFY(!count);
            count = 1;
            break;
        }
    }

    for (const auto& it : ItemsProcessed)
    {
        if (it.Object == object)
        {
            // Msg ("0x%8x found in process items",object);
            VERIFY(!count);
            count = 1;
            break;
        }
    }

    for (const auto& it : Registration)
    {
        if (it.Object == object)
        {
            if (it.OP)
            {
                // Msg ("0x%8x found in registration on register",object);
                VERIFY(!count);
                ++count;
            }
            else
            {
                // Msg ("0x%8x found in registration on UNregister",object);
                VERIFY(count == 1);
                --count;
            }
        }
    }

    if (!count && m_current_step_obj == object)
    {
        VERIFY2(m_processing_now, "trying to unregister self unregistering object while not processing now");
        count = 1;
    }
    VERIFY(!count || count == 1);
    return count == 1;
}
#endif // !MASTER_GOLD

void CSheduler::Register(ISheduled* A, bool RT)
{
#ifndef MASTER_GOLD
    VERIFY(!Registered(A));
#endif

    ItemReg R;
    R.OP = true;
    R.RT = RT;
    R.Object = A;
    R.Object->GetSchedulerData().b_RT = RT;

#ifdef DEBUG_SCHEDULER
    Msg("SCHEDULER: register [%s][%x]", A->shedule_Name().c_str(), A);
#endif

    Registration.push_back(R);
}

void CSheduler::Unregister(ISheduled* A)
{
#ifndef MASTER_GOLD
    VERIFY(Registered(A));
#endif

#ifdef DEBUG_SCHEDULER
    Msg("SCHEDULER: unregister [%s][%x]", A->shedule_Name().c_str(), A);
#endif

    if (m_processing_now)
    {
        if (internal_Unregister(A, A->GetSchedulerData().b_RT, false))
            return;
    }

    ItemReg R;
    R.OP = false;
    R.RT = A->GetSchedulerData().b_RT;
    R.Object = A;

    Registration.push_back(R);
}

void CSheduler::EnsureOrder(ISheduled* Before, ISheduled* After)
{
    VERIFY(Before->GetSchedulerData().b_RT && After->GetSchedulerData().b_RT);

    for (u32 i = 0; i < ItemsRT.size(); i++)
    {
        if (ItemsRT[i].Object == After)
        {
            Item A = ItemsRT[i];
            ItemsRT.erase(ItemsRT.begin() + i);
            ItemsRT.push_back(A);
            return;
        }
    }
}

void CSheduler::Push(Item& item)
{
    Items.emplace_back(std::move(item));
    std::push_heap(Items.begin(), Items.end());
}

void CSheduler::Pop()
{
    std::pop_heap(Items.begin(), Items.end());
    Items.pop_back();
}

void CSheduler::ProcessStep()
{
    ZoneScoped;

#if defined(XR_PLATFORM_ANDROID)
    static u32 lastSchedulerProfile = 0;
    const bool profileScheduler = Device.dwTimeContinual - lastSchedulerProfile >= 5000;
    const u64 profileStart = profileScheduler ? CPU::QPC() : 0;
    u64 neededCycles = 0, updateCycles = 0, slowestCycles = 0;
    u32 profiledObjects = 0, maximumLateness = 0;
    shared_str slowestName;
#endif

    // Normal priority
    const u32 dwTime = Device.dwTimeGlobal;

#ifdef DEBUG
    CTimer eTimer;
#endif

    for (int i = 0; !Items.empty() && Top().dwTimeForExecute < dwTime; ++i)
    {
        if (i > 0 && Device.dwPrecacheFrame == 0 && CPU::QPC() > cycles_limit)
        {
            psShedulerTarget += psShedulerReaction * 3;
            break;
        }
        // Update
#if defined(XR_PLATFORM_ANDROID)
        const u64 beforeNeeded = profileScheduler ? CPU::QPC() : 0;
#endif
        const bool needed = Top().Object && Top().Object->shedule_Needed();
#if defined(XR_PLATFORM_ANDROID)
        if (profileScheduler)
        {
            neededCycles += CPU::QPC() - beforeNeeded;
            maximumLateness = _max(maximumLateness, dwTime - Top().dwTimeForExecute);
        }
#endif
        if (!needed)
        {
#ifdef DEBUG_SCHEDULER
            Msg("SCHEDULER: process unregister [%s][%x][%s]", Top().scheduled_name.c_str(), Top().Object, "false");
#endif
            // Erase element
            Pop();
            continue;
        }

        std::pop_heap(Items.begin(), Items.end());
        Item item = std::move(Items.back());
        Items.pop_back();
        if (!item.Object)
            continue;
        auto& schedulerData = item.Object->GetSchedulerData();

#ifdef DEBUG_SCHEDULER
        Msg("SCHEDULER: process step [%s][%x][false]", item.scheduled_name.c_str(), item.Object);
#endif

        u32 Elapsed = dwTime - item.dwTimeOfLastExecute;

        // Real update call
        // Msg("------- %d:", Device.dwFrame);
#ifdef DEBUG
        schedulerData.dbg_startframe = Device.dwFrame;
        eTimer.Start();
#endif

        // Calc next update interval
        const u32 dwMin = _max(u32(30), schedulerData.t_min);
        u32 dwMax = (1000 + schedulerData.t_max) / 2;
        const float scale = item.Object->shedule_Scale();
        u32 dwUpdate = dwMin + iFloor(float(dwMax - dwMin) * scale);
        clamp(dwUpdate, u32(_max(dwMin, u32(20))), dwMax);

        m_current_step_obj = item.Object;

#if defined(XR_PLATFORM_ANDROID)
        const u64 beforeUpdate = profileScheduler ? CPU::QPC() : 0;
#endif
        item.Object->shedule_Update(
            clampr(Elapsed, u32(1), u32(_max(u32(schedulerData.t_max), u32(1000)))));
#if defined(XR_PLATFORM_ANDROID)
        if (profileScheduler)
        {
            const u64 elapsed = CPU::QPC() - beforeUpdate;
            updateCycles += elapsed;
            ++profiledObjects;
            if (elapsed > slowestCycles)
            {
                slowestCycles = elapsed;
                slowestName = item.scheduled_name;
            }
        }
#endif
        if (!m_current_step_obj)
        {
#ifdef DEBUG_SCHEDULER
            Msg("SCHEDULER: process unregister (self unregistering) [%s][%x][%s]", item.scheduled_name.c_str(), item.Object,
                "false");
#endif
            continue;
        }

        m_current_step_obj = nullptr;

        // Fill item structure
        item.dwTimeForExecute = dwTime + dwUpdate;
        item.dwTimeOfLastExecute = dwTime;
        ItemsProcessed.emplace_back(std::move(item));

#if 0 //def DEBUG
        auto itemName = item.Object->shedule_Name().c_str();
        const u32 delta_ms = dwTime - item.dwTimeForExecute;
        const u32 execTime = eTimer.GetElapsed_ms();
        VERIFY3(item.Object->dbg_update_shedule == item.Object->dbg_startframe,
            "Broken sequence of calls to 'shedule_Update'", itemName);

        if (delta_ms > 3 * dwUpdate)
            Msg("! xrSheduler: failed to shedule object [%s] (%dms)", itemName, delta_ms);

        if (execTime > 15)
            Msg("* xrSheduler: too much time consumed by object [%s] (%dms)", itemName, execTime);
#endif
    }

    // Rebuilding the heap is cheaper than a push_heap per object when a
    // substantial fraction of scheduled objects ran in the same frame.
    // Both paths preserve the heap's due-time ordering and process each
    // object at most once per frame.
#if defined(XR_PLATFORM_ANDROID)
    if (ItemsProcessed.size() > 32 && ItemsProcessed.size() > Items.size() / 4)
    {
        Items.reserve(Items.size() + ItemsProcessed.size());
        for (auto& processed : ItemsProcessed)
            Items.emplace_back(std::move(processed));
        ItemsProcessed.clear();
        std::make_heap(Items.begin(), Items.end());
    }
    else while (!ItemsProcessed.empty())
#else
    while (!ItemsProcessed.empty())
#endif
    {
        Push(ItemsProcessed.back());
        ItemsProcessed.pop_back();
    }

    // always try to decrease target
    psShedulerTarget -= psShedulerReaction;
#if defined(XR_PLATFORM_ANDROID)
    if (profileScheduler)
    {
        const u64 totalCycles = CPU::QPC() - profileStart;
        const double ms = 1000.0 / double(CPU::qpc_freq);
        Msg("[scheduler-profile] frame=%u due=%u total=%.2fms needed=%.2fms update=%.2fms "
            "other=%.2fms slowest=%s:%.2fms budget=%.2fms target=%.2fms late=%ums remaining=%zu",
            Device.dwFrame, profiledObjects, totalCycles * ms, neededCycles * ms,
            updateCycles * ms, (totalCycles - neededCycles - updateCycles) * ms,
            slowestName.c_str(), slowestCycles * ms, psShedulerBudget, psShedulerCurrent,
            maximumLateness, Items.size());
        lastSchedulerProfile = Device.dwTimeContinual;
    }
#endif
}

void CSheduler::Update()
{
    ZoneScoped;
#if defined(XR_PLATFORM_ANDROID)
    const u64 updateStart = CPU::QPC();
#endif

    // Initialize
    stats.Update.Begin();
    cycles_start = CPU::QPC();
    const double budgetMs = _min(double(psShedulerCurrent), double(psShedulerBudget));
    cycles_limit = cycles_start + u64(double(CPU::qpc_freq) * budgetMs / 1000.0);
    internal_Registration();
    isSheduleInProgress = true;

#ifdef DEBUG_SCHEDULER
    Msg("SCHEDULER: PROCESS STEP %d", Device.dwFrame);
#endif
    // Realtime priority
    m_processing_now = true;
    const u32 dwTime = Device.dwTimeGlobal;
    for (auto& item : ItemsRT)
    {
        R_ASSERT(item.Object);
#ifdef DEBUG_SCHEDULER
        Msg("SCHEDULER: process step [%s][%x][true]", item.Object->shedule_Name().c_str(), item.Object);
#endif
        if (!item.Object->shedule_Needed())
        {
#ifdef DEBUG_SCHEDULER
            Msg("SCHEDULER: process unregister [%s][%x][%s]", item.Object->shedule_Name().c_str(), item.Object, "false");
#endif
            item.dwTimeOfLastExecute = dwTime;
            continue;
        }

        const u32 Elapsed = dwTime - item.dwTimeOfLastExecute;
#ifdef DEBUG
        VERIFY(item.Object->GetSchedulerData().dbg_startframe != Device.dwFrame);
        item.Object->GetSchedulerData().dbg_startframe = Device.dwFrame;
#endif
        item.Object->shedule_Update(Elapsed);
        item.dwTimeOfLastExecute = dwTime;
    }

    // Normal (sheduled)
    ProcessStep();
    m_processing_now = false;
#ifdef DEBUG_SCHEDULER
    Msg("SCHEDULER: PROCESS STEP FINISHED %d", Device.dwFrame);
#endif
    clamp(psShedulerTarget, 3.f, psShedulerBudget);
    psShedulerCurrent = 0.9f * psShedulerCurrent + 0.1f * psShedulerTarget;
    stats.Load = psShedulerCurrent;

    // Finalize
    isSheduleInProgress = false;
    internal_Registration();
    stats.Update.End();
#if defined(XR_PLATFORM_ANDROID)
    static u64 updateTotal = 0;
    static u64 updateMaximum = 0;
    static u32 updateSamples = 0;
    static u32 lastReport = 0;
    const u64 elapsed = CPU::QPC() - updateStart;
    updateTotal += elapsed;
    updateMaximum = std::max(updateMaximum, elapsed);
    ++updateSamples;
    if (Device.dwTimeContinual - lastReport >= 5000 && CPU::qpc_freq)
    {
        const double tickToMs = 1000.0 / CPU::qpc_freq;
        Msg("[update-trace] scheduler=%.2fms max=%.2fms samples=%u scheduled=%zu realtime=%zu",
            tickToMs * updateTotal / updateSamples, tickToMs * updateMaximum,
            updateSamples, Items.size(), ItemsRT.size());
        updateTotal = updateMaximum = 0;
        updateSamples = 0;
        lastReport = Device.dwTimeContinual;
    }
#endif
}
