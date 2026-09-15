#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

struct SubmitTokenRing {
    static constexpr u32 SIZE = 256;
    static constexpr u32 MAX_WAITS = 8;

    struct Slot {
        u32 token = 0;
        nvrhi::CommandQueue queue = nvrhi::CommandQueue::Graphics;
        u64 id = 0;
    };

    Slot slots[SIZE];
    u32 next = 1;
    u32 last[u32(nvrhi::CommandQueue::Count)] = {};

    u32 Issue(nvrhi::CommandQueue queue) {
        const u32 token = next++;
        if (next == 0)
            next = 1;
        last[u32(queue)] = token;
        return token;
    }

    void Resolve(u32 token, nvrhi::CommandQueue queue, u64 id) {
        Slot& slot = slots[token % SIZE];
        slot.token = token;
        slot.queue = queue;
        slot.id = id;
    }

    void WaitFor(nvrhi::IDevice* device, nvrhi::CommandQueue waitQueue, u32 token) const {
        const Slot& slot = slots[token % SIZE];
        R_ASSERT2(slot.token == token, "GPU submit token expired before it was waited on");
        if (slot.queue != waitQueue)
            device->queueWaitForCommandList(waitQueue, slot.queue, slot.id);
    }
};

struct SubmitWaitList {
    u32 tokens[SubmitTokenRing::MAX_WAITS] = {};
    u32 count = 0;

    void Add(u32 token) {
        if (!token)
            return;
        for (u32 i = 0; i < count; ++i)
            if (tokens[i] == token)
                return;
        R_ASSERT2(count < SubmitTokenRing::MAX_WAITS, "too many GPU submit waits");
        tokens[count++] = token;
    }

    void Clear() { count = 0; }
};
