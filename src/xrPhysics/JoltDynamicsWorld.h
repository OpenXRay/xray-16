#pragma once

#include <memory>
#include <span>

class CPHIsland;

// Transitional dynamics backend: gameplay retains its body/joint handles and
// contact callbacks while one Jolt PhysicsSystem advances all active islands.
// All legacy state is copied at the step boundary, including joint feedback.
class JoltDynamicsWorld final
{
public:
    explicit JoltDynamicsWorld(unsigned workerThreads = 0);
    ~JoltDynamicsWorld();
    JoltDynamicsWorld(const JoltDynamicsWorld&) = delete;
    JoltDynamicsWorld& operator=(const JoltDynamicsWorld&) = delete;

    void Step(std::span<CPHIsland* const> islands, float step, unsigned iterations);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
