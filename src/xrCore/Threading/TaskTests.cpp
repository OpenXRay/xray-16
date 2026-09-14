#include "stdafx.h"
#include "Threading/TaskManager.hpp"
#include "Threading/ParallelForEach.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace
{
using namespace std::chrono_literals;

std::atomic<const char*> currentScenario{ "runtime initialization" };

[[noreturn]] void Fail(const char* message, size_t index = size_t(-1))
{
    std::fprintf(stderr, "FAIL [%s]: %s", currentScenario.load(), message);
    if (index != size_t(-1))
        std::fprintf(stderr, " (index %zu)", index);
    std::fputc('\n', stderr);
    std::fflush(stderr);
    std::_Exit(EXIT_FAILURE);
}

void Require(bool condition, const char* message, size_t index = size_t(-1))
{
    if (!condition)
        Fail(message, index);
}

class Signal
{
    std::mutex mutex;
    std::condition_variable changed;
    bool ready = false;

public:
    void Set()
    {
        std::lock_guard lock(mutex);
        ready = true;
        changed.notify_all();
    }

    bool Wait(std::chrono::seconds timeout = 60s)
    {
        std::unique_lock lock(mutex);
        return changed.wait_for(lock, timeout, [this] { return ready; });
    }
};

class Deadline
{
    Signal finished;
    std::thread watchdog;

public:
    Deadline() : watchdog([this]
    {
        if (!finished.Wait())
            Fail("scenario exceeded 60 seconds; scheduler may be deadlocked");
    }) {}

    ~Deadline()
    {
        finished.Set();
        watchdog.join();
    }
};

class SchedulerSession
{
public:
    explicit SchedulerSession(bool spawnWorkers)
    {
        Require(!TaskScheduler, "previous scheduler was not destroyed");
        TaskScheduler = xr_make_unique<TaskManager>();
        if (spawnWorkers)
            TaskScheduler->SpawnThreads();
    }

    ~SchedulerSession()
    {
        TaskScheduler.reset();
    }
};

void NestedHelping()
{
    SchedulerSession scheduler(false);
    TaskHandle empty;
    TaskScheduler->Wait(empty);
    Require(empty.IsFinished(), "null handle did not report completion");

    std::array<int, 32> outputs{};
    auto root = TaskScheduler->AddTask([&outputs](Task& parent)
    {
        for (size_t i = 0; i < outputs.size(); ++i)
        {
            auto child = TaskScheduler->AddTask(parent, [&outputs, i](Task& childTask)
            {
                auto grandchild = TaskScheduler->AddTask(childTask, [&outputs, i]
                {
                    outputs[i] = static_cast<int>(i * 17 + 3);
                });
                TaskScheduler->Wait(grandchild);
                Require(outputs[i] == static_cast<int>(i * 17 + 3), "nested wait returned before output publication", i);
            });
            TaskScheduler->Wait(child);
        }
    });
    TaskScheduler->Wait(root);
    for (size_t i = 0; i < outputs.size(); ++i)
        Require(outputs[i] == static_cast<int>(i * 17 + 3), "nested output was lost", i);
}

void ResetDoesNotJoinOrCancel()
{
    SchedulerSession scheduler(false);
    Signal released;
    int output = 0;
    auto root = TaskScheduler->AddTask([&released, &output](Task& parent)
    {
        auto child = TaskScheduler->AddTask(parent, [&released, &output]
        {
            Require(released.Wait(), "reset joined a child instead of releasing ownership");
            output = 91;
        });
        child.Reset();
        Require(!child, "reset handle still owns a task");
        released.Set();
    });
    TaskScheduler->Wait(root);
    Require(output == 91, "reset cancelled a scheduled child or broke parent completion");
}

struct Lifetime
{
    std::atomic<unsigned> destroyed{};
    Signal released;
};

struct Probe
{
    Lifetime& lifetime;
    explicit Probe(Lifetime& lifetime) : lifetime(lifetime) {}
    ~Probe()
    {
        lifetime.destroyed.fetch_add(1);
        lifetime.released.Set();
    }
};

struct Result
{
    std::unique_ptr<Probe> owner;
    int value;
};

void RetainedResultsAndOwnership()
{
    SchedulerSession scheduler(false);
    Lifetime lifetime;
    auto original = TaskScheduler->AddTask([&lifetime]
    {
        return Result{ std::make_unique<Probe>(lifetime), 137 };
    });
    TaskHandle copy(original);
    TaskHandle assigned;
    assigned = copy;
    TaskHandle moved(std::move(original));
    Require(!original, "move construction did not release the source handle");
    TaskHandle retained;
    retained = std::move(assigned);
    Require(!assigned, "move assignment did not release the source handle");
    copy.Reset();
    moved.Reset();
    TaskScheduler->Wait(retained);
    Require(retained.GetData<Result>() && retained.GetData<Result>()->value == 137, "copied handle lost its result");

    for (size_t i = 0; i < 8193; ++i)
    {
        auto churn = TaskScheduler->AddTask([i] { return i * 7 + 11; });
        TaskScheduler->Wait(churn);
        Require(churn.GetData<size_t>() && *churn.GetData<size_t>() == i * 7 + 11, "subsequent task returned corrupt data", i);
    }

    Require(retained.IsFinished(), "retained completion changed after subsequent tasks");
    const auto* result = retained.GetData<Result>();
    Require(result && result->value == 137 && result->owner, "retained result was overwritten by subsequent tasks");
    Require(lifetime.destroyed.load() == 0, "result was destroyed while a handle retained it");
    TaskScheduler->Wait(retained);
    retained.Reset();
    Require(lifetime.released.Wait(), "last handle release did not destroy its result");
    Require(lifetime.destroyed.load() == 1, "returned resource was not destroyed exactly once");
}

void ConcurrentFanout()
{
    SchedulerSession scheduler(true);
    constexpr size_t branches = 64;
    constexpr size_t children = 128;
    std::array<std::atomic<unsigned>, branches> branchCalls{};
    std::array<std::atomic<unsigned>, branches * children> calls{};
    std::array<size_t, branches * children> outputs{};
    std::atomic<unsigned> rootCalls{};

    auto root = TaskScheduler->AddTask([&](Task& parent)
    {
        Require(rootCalls.fetch_add(1) == 0, "root executed more than once");
        for (size_t branch = 0; branch < branches; ++branch)
        {
            TaskScheduler->AddTask(parent, [&, branch](Task& branchTask)
            {
                Require(branchCalls[branch].fetch_add(1) == 0, "branch executed more than once", branch);
                for (size_t child = 0; child < children; ++child)
                {
                    const size_t index = branch * children + child;
                    TaskScheduler->AddTask(branchTask, [&, index]
                    {
                        Require(calls[index].fetch_add(1) == 0, "leaf executed more than once", index);
                        outputs[index] = index * 31 + 7;
                    });
                }
            });
        }
    });
    TaskScheduler->Wait(root);
    Require(rootCalls.load() == 1, "root was not executed");
    for (size_t i = 0; i < branches; ++i)
        Require(branchCalls[i].load() == 1, "parent completed without executing its branch", i);
    for (size_t i = 0; i < outputs.size(); ++i)
    {
        Require(calls[i].load() == 1, "parent completed without executing its leaf", i);
        Require(outputs[i] == i * 31 + 7, "wait did not publish a descendant's output", i);
    }
}

void PauseResume()
{
    SchedulerSession scheduler(true);
    for (size_t iteration = 0; iteration < 32; ++iteration)
    {
        Signal completed;
        size_t output = 0;
        TaskScheduler->Pause(true);
        auto task = TaskScheduler->AddTask([&]
        {
            output = iteration + 1;
            completed.Set();
        });
        TaskScheduler->Pause(false);
        if (TaskScheduler->GetWorkersCount() > 1)
            Require(completed.Wait(), "resume did not wake workers for pending work", iteration);
        TaskScheduler->Wait(task);
        Require(output == iteration + 1, "pause/resume lost pending work", iteration);
    }
}

void AsyncCallableLifetime()
{
    SchedulerSession scheduler(false);
    std::array<int, 32> outputs{};
    auto task = xr_parallel_for_each(outputs, false, [value = std::make_shared<int>(73)](int& output)
    {
        output = *value;
    });
    TaskScheduler->Wait(task);
    for (int output : outputs)
        Require(output == 73, "asynchronous range lost its temporary callable");

    auto parent = TaskScheduler->AddTask([&outputs](Task& parentTask)
    {
        xr_parallel_for_each(parentTask, outputs, false, [value = std::make_shared<int>(91)](int& output)
        {
            output = *value;
        });
    });
    TaskScheduler->Wait(parent);
    for (int output : outputs)
        Require(output == 91, "parented asynchronous range lost its temporary callable");
}

void ShutdownNestedHelping()
{
    SchedulerSession scheduler(false);
    int output = 0;
    auto parent = TaskScheduler->AddTask([&output](Task& parentTask)
    {
        auto child = TaskScheduler->AddTask(parentTask, [&output] { output = 97; });
        TaskScheduler->Wait(child);
    });
    TaskScheduler->Shutdown();
    Require(parent.IsFinished() && output == 97, "shutdown did not finish nested work before owner teardown");
}

void ShutdownDrainAndRestart()
{
    for (size_t cycle = 0; cycle < 4; ++cycle)
    {
        std::array<size_t, 8193> outputs{};
        std::vector<TaskHandle> handles;
        handles.reserve(outputs.size());
        {
            SchedulerSession scheduler(cycle % 2 != 0);
            TaskScheduler->Pause(true);
            for (size_t i = 0; i < outputs.size(); ++i)
            {
                handles.emplace_back(TaskScheduler->AddTask([&outputs, i]
                {
                    outputs[i] = i + 5;
                    return i + 13;
                }));
            }
        }
        for (size_t i = 0; i < outputs.size(); ++i)
        {
            Require(outputs[i] == i + 5, "shutdown failed to drain pending work", i);
            Require(handles[i].IsFinished(), "drained handle lost completion after scheduler destruction", i);
            Require(handles[i].GetData<size_t>() && *handles[i].GetData<size_t>() == i + 13,
                "drained result did not survive scheduler destruction", i);
        }
    }
}

void Run(const char* name, void (*test)())
{
    currentScenario.store(name);
    Deadline deadline;
    test();
    std::printf("PASS [%s]\n", name);
    std::fflush(stdout);
}
}

int main()
{
    Memory._initialize();
    _initialize_cpu_thread();
    Run("single-worker nested helping and visibility", NestedHelping);
    Run("reset releases ownership without joining or cancelling", ResetDoesNotJoinOrCancel);
    Run("retained results and copied/moved ownership", RetainedResultsAndOwnership);
    Run("concurrent parent/child fanout", ConcurrentFanout);
    Run("worker pause/resume", PauseResume);
    Run("asynchronous range owns its temporary callable", AsyncCallableLifetime);
    Run("shutdown finishes nested work before owner teardown", ShutdownNestedHelping);
    Run("shutdown drains and restart preserves retained completion", ShutdownDrainAndRestart);
    Memory._destroy();
    std::puts("All scheduler regressions passed.");
    return EXIT_SUCCESS;
}
