#pragma once
#include "xrCore/xr_types.h"

class XRCORE_API Event
{
    void* handle;
#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_SWITCH)
    struct EventHandle
    {
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        bool signaled;
    };

private:
    EventHandle m_id;
#endif

public:
    Event() noexcept;
    Event(std::nullptr_t) noexcept;
    Event(void* event) noexcept;
    ~Event() noexcept;

    // Reset the event to the unsignalled state.
    void Reset() noexcept;
    // Set the event to the signalled state.
    void Set() noexcept;
    // Wait indefinitely for the object to become signalled.
    void Wait() noexcept;
    /*! \brief Wait, with a time limit, for the object to become signalled

        \return True if the object becomes signalled in the time limit, false otherwise
    */
    bool Wait(u32 millisecondsTimeout) noexcept;

    void* GetHandle() const noexcept { return handle; }
    bool Valid() const noexcept { return handle != nullptr; }
};
