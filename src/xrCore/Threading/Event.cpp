#include "stdafx.h"
#include "Event.hpp"

Event::Event() noexcept
{
    signaled = false;
    mutex = SDL_CreateMutex();
    cond = SDL_CreateCond();
}
Event::~Event() noexcept
{
    SDL_DestroyMutex(mutex);
    SDL_DestroyCond(cond);
}
void Event::Reset() noexcept
{
    SDL_LockMutex(mutex);
    SDL_CondSignal(cond);
    signaled = false;
    SDL_UnlockMutex(mutex);
}
void Event::Set() noexcept
{
    SDL_LockMutex(mutex);
    SDL_CondSignal(cond);
    signaled = true;
    SDL_UnlockMutex(mutex);
}
void Event::Wait() noexcept
{
    SDL_LockMutex(mutex);

    while (!signaled)
    {
        SDL_CondWait(cond, mutex);
    }
    signaled = false; // due in WaitForSingleObject() "Before returning, a wait function modifies the state of some types of synchronization"

    SDL_UnlockMutex(mutex);
}
bool Event::Wait(u32 millisecondsTimeout) noexcept
{
    bool result = false;
    SDL_LockMutex(mutex);

    while(!signaled)
    {
        int res = SDL_CondWaitTimeout(cond, mutex, millisecondsTimeout);

        if (res == SDL_MUTEX_TIMEDOUT)
        {
            result = true;
            break;
        }
    }
    signaled = false;

    SDL_UnlockMutex(mutex);

    return result;
}
