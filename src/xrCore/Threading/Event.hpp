#pragma once
#include "xrCore/_types.h"

class XRCORE_API Event
{
private:
    void* handle;

public:
    Event() noexcept;
    ~Event() noexcept;

    // Reset the event to the unsignalled state.
    void Reset() noexcept;
    // Set the event to the signalled state.
    void Set() noexcept;
    // Wait indefinitely for the object to become signalled.
    void Wait() const noexcept;
    // Wait, with a time limit, for the object to become signalled.
    bool Wait(u32 millisecondsTimeout) const noexcept;

    void* GetHandle() const noexcept { return handle; }
};
