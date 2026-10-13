#pragma once

#include "xrCore/xrCore.h"
#include "xrCommon/xr_unordered_map.h"
#include "xrEngine/IRenderBackend.h"

#include <mutex>
#include <nvrhi/nvrhi.h>

namespace xray::render::backend {

class SubmissionTracker
{
public:
    static constexpr u32 QUEUE_COUNT = u32(nvrhi::CommandQueue::Count);

    void Initialize(nvrhi::IDevice* device);
    void Shutdown();

    u64 RegisterTicket(nvrhi::CommandQueue queue);
    void ArmTicket(u64 ticket);
    void CancelTicket(u64 ticket);

    u64 OpenLease();
    void CloseOpenLeases();
    void DiscardOpenLeases();
    void CloseLease(u64 lease);
    void ReleaseLease(u64 lease);
    IRenderBackend::SubmissionLeaseState PollLease(u64 lease);
    IRenderBackend::SubmissionLeaseState PeekLease(u64 lease) const;
    bool IsLeaseSubmitted(u64 lease) const;

    u32 PendingTicketCount() const;
    u32 OpenLeaseCount() const;

private:
    enum class TicketState : u8
    {
        Pending,
        Armed,
        Canceled,
        Complete
    };

    class Ticket
    {
    public:
        u64 seq = 0;
        u32 slot = UINT32_MAX;
        TicketState state = TicketState::Pending;
    };

    class QueueState
    {
    public:
        xr_vector<Ticket> tickets;
        u32 head = 0;
        u64 issuedSeq = 0;
        u64 completedSeq = 0;
    };

    class QuerySlot
    {
    public:
        nvrhi::EventQueryHandle query;
        bool busy = false;
    };

    class Lease
    {
    public:
        u64 openSeq[QUEUE_COUNT] = {};
        u64 requiredSeq[QUEUE_COUNT] = {};
        bool closed = false;
        bool failed = false;
    };

    u32 AcquireSlotLocked(u32 queueIndex);
    void AdvanceLocked(u32 queueIndex);
    void AdvanceAllLocked();
    void MarkFailureLocked(u32 queueIndex, u64 seq);
    Ticket* FindTicketLocked(u32 queueIndex, u64 seq);
    IRenderBackend::SubmissionLeaseState EvaluateLeaseLocked(const Lease& lease) const;

    mutable std::mutex m_mutex;
    nvrhi::IDevice* m_device = nullptr;

    QueueState m_queues[QUEUE_COUNT];
    xr_vector<QuerySlot> m_slots[QUEUE_COUNT];

    xr_unordered_map<u64, Lease> m_leases;
    u64 m_nextLease = 1;
};

}
