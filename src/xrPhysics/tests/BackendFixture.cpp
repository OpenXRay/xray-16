#include "StdAfx.h"
#include "BackendFixture.h"
#include "PHIsland.h"
#ifdef XRAY_USE_JOLT_PHYSICS
#include "JoltDynamicsWorld.h"
#endif
#include <cmath>
#include <cstdio>
#include <limits>
#include <span>
#include <stdexcept>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void Near(float actual, float expected, float tolerance, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance)
    {
        std::fprintf(stderr, "%s: actual=%g expected=%g tolerance=%g\n", message, actual, expected, tolerance);
        throw std::runtime_error(message);
    }
}

struct Island
{
    CPHIsland island;
    Island() { island.Init(); }
    ~Island()
    {
        while (island.firstjoint)
        {
            auto* joint = island.firstjoint;
            island.RemoveJoint(joint);
            dJointDestroy(joint);
        }
        while (island.firstbody)
        {
            auto* body = island.firstbody;
            island.RemoveBody(body);
            dBodyDestroy(body);
        }
    }
    dBodyID Body(float mass = 2.f)
    {
        auto* body = dBodyCreate(nullptr);
        dMass properties;
        dMassSetBoxTotal(&properties, mass, 1, 2, 3);
        dBodySetMass(body, &properties);
        island.AddBody(body);
        return body;
    }
    dJointID Joint(dJointID joint, dBodyID first, dBodyID second = nullptr)
    {
        island.AddJoint(joint);
        dJointAttach(joint, first, second);
        return joint;
    }
};

class Solver
{
#ifdef XRAY_USE_JOLT_PHYSICS
    JoltDynamicsWorld world;
#endif
public:
    explicit Solver(unsigned workers = 0)
#ifdef XRAY_USE_JOLT_PHYSICS
        : world(workers)
#endif
    {}
    void Step(std::span<CPHIsland* const> islands, float dt)
    {
#ifdef XRAY_USE_JOLT_PHYSICS
        world.Step(islands, dt, 40);
#else
        for (auto* island : islands)
            if (island->IsActive())
                dWorldQuickStep(island->DWorld(), dt);
#endif
    }
    void Step(Island& island, float dt = .025f)
    {
        CPHIsland* islands[] = { &island.island };
        Step(islands, dt);
    }
};

void FreeMotion(unsigned workers)
{
    Island first, second;
    auto* gravity = first.Body();
    auto* forced = second.Body();
    dBodySetGravityMode(forced, 0);
    dBodySetPosition(gravity, 0, 10, 0);
    dBodyAddForce(forced, 8, 0, 0);
    dBodyAddTorque(forced, 0, 0, 4);
    CPHIsland* islands[] = { &first.island, &second.island };
    Solver solver(workers);
    solver.Step(islands, .025f);
    Near(dBodyGetLinearVel(gravity)[1], -.24525f, 1.e-5f, "Gravity velocity");
    Near(dBodyGetPosition(gravity)[1], 10.f - .24525f * .025f, 1.e-5f, "Gravity position");
    Near(dBodyGetLinearVel(forced)[0], .1f, 1.e-5f, "Force/mass conversion");
    Near(dBodyGetLinearVel(forced)[1], 0, 1.e-6f, "Gravity-disabled body");
    Near(dBodyGetAngularVel(forced)[2], .12f, 1.e-5f, "Anisotropic inertia");
    Near(dBodyGetForce(forced)[0], 0, 0, "Force accumulator cleared");
    Near(dBodyGetTorque(forced)[2], 0, 0, "Torque accumulator cleared");
    solver.Step(islands, .0125f);
    Near(dBodyGetLinearVel(gravity)[1], -.367875f, 1.e-5f, "Variable timestep");
    Near(dBodyGetLinearVel(forced)[0], .1f, 1.e-5f, "Force applied only once");
    forced->flags |= dxBodyNoUpdatePos;
    const float position = dBodyGetPosition(forced)[0];
    solver.Step(second);
    Near(dBodyGetPosition(forced)[0], position, 0, "No-update-position flag");
    // Retiring the other island must remove its Jolt body without stepping it.
    Near(dBodyGetLinearVel(gravity)[1], -.367875f, 1.e-5f, "Isolated island does not advance another");
    solver.Step(std::span<CPHIsland* const>{}, .025f);
}

