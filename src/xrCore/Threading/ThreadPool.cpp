#include "stdafx.h"
#include "Threading/ThreadPool.hpp"
#ifdef _GPA_ENABLED
#include <tal.h>
#endif

/*
* Basic C++11 based thread pool with per-thread job queues
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

Thread::Thread()
{
    worker = std::thread(&Thread::queueLoop, this);
}

Thread::~Thread()
{
    if (worker.joinable())
    {
        wait();
        queueMutex.lock();
        destroying = true;
        condition.notify_one();
        queueMutex.unlock();
        worker.join();
    }
}

void Thread::addJob(std::function<void()> function)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    jobQueue.push(std::move(function));
    condition.notify_one();
}

void Thread::wait()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    condition.wait(lock, [this]() { return jobQueue.empty(); });
}

void Thread::queueLoop()
{
    while (true)
    {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !jobQueue.empty() || destroying; });
            if (destroying)
            {
                break;
            }
            job = jobQueue.front();
        }

        job();

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            jobQueue.pop();
            condition.notify_one();
        }
    }
}

XRCORE_API ThreadPool ttapi;
