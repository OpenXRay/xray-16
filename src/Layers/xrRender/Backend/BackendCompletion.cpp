#include "stdafx.h"
#include "BackendCompletion.h"

namespace xray::render::backend {

namespace {

constexpr u32 kTicketQueueShift = 56;

u32 TicketQueue(u64 ticket)
{
    return u32(ticket >> kTicketQueueShift) - 1u;
}

u64 TicketSeq(u64 ticket)
{
    return ticket & ((1ull << kTicketQueueShift) - 1ull);
}

u64 MakeTicket(u32 queueIndex, u64 seq)
{
    return (u64(queueIndex + 1u) << kTicketQueueShift) | seq;
}

}

void SubmissionTracker::Initialize(nvrhi::IDevice* device)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_device = device;
    for (u32 q = 0; q < QUEUE_COUNT; ++q) {
        m_queues[q] = QueueState();
        m_slots[q].clear();
    }
    m_leases.clear();
    m_nextLease = 1;
}

void SubmissionTracker::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (u32 q = 0; q < QUEUE_COUNT; ++q) {
        for (QuerySlot& slot : m_slots[q]) {
            if (slot.query && m_device)
                m_device->resetEventQuery(slot.query);
            slot.query = nullptr;
            slot.busy = false;
        }
        m_slots[q].clear();
        m_queues[q] = QueueState();
    }
    m_leases.clear();
    m_device = nullptr;
}

u64 SubmissionTracker::RegisterTicket(nvrhi::CommandQueue queue)
{
    const u32 queueIndex = u32(queue);
    if (queueIndex >= QUEUE_COUNT)
        return 0;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_device)
        return 0;

    AdvanceLocked(queueIndex);
    const u32 slot = AcquireSlotLocked(queueIndex);
    QueueState& state = m_queues[queueIndex];
    if (state.issuedSeq == ((1ull << kTicketQueueShift) - 1ull))
        FATAL("[Backend] submission completion sequence exhausted");
    Ticket ticket;
    ticket.seq = ++state.issuedSeq;
    ticket.slot = slot;
    ticket.state = TicketState::Pending;
    state.tickets.push_back(ticket);
    return MakeTicket(queueIndex, ticket.seq);
}

SubmissionTracker::Ticket* SubmissionTracker::FindTicketLocked(u32 queueIndex, u64 seq)
{
    QueueState& state = m_queues[queueIndex];
    if (state.tickets.empty() || seq < state.tickets.front().seq)
        return nullptr;
    const u64 index = seq - state.tickets.front().seq;
    if (index < state.head || index >= state.tickets.size())
        return nullptr;
    return &state.tickets[size_t(index)];
}

u32 SubmissionTracker::AcquireSlotLocked(u32 queueIndex)
{
    xr_vector<QuerySlot>& slots = m_slots[queueIndex];
    for (u32 i = 0; i < u32(slots.size()); ++i)
    {
        if (slots[i].busy)
            continue;
        if (!slots[i].query)
            slots[i].query = m_device->createEventQuery();
        if (!slots[i].query)
            FATAL("[Backend] cannot allocate a submission completion query");
        slots[i].busy = true;
        return i;
    }

    if (slots.size() >= UINT32_MAX)
        FATAL("[Backend] submission completion query index exhausted");
    QuerySlot slot;
    slot.query = m_device->createEventQuery();
    if (!slot.query)
        FATAL("[Backend] cannot allocate a submission completion query");
    slot.busy = true;
    slots.push_back(slot);
    return u32(slots.size()) - 1u;
}

void SubmissionTracker::ArmTicket(u64 ticket)
{
    if (ticket == 0)
        return;
    const u32 queueIndex = TicketQueue(ticket);
    if (queueIndex >= QUEUE_COUNT)
        return;
    const u64 seq = TicketSeq(ticket);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_device)
        FATAL("[Backend] submission completed after tracker shutdown");

    Ticket* entry = FindTicketLocked(queueIndex, seq);
    if (!entry || entry->state != TicketState::Pending || entry->slot == UINT32_MAX)
        FATAL("[Backend] submitted command list has no pending completion ticket");

    QuerySlot& slot = m_slots[queueIndex][entry->slot];
    m_device->resetEventQuery(slot.query);
    m_device->setEventQuery(slot.query, nvrhi::CommandQueue(queueIndex));
    entry->state = TicketState::Armed;
}