void Contacts(unsigned workers, float friction)
{
    Island island;
    auto* body = island.Body();
    dBodySetPosition(body, 0, .5f, 0);
    dBodySetLinearVel(body, 1, 0, 0);
    auto* rotationLock = island.Joint(dJointCreateAMotor(nullptr, nullptr), body);
    dJointSetAMotorMode(rotationLock, dAMotorUser);
    dJointSetAMotorNumAxes(rotationLock, 3);
    for (int axis = 0; axis < 3; ++axis)
    {
        dJointSetAMotorAxis(rotationLock, axis, 0, axis == 0, axis == 1, axis == 2);
        dJointSetAMotorParam(rotationLock, dParamFMax + axis * dParamGroup, dInfinity);
    }
    Solver solver(workers);
    for (int i = 0; i < 160; ++i)
    {
        dContact contact{};
        contact.geom.normal[1] = 1;
        contact.geom.pos[0] = dBodyGetPosition(body)[0];
        contact.geom.pos[1] = 0;
        contact.geom.pos[2] = dBodyGetPosition(body)[2];
        contact.geom.depth = std::max(0.f, .5f - dBodyGetPosition(body)[1]);
        contact.surface.mode = dContactApprox1 | dContactSoftERP | dContactSoftCFM;
        contact.surface.mu = friction;
        contact.surface.soft_erp = .2f;
        contact.surface.soft_cfm = 1.e-5f;
        auto* joint = island.Joint(dJointCreateContact(nullptr, nullptr, &contact), body);
        dJointFeedback feedback{};
        dJointSetFeedback(joint, &feedback);
        solver.Step(island);
        if (i > 80)
            Near(feedback.f1[1], 19.62f, .04f, "Resting contact feedback is a force");
        Near(dBodyGetAngularVel(body)[2], 0, .001f, "Character rotation lock");
        Require(std::isfinite(dBodyGetLinearVel(body)[0]), "Infinite friction generated invalid velocity");
        island.island.RemoveJoint(joint);
        dJointDestroy(joint);
    }
    Near(dBodyGetPosition(body)[1], .5f, .005f, "Resting contact height");
    Near(dBodyGetLinearVel(body)[0], 0, .02f, "Contact friction");
}

void Bounce()
{
    Island island;
    auto* body = island.Body();
    dBodySetGravityMode(body, 0);
    dBodySetLinearVel(body, 0, -2, 0);
    dContact contact{};
    contact.geom.normal[1] = 1;
    contact.surface.mode = dContactBounce | dContactSoftCFM;
    contact.surface.bounce = .5f;
    contact.surface.bounce_vel = .1f;
    island.Joint(dJointCreateContact(nullptr, nullptr, &contact), body);
    Solver solver;
    solver.Step(island);
    Near(dBodyGetLinearVel(body)[1], 1, .001f, "Contact restitution");
}

void FixedPair(unsigned workers)
{
    Island island;
    auto* first = island.Body();
    auto* second = island.Body();
    dBodySetPosition(second, 1, 0, 0);
    auto* joint = island.Joint(dJointCreateFixed(nullptr, nullptr), first, second);
    dJointSetFixed(joint);
    dJointFeedback feedback{};
    dJointSetFeedback(joint, &feedback);
    Solver solver(workers);
    for (int i = 0; i < 80; ++i)
    {
        dBodyAddForce(first, 4, 0, 0);
        solver.Step(island);
        Near(dBodyGetPosition(second)[0] - dBodyGetPosition(first)[0], 1, .005f, "Fixed pair separation");
        Near(feedback.f1[0], -2, .005f, "Joint feedback magnitude");
        Near(feedback.f1[0] + feedback.f2[0], 0, .001f, "Joint feedback action/reaction");
    }
}

