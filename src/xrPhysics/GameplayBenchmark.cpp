#include "StdAfx.h"
#include "GameplayBenchmark.h"
#include "PHWorld.h"
#include "PhysicsShell.h"
#include "xrEngine/device.h"
#include "xrEngine/defines.h"
#include "xrCDB/xr_area.h"
#include <array>
#include <random>

namespace
{
#ifdef XRAY_USE_JOLT_PHYSICS
constexpr pcstr dynamics = "Jolt";
#else
constexpr pcstr dynamics = "ODE";
#endif
#ifdef XRAY_BENCHMARK_JOLT_CDB
constexpr pcstr collision = "Jolt";
#else
constexpr pcstr collision = "OPCODE";
#endif
}

double GameplayBenchmark::Milliseconds(Clock::duration duration)
{
    return std::chrono::duration<double, std::milli>(duration).count();
}

GameplayBenchmark::~GameplayBenchmark() { End(); }

void GameplayBenchmark::Begin(pcstr name)
{
    R_ASSERT2(!Active(), "Finish the current benchmark phase first");
    R_ASSERT2(name && *name && xr_strlen(name) < 64 &&
        std::strspn(name, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == xr_strlen(name),
        "Invalid benchmark phase name");
    steps.clear();
    frames.clear();
    steps.reserve(30000);
    frames.reserve(60000);
    previousFrame = {};
    shells.clear();
    simulatedTime = 0;
    phase = name;
    Msg("GAMEPLAY_BENCHMARK BEGIN phase=%s collision=%s dynamics=%s", name, collision, dynamics);
}

void GameplayBenchmark::Frame()
{
    if (!Active())
        return;
    const auto now = Clock::now();
    if (previousFrame != Clock::time_point{})
        frames.push_back({ Device.dwFrame, Milliseconds(now - previousFrame) });
    previousFrame = now;
}

void GameplayBenchmark::End()
{
    if (!Active())
        return;
    const auto name = "benchmark-" + phase + ".csv";
    auto* output = FS.w_open("$app_data_root$", name.c_str());
    R_ASSERT2(output, "Could not write gameplay benchmark");
    output->w_string("kind,phase,collision_backend,dynamics_backend,index,dt_ms,total_ms,collision_ms,solver_ms,islands,bodies,joints,contacts");
    for (const auto& sample : steps)
        output->w_printf("step,%s,%s,%s,%llu,%.6f,%.6f,%.6f,%.6f,%u,%u,%u,%u\n",
            phase.c_str(), collision, dynamics, static_cast<unsigned long long>(sample.step),
            sample.dt * 1000., sample.total, sample.collision, sample.solver,
            sample.islands, sample.bodies, sample.joints, sample.contacts);
    for (const auto& sample : frames)
        output->w_printf("frame,%s,%s,%s,%u,0,%.6f,0,0,0,0,0,0\n",
            phase.c_str(), collision, dynamics, sample.frame, sample.milliseconds);
    FS.w_close(output);
    Msg("GAMEPLAY_BENCHMARK END phase=%s steps=%zu frames=%zu", phase.c_str(), steps.size(), frames.size());
    phase.clear();
    shells.clear();
}

void GameplayBenchmark::ForceShell(CPhysicsShell* shell)
{
    R_ASSERT2(Active() && shell, "Register a valid shell during an active benchmark phase");
    shells.push_back(shell);
}

void GameplayBenchmark::MaintainWorkload(float step)
{
    // Exactly one force application per simulation step, independent of FPS.
    // The isolated script keeps these shells alive until End clears the list.
    for (size_t i = 0; i < shells.size(); ++i)
    {
        const float phase = simulatedTime * 6 + float(i);
        shells[i]->applyForce(250 * std::sin(phase), 700 * std::cos(phase), 250 * std::cos(phase));
    }
    simulatedTime += step;
}

void GameplayBenchmark::Queries(CObjectSpace& space, const Fvector& origin)
{
    constexpr unsigned count = 4096;
    struct Query { Fvector origin, direction, extents; float range; };
    std::array<Query, count> queries;
    // Identical seed and actor location produce identical input on every
    // backend. These are probes of the real loaded level, not a synthetic mesh.
    std::mt19937 random(2139);
    auto unit = [&] { return float(random() >> 8) / float(1u << 24); };
    for (auto& query : queries)
    {
        query.origin.set(origin.x + (unit() * 2 - 1) * 20,
            origin.y + 1 + unit() * 8, origin.z + (unit() * 2 - 1) * 20);
        query.direction.set(unit() * 2 - 1, unit() * 2 - 1, unit() * 2 - 1);
        query.direction.normalize_safe();
        query.range = 2 + unit() * 80;
        const float radius = .25f + unit() * 3;
        query.extents.set(radius, radius, radius);
    }
    auto* model = space.GetStaticModel();
    model->syncronize();
    CDB::COLLIDER collider;
    auto* output = FS.w_open("$app_data_root$", "benchmark-queries.csv");
    R_ASSERT2(output, "Could not write level-query benchmark");
    output->w_string("workload,collision_backend,dynamics_backend,iteration,queries,total_ms,hits,range_sum,model_bytes,origin_x,origin_y,origin_z");
    for (int workload = 0; workload < 3; ++workload)
    {
        pcstr name = workload == 0 ? "nearest_ray" : workload == 1 ? "any_ray" : "full_box";
        for (int iteration = -1; iteration < 7; ++iteration) // One unmeasured warm-up.
        {
            u64 hits = 0;
            double ranges = 0;
            const auto begin = Clock::now();
            for (const auto& query : queries)
            {
                if (workload == 2)
                    collider.box_query(CDB::OPT_FULL_TEST, model, query.origin, query.extents);
                else
                    collider.ray_query(workload == 0 ? CDB::OPT_ONLYNEAREST : CDB::OPT_ONLYFIRST,
                        model, query.origin, query.direction, query.range);
                hits += collider.r_count();
                if (workload == 0 && collider.r_count())
                    ranges += collider.r_begin()->range;
            }
            const auto milliseconds = Milliseconds(Clock::now() - begin);
            if (iteration >= 0)
                output->w_printf("%s,%s,%s,%d,%u,%.6f,%llu,%.9f,%zu,%.9f,%.9f,%.9f\n",
                    name, collision, dynamics, iteration, count, milliseconds,
                    static_cast<unsigned long long>(hits), ranges, model->memory(), origin.x, origin.y, origin.z);
        }
    }
    FS.w_close(output);
    Msg("GAMEPLAY_BENCHMARK QUERIES collision=%s dynamics=%s origin=%.9f,%.9f,%.9f",
        collision, dynamics, origin.x, origin.y, origin.z);
}

void CPHWorld::BenchmarkBegin(pcstr phase)
{
    R_ASSERT2(!psDeviceFlags.test(mtPhysics), "Benchmark capture requires mt_physics off to serialize Lua control and physics");
    m_benchmark->Begin(phase);
}
void CPHWorld::BenchmarkEnd() { m_benchmark->End(); }
void CPHWorld::BenchmarkForceShell(CPhysicsShell* shell) { m_benchmark->ForceShell(shell); }
void CPHWorld::BenchmarkQueries(const Fvector& origin)
{
    R_ASSERT2(!m_benchmark->Active(), "Run query batches outside frame/physics capture");
    GameplayBenchmark::Queries(ObjectSpace(), origin);
}
