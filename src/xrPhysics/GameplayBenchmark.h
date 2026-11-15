#pragma once

#include <chrono>
#include <string>
#include <vector>

class CObjectSpace;
class CPhysicsShell;

// Developer-only capture. Enable XRAY_GAMEPLAY_BENCHMARK at build time and
// start a phase explicitly from Lua. No per-sample logging or disk writes.
class GameplayBenchmark final
{
public:
    using Clock = std::chrono::steady_clock;
    struct StepSample
    {
        u64 step = 0;
        float dt = 0;
        double total = 0, collision = 0, solver = 0;
        unsigned islands = 0, bodies = 0, joints = 0, contacts = 0;
    };
    struct FrameSample { u32 frame; double milliseconds; };

    ~GameplayBenchmark();
    bool Active() const { return !phase.empty(); }
    void Begin(pcstr name);
    void End();
    void Record(const StepSample& sample) { steps.push_back(sample); }
    void Frame();
    void ForceShell(CPhysicsShell* shell);
    void MaintainWorkload(float step);
    static double Milliseconds(Clock::duration duration);
    static void Queries(CObjectSpace& space, const Fvector& origin);

private:
    std::string phase;
    std::vector<StepSample> steps;
    std::vector<FrameSample> frames;
    std::vector<CPhysicsShell*> shells;
    float simulatedTime = 0;
    Clock::time_point previousFrame{};
};
