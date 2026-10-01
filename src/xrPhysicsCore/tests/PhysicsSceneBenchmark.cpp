// Same scene generator for the engine's ODE DLL and native Jolt core.
#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#ifdef XRAY_BENCHMARK_ODE
#include <ode/ode.h>
#else
#include "xrPhysicsCore/IPhysicsCore.h"
#endif
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr float dt = .01f;
struct Position { float x, y, z; };
double ProcessCPUTimeMilliseconds() {
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
        throw std::runtime_error("Cannot read process CPU time");
    ULARGE_INTEGER kernelTicks{}, userTicks{};
    kernelTicks.LowPart = kernel.dwLowDateTime; kernelTicks.HighPart = kernel.dwHighDateTime;
    userTicks.LowPart = user.dwLowDateTime; userTicks.HighPart = user.dwHighDateTime;
    return double(kernelTicks.QuadPart + userTicks.QuadPart) / 10000.0;
}
#ifdef XRAY_BENCHMARK_ODE
using Body = dBodyID;
class Scene {
    dSpaceID space = dHashSpaceCreate(nullptr);
    dJointGroupID contacts = dJointGroupCreate(0);
    std::vector<dJointID> contactJoints;
    struct JointRecord { dJointID joint; Body first, second; };
    std::vector<JointRecord> permanentJoints;
    std::vector<dWorldID> worlds;
    std::vector<unsigned> parents;
    unsigned Root(unsigned index) {
        while (index != parents[index]) { parents[index] = parents[parents[index]]; index = parents[index]; }
        return index;
    }
    unsigned Index(Body body) const { return static_cast<unsigned>(reinterpret_cast<uintptr_t>(dBodyGetData(body)) - 1); }
    void Connect(Body first, Body second) { if (first && second) parents[Root(Index(first))] = Root(Index(second)); }
    dWorldID World(Body first, Body second) { return worlds[Root(Index(first ? first : second))]; }
    static void Collide(void* data, dGeomID first, dGeomID second) {
        auto& scene = *static_cast<Scene*>(data);
        const auto body1 = dGeomGetBody(first), body2 = dGeomGetBody(second);
        if (body1 && body2 && dAreConnectedExcluding(body1, body2, dJointTypeContact)) return;
        dContact points[8]{};
        const int count = dCollide(first, second, 8, &points[0].geom, sizeof(dContact));
        scene.contactCount += count;
        if (count) scene.Connect(body1, body2);
        for (int i = 0; i < count; ++i) {
            points[i].surface.mode = dContactApprox1 | dContactBounce | dContactSoftERP | dContactSoftCFM;
            points[i].surface.mu = .7f;
            points[i].surface.bounce = .1f;
            points[i].surface.bounce_vel = 1;
            points[i].surface.soft_erp = .2f;
            points[i].surface.soft_cfm = 1e-5f;
            auto joint = dJointCreateContact(nullptr, scene.contacts, &points[i]);
            scene.contactJoints.push_back(joint);
            dJointAttach(joint, body1, body2);
        }
    }
public:
    unsigned contactCount = 0;
    Scene() { dCreatePlane(space, 0, 1, 0, 0); }
    ~Scene() {
        dJointGroupDestroy(contacts);
        dSpaceDestroy(space);
        for (const auto& record : permanentJoints) dJointDestroy(record.joint);
        for (auto body : ownedBodies) dBodyDestroy(body);
        for (auto world : worlds) dWorldDestroy(world);
    }
    std::vector<Body> ownedBodies;
    Body Box(Position position) {
        auto body = dBodyCreate(nullptr);
        dBodySetData(body, reinterpret_cast<void*>(uintptr_t(ownedBodies.size() + 1)));
        ownedBodies.push_back(body);
        worlds.push_back(dWorldCreate());
        parents.push_back(0);
        // The patched ODE world settings are static, shared by all islands.
        dWorldSetGravity(nullptr, 0, -9.81f, 0);
        dWorldSetQuickStepNumIterations(nullptr, 18);
        dWorldSetAutoDisableFlag(nullptr, 0);
        dMass mass;
        dMassSetBoxTotal(&mass, 2, 1, 1, 1);
        dBodySetMass(body, &mass);
        dBodySetPosition(body, position.x, position.y, position.z);
        dGeomSetBody(dCreateBox(space, 1, 1, 1), body);
        return body;
    }
    void Joint(Body first, Body second, Position anchor) {
        auto joint = dJointCreateBall(nullptr, nullptr);
        dJointAttach(joint, first, second);
        dJointSetBallAnchor(joint, anchor.x, anchor.y, anchor.z);
        permanentJoints.push_back({joint, first, second});
    }
    void Force(Body body, Position force) { dBodyEnable(body); dBodyAddForce(body, force.x, force.y, force.z); }
    void Step(const std::vector<Body>& bodies) {
        // Match the reference native core's configured linear/angular damping.
        // This loop is included in the ODE timing.
        for (auto body : bodies) {
            const auto* v = dBodyGetLinearVel(body);
            dBodySetLinearVel(body, v[0] * (1-.05f*dt), v[1] * (1-.05f*dt), v[2] * (1-.05f*dt));
            const auto* w = dBodyGetAngularVel(body);
            dBodySetAngularVel(body, w[0] * (1-.05f*dt), w[1] * (1-.05f*dt), w[2] * (1-.05f*dt));
        }
        contactCount = 0;
        for (unsigned i = 0; i < parents.size(); ++i) parents[i] = i;
        for (const auto& record : permanentJoints) Connect(record.first, record.second);
        dSpaceCollide(space, this, Collide);
        std::vector<unsigned> activeWorlds;
        for (unsigned i = 0; i < bodies.size(); ++i) {
            const auto root = Root(i);
            dWorldAddBody(worlds[root], bodies[i]);
            if (root == i) activeWorlds.push_back(root);
        }
        for (const auto& record : permanentJoints) dWorldAddJoint(World(record.first, record.second), record.joint);
        for (auto joint : contactJoints) dWorldAddJoint(World(dJointGetBody(joint, 0), dJointGetBody(joint, 1)), joint);
        for (const auto index : activeWorlds) dWorldQuickStep(worlds[index], dt);
        // X-Ray's patched ODE group reset leaves world lists to the engine.
        for (auto joint : contactJoints) {
            dWorldRemoveJoint(World(dJointGetBody(joint, 0), dJointGetBody(joint, 1)), joint);
            dJointAttach(joint, nullptr, nullptr);
        }
        for (const auto& record : permanentJoints) dWorldRemoveJoint(World(record.first, record.second), record.joint);
        for (unsigned i = 0; i < bodies.size(); ++i) dWorldRemoveBody(worlds[Root(i)], bodies[i]);
        contactJoints.clear();
        dJointGroupEmpty(contacts);
    }
    Position GetPosition(Body body) const { const auto* p = dBodyGetPosition(body); return {p[0], p[1], p[2]}; }
    unsigned Workers() const { return 0; }
};
constexpr Body worldBody = nullptr;
constexpr const char* backend = "ODE";
#else
using Body = BodyHandle;
Fvector V(Position value) { return Fvector().set(value.x, value.y, value.z); }
class Scene {
    IPhysicsCore& core = *GetPhysicsCore();
public:
    unsigned contactCount = 0;
    Scene() {
        core.Clear();
        core.SetSimulationParameters(9.81f, 18);
        // CreateBox supplies friction .7 and restitution .1, matching the
        // dynamic boxes and ODE contact settings. Preserve those properties
        // when making the floor static; CreateStaticBody uses Jolt defaults.
        const auto floor = core.CreateBox(V({1000, .5f, 1000}), V({0, -.5f, 0}), 2);
        if (floor == INVALID_BODY_HANDLE) throw std::runtime_error("Floor creation failed");
        core.SetBodyMotionType(floor, 0);
    }
    ~Scene() { core.Clear(); core.Destroy(); }
    Body Box(Position position) { return core.CreateBox(V({.5f, .5f, .5f}), V(position), 2); }
    void Joint(Body first, Body second, Position anchor) {
        if (core.CreateJoint(0, first, second, V(anchor), V({1,0,0}), V({0,1,0}), V({0,0,1}),
            V({0,0,0}), V({0,0,0})) == INVALID_JOINT_HANDLE) throw std::runtime_error("Joint creation failed");
    }
    void Force(Body body, Position force) { core.ApplyForce(body, V(force)); }
    void Step(const std::vector<Body>&) { core.Step(dt); contactCount = core.GetStatistics().contact_points; }
    Position GetPosition(Body body) const { Fvector p; core.GetBodyPosition(body, p); return {p.x,p.y,p.z}; }
    unsigned Workers() const { return core.GetStatistics().workers; }
};
constexpr Body worldBody = INVALID_BODY_HANDLE;
constexpr const char* backend = "JoltNative";
#endif
}

