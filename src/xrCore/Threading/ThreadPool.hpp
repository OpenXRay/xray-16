#pragma once
#include "xrCore/xrCore.h"

/*
* Basic C++11 based thread pool with per-thread job queues
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "xrCommon/xr_smart_pointers.h"

class XRCORE_API Thread
{
    bool destroying = false;
    std::thread worker;
    std::queue<std::function<void()>> jobQueue;
    std::mutex queueMutex;
    std::condition_variable condition;

    // Loop through all remaining jobs
    void queueLoop();

public:
    Thread();
    ~Thread();

    // Add a new job to the thread's queue
    void addJob(std::function<void()> function);

    // Wait until all work items have been finished
    void wait();
};

class ThreadPool
{
public:
    xr_vector<xr_unique_ptr<Thread>> threads;

    void initialize()
    {
        const int num_threads = std::thread::hardware_concurrency();
        R_ASSERT(num_threads > 0);
        setThreadCount(num_threads);
    }

    void destroy()
    {
        wait();
        threads.clear();
    }

    // Sets the number of threads to be allocated in this pool
    void setThreadCount(const uint32_t count)
    {
        threads.clear();
        threads.reserve(count);
        for (auto i = 0; i < count; i++)
            threads.emplace_back(std::make_unique<Thread>());
    }

    // Wait until all threads have finished their work items
    void wait()
    {
        for (auto &thread : threads)
            thread->wait();
    }
};

extern XRCORE_API ThreadPool ttapi;
