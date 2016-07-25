#include "stdafx.h"
#include "Event.hpp"
#include <windows.h>

Event::Event() noexcept { handle = (void*)CreateEvent(NULL, FALSE, FALSE, NULL); }
Event::~Event() noexcept { CloseHandle(handle); }
void Event::Reset() noexcept { ResetEvent(handle); }
void Event::Set() noexcept { SetEvent(handle); }
void Event::Wait() const noexcept { WaitForSingleObject(handle, INFINITE); }
bool Event::Wait(u32 millisecondsTimeout) const noexcept
{
    return WaitForSingleObject(handle, millisecondsTimeout) != WAIT_TIMEOUT;
}