int main(int argc, char** argv) {
    if (argc != 4) { std::fprintf(stderr, "usage: benchmark stacks|chains body_count output.csv\n"); return 2; }
    Core.Initialize("PhysicsSceneBenchmark", "", false);
    int result = 0;
    try {
        const std::string scenario = argv[1];
        if (scenario != "stacks" && scenario != "chains") throw std::runtime_error("Unknown scenario");
        const unsigned count = static_cast<unsigned>(std::stoul(argv[2]));
        if (!count || count > 8192 || count % 8) throw std::runtime_error("Body count must be a multiple of eight, 8..8192");
        Scene scene;
        std::vector<Body> bodies;
        for (unsigned i = 0; i < count; ++i) {
            const unsigned group = i / 8, height = i % 8;
            const Position position{float(group % 32) * 2, .51f + height * 1.01f, float(group / 32) * 2};
            const auto body = scene.Box(position);
            bodies.push_back(body);
            if (scenario == "chains") {
                if (!height) scene.Joint(worldBody, body, {position.x, position.y - .5f, position.z});
                else scene.Joint(bodies[i-1], body, {position.x, position.y - .505f, position.z});
            }
        }
        FILE* file = nullptr;
        if (fopen_s(&file, argv[3], "w") || !file) throw std::runtime_error("Cannot open output");
        std::fprintf(file, "backend,workers,scenario,bodies,joints,step,dt_ms,total_ms,contacts,min_y,max_y,working_set_bytes,private_bytes\n");
        double timedCPUStart = 0;
        for (unsigned step = 0; step < 600; ++step) {
            if (step == 100) timedCPUStart = ProcessCPUTimeMilliseconds();
            const auto start = std::chrono::steady_clock::now();
            for (unsigned i = 0; i < count; ++i) {
                const float phase = step * dt * 6 + float(i);
                scene.Force(bodies[i], {2 * std::sin(phase), 0, 2 * std::cos(phase)});
            }
            scene.Step(bodies);
            const double time = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            float minY = 1e10f, maxY = -1e10f;
            for (auto body : bodies) {
                const auto position = scene.GetPosition(body);
                if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
                    position.y < -.5f || std::abs(position.x) > 2000 || std::abs(position.z) > 2000 || position.y > 100)
                    throw std::runtime_error("Scene became invalid or penetrated floor");
                minY = std::min(minY, position.y); maxY = std::max(maxY, position.y);
            }
            if (step >= 100) {
                PROCESS_MEMORY_COUNTERS_EX memory{};
                GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory));
                std::fprintf(file, "%s,%u,%s,%u,%u,%u,10,%.9f,%u,%.6f,%.6f,%zu,%zu\n",
                    backend, scene.Workers(), scenario.c_str(), count, scenario == "chains" ? count : 0,
                    step, time, scene.contactCount, minY, maxY, memory.WorkingSetSize, memory.PrivateUsage);
            }
        }
        const double timedCPU = ProcessCPUTimeMilliseconds() - timedCPUStart;
        std::fclose(file);
        // Includes validation, CSV output, and memory sampling alongside all
        // worker CPU time. Per-step total_ms measures physics wall time only.
        std::printf("TIMED_PROCESS_CPU_MS %.6f\n", timedCPU);
        std::printf("PASS %s %s %u bodies (workers=%u)\n", backend, scenario.c_str(), count, scene.Workers());
    } catch (const std::exception& exception) { std::fprintf(stderr, "FAIL: %s\n", exception.what()); result = 1; }
    Core._destroy();
    return result;
}
