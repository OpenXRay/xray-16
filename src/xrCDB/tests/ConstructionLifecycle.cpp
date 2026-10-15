#include "Instrumentation.h"
#include "TestSupport.h"

#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>

class BuildGate
{
    std::mutex mutex;
    std::condition_variable condition;
    bool entered = false, released = false;
    CDB::TestEvent phase;
    static BuildGate* active;

    static void Observe(CDB::TestEvent event, const CDB::MODEL*)
    {
        if (event != active->phase)
            return;
        std::unique_lock<std::mutex> lock(active->mutex);
        active->entered = true;
        active->condition.notify_all();
        active->condition.wait(lock,
            []
            {
                return active->released;
            });
    }

public:
    explicit BuildGate(CDB::TestEvent phase) : phase(phase)
    {
        active = this;
        CDB::SetTestObserver(Observe);
    }

    ~BuildGate()
    {
        Release();
        CDB::SetTestObserver(nullptr);
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock(mutex);
        Require(condition.wait_for(lock, std::chrono::seconds(5),
                    [&]
                    {
                        return entered;
                    }),
            "Build gate not reached");
    }

    void Release()
    {
        std::lock_guard<std::mutex> lock(mutex);
        released = true;
        condition.notify_all();
    }
};

BuildGate* BuildGate::active = nullptr;

void CheckConstructionLifecycle()
{
    if (!strstr(Core.Params, "-mt_cdb"))
        return;
    auto mesh = Fixture();
    for (const bool serialize : { false, true })
    {
        BuildGate gate(CDB::TestEvent::BuildStarted);
        CDB::MODEL model;
        mesh.Build(model);
        gate.Wait();
        std::promise<void> started;
        const auto path = TestPath("cdb-building.cache");
        auto operation = std::async(std::launch::async,
            [&]
            {
                started.set_value();
                if (serialize)
                    Require(model.serialize(path.c_str()), "Concurrent serialization failed");
                else
                {
                    CDB::COLLIDER collider;
                    collider.ray_query(0, &model, Vector(.25f, .25f, -1), Vector(0, 0, 1), 10);
                    Require(Ids(collider, mesh) == std::set<int>{ 0, 1, 3, 4 }, "Query observed unfinished model");
                }
            });
        started.get_future().wait();
        const bool blocked = operation.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout;
        gate.Release();
        operation.get();
        Require(blocked, "Query or serializer failed to wait for construction");
        if (serialize)
        {
            CDB::MODEL cached;
            Require(cached.deserialize(path.c_str()), "Concurrent serialization wrote incomplete cache");
            auto stored = path;
#ifdef XRAY_USE_JOLT_CDB
            stored += ".jolt";
#endif
            FS.file_delete(stored.c_str());
        }
    }
    for (const auto phase : { CDB::TestEvent::BuildStarted, CDB::TestEvent::BuildReady })
    {
        BuildGate gate(phase);
        auto model = std::make_unique<CDB::MODEL>();
        mesh.Build(*model);
        gate.Wait();
        std::promise<void> started;
        auto destruction = std::async(std::launch::async,
            [model = std::move(model), &started]() mutable
            {
                started.set_value();
                model.reset();
            });
        started.get_future().wait();
        const bool blocked = destruction.wait_for(std::chrono::milliseconds(50)) == std::future_status::timeout;
        gate.Release();
        destruction.get();
        Require(blocked, "Destruction returned while builder was active");
    }
    CDB::MODEL model;
    mesh.Build(model);
    std::vector<std::future<void>> readers;
    for (u32 i = 0; i < 4; i++)
        readers.push_back(std::async(std::launch::async,
            [&]
            {
                for (u32 j = 0; j < 32; j++)
                {
                    CDB::COLLIDER collider;
                    collider.ray_query(0, &model, Vector(.25f, .25f, -1), Vector(0, 0, 1), 10);
                    Require(Ids(collider, mesh) == std::set<int>{ 0, 1, 3, 4 }, "Concurrent readers disagree");
                }
            }));
    for (auto& reader : readers)
        reader.get();
}