void MotorAndLimit()
{
    Island island;
    auto* body = island.Body();
    dBodySetGravityMode(body, 0);
    auto* slider = island.Joint(dJointCreateSlider(nullptr, nullptr), body);
    dJointSetSliderAxis(slider, 1, 0, 0);
    dJointSetSliderParam(slider, dParamVel, 1);
    dJointSetSliderParam(slider, dParamFMax, 100);
    dJointSetSliderParam(slider, dParamLoStop, 0);
    dJointSetSliderParam(slider, dParamHiStop, .5f);
    Solver solver;
    for (int i = 0; i < 120; ++i)
        solver.Step(island);
    Near(dJointGetSliderPosition(slider), .5f, .005f, "Slider motor upper stop");
    dJointSetSliderParam(slider, dParamVel, -1);
    for (int i = 0; i < 120; ++i)
        solver.Step(island);
    Near(dJointGetSliderPosition(slider), 0, .005f, "Slider motor lower stop");
}

void MergedIslandsAndLifecycle()
{
    Island first, second;
    auto* a = first.Body();
    auto* b = second.Body();
    dBodySetPosition(b, 1, 0, 0);
    first.island.Merge(&second.island);
    Solver solver;
    CPHIsland* islands[] = { &first.island, &second.island };
    solver.Step(islands, .025f);
    Near(dBodyGetLinearVel(a)[1], -.24525f, 1.e-5f, "Merged island first body advances once");
    Near(dBodyGetLinearVel(b)[1], -.24525f, 1.e-5f, "Merged island second body advances once");
    first.island.Unmerge();
    second.island.Unmerge();
    first.island.RemoveBody(a);
    dBodyDestroy(a);
    solver.Step(first);
    a = first.Body(4);
    dBodySetGravityMode(a, 0);
    dBodySetPosition(a, 5, 6, 7);
    dBodyAddForce(a, 16, 0, 0);
    solver.Step(first);
    Near(dBodyGetLinearVel(a)[0], .1f, 1.e-5f, "Recreated body uses new mass/state");
    dBodySetPosition(a, 9, 6, 7);
    dBodySetLinearVel(a, 0, 0, 0);
    solver.Step(first);
    Near(dBodyGetPosition(a)[0], 9, 1.e-5f, "External teleport is synchronized");
}

void HingeMotorAndLimits()
{
    Island island;
    auto* body = island.Body();
    dBodySetPosition(body, 2, 3, 4);
    dBodySetGravityMode(body, 0);
    auto* hinge = island.Joint(dJointCreateHinge(nullptr, nullptr), body);
    dJointSetHingeAnchor(hinge, 2, 3, 4);
    dJointSetHingeAxis(hinge, 0, 1, 0);
    dJointSetHingeParam(hinge, dParamLoStop, -.3f);
    dJointSetHingeParam(hinge, dParamHiStop, .3f);
    dJointSetHingeParam(hinge, dParamVel, 1);
    dJointSetHingeParam(hinge, dParamFMax, 30);
    Solver solver;
    for (int i = 0; i < 120; ++i)
    {
        dBodyAddForce(body, 10, 0, 0);
        solver.Step(island);
    }
    Near(dJointGetHingeAngle(hinge), .3f, .005f, "Hinge motor upper stop");
    Near(dBodyGetPosition(body)[0], 2, .001f, "Hinge world anchor");
    dJointSetHingeParam(hinge, dParamVel, -1);
    for (int i = 0; i < 120; ++i)
        solver.Step(island);
    Near(dJointGetHingeAngle(hinge), -.3f, .005f, "Hinge motor lower stop");
}
}

void RunPhysicsBackendTests()
{
    dWorldSetGravity(nullptr, 0, -9.81f, 0);
    dWorldSetCFM(nullptr, 1.e-5f);
    dWorldSetERP(nullptr, .2f);
    dWorldSetQuickStepNumIterations(nullptr, 40);
    for (unsigned workers : {0u, 2u})
    {
        std::fprintf(stderr, "Free motion (%u workers)\n", workers);
        FreeMotion(workers);
        std::fprintf(stderr, "Contact friction (%u workers)\n", workers);
        Contacts(workers, .8f);
        std::fprintf(stderr, "Infinite contact friction (%u workers)\n", workers);
        Contacts(workers, std::numeric_limits<float>::infinity());
        std::fprintf(stderr, "Fixed pair (%u workers)\n", workers);
        FixedPair(workers);
    }
    std::fprintf(stderr, "Restitution\n");
    Bounce();
    std::fprintf(stderr, "Motor and stops\n");
    MotorAndLimit();
    MergedIslandsAndLifecycle();
    HingeMotorAndLimits();
}
