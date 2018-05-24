#include "stdafx.h"
#include "Event.hpp"
#if defined(WINDOWS)
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
#elif defined(LINUX)
Event::Event() noexcept { handle = (void*)malloc(1); }
Event::~Event() noexcept { free(handle); }
void Event::Reset() noexcept { memset(handle, 1, 1); }
void Event::Set() noexcept { memset(handle, 0, 1); }
void Event::Wait() const noexcept { Sleep(0); }
bool Event::Wait(u32 millisecondsTimeout) const noexcept
{
	Sleep(millisecondsTimeout);
    return true;
}
#endif
