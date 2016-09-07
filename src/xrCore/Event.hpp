#pragma once
#include "xrCore.h"

class XRCORE_API Event
{
private:
    void *handle;

public:
    Event();
    ~Event();

    // Reset the event to the unsignalled state.
    void Reset();
    // Set the event to the signalled state.
    void Set();
    // Wait indefinitely for the object to become signalled.
    void Wait() const;
    // Wait, with a time limit, for the object to become signalled. 
    bool Wait(u32 millisecondsTimeout) const;

    void *GetHandle() const { return handle; }
};
