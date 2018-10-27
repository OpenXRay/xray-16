#pragma once
#include "xrCore/_types.h"

class XRCORE_API Event
{
    void* handle;

private:
    SDL_mutex* mutex;
    SDL_cond* cond;
    bool signaled;

public:
    Event() noexcept;
    ~Event() noexcept;

    // Reset the event to the unsignalled state.
    void Reset() noexcept;
    // Set the event to the signalled state.
    void Set() noexcept;
    // Wait indefinitely for the object to become signalled.
    void Wait() noexcept;
    // Wait, with a time limit, for the object to become signalled.
    bool Wait(u32 millisecondsTimeout) noexcept;

    void* GetHandle() noexcept { return handle; }
};