void SubmissionTracker::CancelTicket(u64 ticket)
{
    if (ticket == 0)
        return;
    const u32 queueIndex = TicketQueue(ticket);
    if (queueIndex >= QUEUE_COUNT)
        return;
    const u64 seq = TicketSeq(ticket);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (seq == 0 || seq > m_queues[queueIndex].issuedSeq)
        return;
    MarkFailureLocked(queueIndex, seq);
    Ticket* entry = FindTicketLocked(queueIndex, seq);
    if (!entry || entry->state == TicketState::Complete || entry->state == TicketState::Canceled)
        return;

    if (entry->state == TicketState::Pending)
    {
        QuerySlot& slot = m_slots[queueIndex][entry->slot];
        slot.busy = false;
        entry->slot = UINT32_MAX;
        entry->state = TicketState::Canceled;
    }
    AdvanceLocked(queueIndex);
}

void SubmissionTracker::AdvanceLocked(u32 queueIndex)
{
    QueueState& state = m_queues[queueIndex];
    if (!m_device)
        return;

    for (u32 i = state.head; i < u32(state.tickets.size()); ++i)
    {
        Ticket& ticket = state.tickets[i];
        if (ticket.state != TicketState::Armed)
            continue;
        QuerySlot& slot = m_slots[queueIndex][ticket.slot];
        if (!m_device->pollEventQuery(slot.query))
            continue;
        m_device->resetEventQuery(slot.query);
        slot.busy = false;
        ticket.slot = UINT32_MAX;
        ticket.state = TicketState::Complete;
    }

    while (state.head < u32(state.tickets.size()))
    {
        const Ticket& ticket = state.tickets[state.head];
        if (ticket.state != TicketState::Complete && ticket.state != TicketState::Canceled)
            break;
        state.completedSeq = ticket.seq;
        ++state.head;
    }

    if (state.head == u32(state.tickets.size()))
    {
        state.tickets.clear();
        state.head = 0;
    }
    else if (state.head != 0 && state.head >= state.tickets.size() / 2)
    {
        state.tickets.erase(state.tickets.begin(), state.tickets.begin() + state.head);
        state.head = 0;
    }
}

void SubmissionTracker::MarkFailureLocked(u32 queueIndex, u64 seq)
{
    for (auto& pair : m_leases)
    {
        Lease& lease = pair.second;
        if (seq > lease.openSeq[queueIndex]
            && (!lease.closed || seq <= lease.requiredSeq[queueIndex]))
            lease.failed = true;
    }
}

u64 SubmissionTracker::OpenLease()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_device)
        return 0;

    const u64 id = m_nextLease++;
    Lease lease;
    for (u32 q = 0; q < QUEUE_COUNT; ++q) {
        lease.openSeq[q] = m_queues[q].issuedSeq;
        lease.requiredSeq[q] = m_queues[q].issuedSeq;
    }
    m_leases.emplace(id, lease);
    return id;
}

void SubmissionTracker::CloseOpenLeases()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_leases) {
        if (pair.second.closed)
            continue;
        for (u32 q = 0; q < QUEUE_COUNT; ++q)
            pair.second.requiredSeq[q] = m_queues[q].issuedSeq;
        pair.second.closed = true;
    }
}

void SubmissionTracker::DiscardOpenLeases()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_leases)
    {
        if (pair.second.closed)
            continue;
        for (u32 q = 0; q < QUEUE_COUNT; ++q)
            pair.second.requiredSeq[q] = m_queues[q].issuedSeq;
        pair.second.failed = true;
        pair.second.closed = true;
    }
}

void SubmissionTracker::CloseLease(u64 lease)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_leases.find(lease);
    if (it == m_leases.end() || it->second.closed)
        return;
    for (u32 q = 0; q < QUEUE_COUNT; ++q)
        it->second.requiredSeq[q] = m_queues[q].issuedSeq;
    it->second.closed = true;
}

void SubmissionTracker::ReleaseLease(u64 lease)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_leases.erase(lease);
}

IRenderBackend::SubmissionLeaseState SubmissionTracker::PollLease(u64 lease)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_leases.find(lease);
    if (it == m_leases.end())
        return IRenderBackend::SubmissionLeaseState::Unknown;
    if (!it->second.closed)
        return IRenderBackend::SubmissionLeaseState::Open;

    for (u32 q = 0; q < QUEUE_COUNT; ++q)
    {
        AdvanceLocked(q);
        if (m_queues[q].completedSeq < it->second.requiredSeq[q])
            return IRenderBackend::SubmissionLeaseState::Pending;
    }
    return it->second.failed ? IRenderBackend::SubmissionLeaseState::Failed
                            : IRenderBackend::SubmissionLeaseState::Complete;
}

u32 SubmissionTracker::PendingTicketCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    u32 count = 0;
    for (u32 q = 0; q < QUEUE_COUNT; ++q) {
        const QueueState& state = m_queues[q];
        for (u32 i = state.head; i < u32(state.tickets.size()); ++i) {
            if (state.tickets[i].state == TicketState::Pending
                || state.tickets[i].state == TicketState::Armed)
                ++count;
        }
    }
    return count;
}

u32 SubmissionTracker::OpenLeaseCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return u32(m_leases.size());
}

}
