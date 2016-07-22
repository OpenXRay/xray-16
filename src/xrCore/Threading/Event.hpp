#pragma once
#include "xrCore/_types.h"

class XRCORE_API Event
{
private:
    void* handle;

public:
    Event() throw();
    ~Event() throw();

    // Reset the event to the unsignalled state.
    void Reset() throw();
    // Set the event to the signalled state.
    void Set() throw();
    // Wait indefinitely for the object to become signalled.
    void Wait() const throw();
    // Wait, with a time limit, for the object to become signalled.
    bool Wait(u32 millisecondsTimeout) const throw();

    void* GetHandle() const throw() { return handle; }
};
