#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrPhysicsCore/IPhysicsCore.h"
#include "xrPhysicsCore/BodyState.h"
#include "xrPhysics/NativeContactEffects.h"
#include "xrPhysics/PHJointDestroyInfo.h"
#include "xrCDB/xrCDB.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <thread>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void Near(float value, float expected, float tolerance, const char* message) {
    if (!std::isfinite(value) || std::abs(value - expected) > tolerance) {
        std::fprintf(stderr, "%s: got %g, expected %g (+/- %g)\n", message, value, expected, tolerance);
        throw std::runtime_error(message);
    }
}
Fvector V(float x, float y, float z) { return Fvector().set(x, y, z); }
IPhysicsCore* policyCore = nullptr;
bool rejectContacts = false, stopImpact = false;
bool overrideFriction = false;
bool staticEnvironment = false;
float contactFriction = 0;
u32 contactCalls = 0, geometryMask = 0;
bool ContactPolicy(const NativePhysicsContact& contact) {
    ++contactCalls;
    if (overrideFriction) contact.friction = contactFriction;
    if (staticEnvironment) {
        if (contact.object1 == policyCore) contact.static_body2 = true;
        else contact.static_body1 = true;
    }
    if (contact.object1) geometryMask |= 1u << contact.geometry1;
    if (contact.object2) geometryMask |= 1u << contact.geometry2;
    Fvector velocity;
    // Calling a locking native API here verifies callbacks run outside the
    // native solver locks; missiles and capture callbacks do this in the game.
    if (contact.kind1 != 6 && contact.kind2 != 6)
        policyCore->GetBodyLinearVelocity(contact.object1 ? contact.body1 : contact.body2, velocity);
    if (stopImpact) policyCore->SetBodyLinearVelocity(contact.object1 ? contact.body1 : contact.body2, V(0, 0, 0));
    return !rejectContacts;
}
void Motion(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(9.81f, 18);
    const auto body = core.CreateBox(V(.5f, 1, 1.5f), V(0, 10, 0), 2);
    Near(core.GetBodyMass(body), 2, 1e-5f, "Body mass");
    core.ApplyForce(body, V(3, 0, 0));
    core.SetBodyForce(body, V(8, 0, 0));
    Fvector value;
    core.GetBodyForce(body, value);
    Near(value.x, 8, 1e-6f, "SetForce replaces the accumulator");
    core.SetBodyInWorld(body, false);
    core.Step(.025f);
    core.GetBodyPosition(body, value);
    Near(value.y, 10, 1e-5f, "Prepared body remains outside simulation");
    core.SetBodyInWorld(body, true);
    core.Step(.025f);
    core.GetBodyLinearVelocity(body, value);
    Near(value.x, .1f, .0002f, "Force/mass integration");
    Near(value.y, -.24525f, .0004f, "Gravity integration");
    core.GetBodyForce(body, value);
    Near(value.x, 0, 1e-6f, "Force cleared after integration");
    core.SetBodyMass(body, 4);
    Near(core.GetBodyMass(body), 4, 1e-5f, "Runtime body mass");
    auto properties = core.GetBodyMassProperties(body);
    Near(properties.inertia.i.x, 4 * (4 + 9) / 12.f, 1e-4f, "Anisotropic box inertia");
    properties.inertia.i.x *= 2;
    core.SetBodyMassProperties(body, properties);
    Near(core.GetBodyMassProperties(body).inertia.i.x, properties.inertia.i.x, 1e-4f, "Runtime inertia tensor");
    core.ApplyTorque(body, V(1, 2, 3));
    core.SetBodyTorque(body, V(4, 0, 0));
    core.GetBodyTorque(body, value);
    Near(value.x, 4, 1e-5f, "SetTorque replaces the accumulator");
    Near(value.y, 0, 1e-5f, "SetTorque clears previous axes");
    core.Step(.01f);
    core.GetBodyTorque(body, value);
    Near(value.x, 0, 1e-5f, "Torque cleared after integration");
    core.SetBodyGravityFactor(body, 0);
    core.SetBodyDamping(body, 2.f, 3.f);
    core.SetBodyLinearVelocity(body, V(10, 0, 0));
    core.SetBodyAngularVelocity(body, V(0, 0, 2));
    core.Step(.1f);
    core.GetBodyLinearVelocity(body, value);
    Near(value.x, 8, .001f, "Runtime linear damping");
    core.GetBodyAngularVelocity(body, value);
    Near(value.z, 1.4f, .001f, "Runtime angular damping");
    core.DestroyBody(body);
    Require(core.GetStatistics().bodies == 0, "Body retirement");
}
void CompoundTransforms(IPhysicsCore& core) {
    core.Clear();
    auto child = core.CreateBoxShape(V(.5f, .5f, .5f));
    Fmatrix offset = Fidentity;
    offset.c = V(2, 0, 0);
    auto compound = core.CreateCompoundShape(&child, &offset, 1);
    const auto body = core.CreateBodyFromShape(compound, V(0, 3, 0), 2);
    core.DestroyCDBModel(child);
    core.DestroyCDBModel(compound);
    Fvector center, extents, position;
    core.GetBodyPosition(body, position);
    Near(position.x, 0, .0001f, "Compound preserves prescribed mass center");
    core.GetBodyAABB(body, center, extents);
    Near(center.x, 2, .0001f, "Compound geometry uses element offset");
    Fmatrix transform = Fidentity;
    transform.i.x = transform.k.z = -1.f;
    transform.c = V(4, 3, 0);
    core.SetBodyTransform(body, transform);
    core.GetBodyAABB(body, center, extents);
    Near(center.x, 2, .0001f, "Rotated compound geometry matches bone transform");
    core.GetBodyPosition(body, position);
    Near(position.x, 4, .0001f, "Compound transform round trip");
    core.SetBodyLinearVelocity(body, V(1, 2, 3));
    core.SetBodyAngularVelocity(body, V(4, 5, 6));
    core.SetBodyForce(body, V(7, 8, 9));
    core.SetBodyTorque(body, V(10, 11, 12));
    NativeBodyState state;
    Require(GetPhysicsBodyState(body, state) && state.active, "Coherent state reads an active compound");
    Near(state.transform.c.x, 4, .0001f, "Coherent state uses mass center, not offset geometry");
    Near(state.transform.c.y, 3, .0001f, "Coherent state position");
    Near(state.transform.i.x, -1, .0001f, "Coherent state rotation");
    Near(state.mass, 2, .0001f, "Coherent state mass");
    Near(state.linear_velocity.y, 2, .0001f, "Coherent state linear velocity");
    Near(state.angular_velocity.z, 6, .0001f, "Coherent state angular velocity");
    Near(state.force.z, 9, .0001f, "Coherent state force accumulator");
    Near(state.torque.y, 11, .0001f, "Coherent state torque accumulator");
    Require(GetPhysicsBodyState(body, state, true) && state.active, "Active-only state retains awake data");
    Near(state.force.z, 9, .0001f, "Active-only state retains awake force");
    // The read releases its lock before returning, so mutation is safe.
    core.DeactivateBody(body);
    Require(GetPhysicsBodyState(body, state) && !state.active, "Coherent state reads a sleeping body");
    Require(GetPhysicsBodyState(body, state, true) && !state.active, "Active-only state recognizes sleeping bodies");
    Near(state.mass, 0, 0, "Inactive shortcut clears unused state");
    Near(state.transform.c.x, 0, 0, "Inactive shortcut avoids unused transform");
    core.DestroyBody(body);
    Require(!GetPhysicsBodyState(body, state), "Coherent state rejects retired handles");
    Near(state.mass, 0, 0, "Failed coherent read clears previous data");
    Require(!GetPhysicsBodyState(INVALID_BODY_HANDLE, state), "Coherent state rejects invalid handles");
    child = core.CreateBoxShape(V(.5f, .5f, .5f));
    const auto fixed = core.CreateStaticBody(child, V(1, 2, 3));
    core.DestroyCDBModel(child);
    Require(GetPhysicsBodyState(fixed, state) && !state.active, "Coherent state reads a static body");
    Near(state.transform.c.z, 3, .0001f, "Static coherent state position");
    Near(state.linear_velocity.x, 0, 0, "Static coherent state has zero velocity");
    Near(state.force.x, 0, 0, "Static coherent state has zero force");
}
struct ContactEffectTestParameters {
    static constexpr float vel_cret_wallmark = 10, vel_cret_sound = 5, vel_cret_particles = 2;
};
void ContactEffectEligibility() {
    auto eligible = [](float energy, float distance, bool passable, bool triangle) {
        return NativeContactHasEffects<ContactEffectTestParameters>(energy, distance, passable, triangle, 100, 25);
    };
    Require(!eligible(2, 0, false, true), "Exact dry effect thresholds stay silent");
    Require(eligible(2.01f, 24, false, false), "Nearby particles survive the early effect filter");
    Require(!eligible(2.01f, 25, false, false), "Exact particle distance remains excluded");
    Require(eligible(5.01f, 99, false, false), "Nearby impact sounds survive the early filter");
    Require(!eligible(5.01f, 100, false, false), "Exact sound distance remains excluded");
    Require(eligible(0, 99, true, false), "Continuous passable sounds need no impact threshold");
    Require(!eligible(0, 100, true, false), "Distant passable sounds remain excluded");
    Require(eligible(10.01f, 10000, false, true), "Distant wallmarks remain eligible");
    Require(!eligible(10.01f, 10000, false, false), "Wallmarks require a static triangle");
}
void Contact(IPhysicsCore& core) {
    core.Clear();
    const auto floorShape = core.CreateBoxShape(V(50, .5f, 50));
    core.CreateStaticBody(floorShape, V(0, -.5f, 0));
    core.DestroyCDBModel(floorShape);
    const auto body = core.CreateBox(V(.5f, .5f, .5f), V(0, 2, 0), 2);
    for (int i = 0; i < 400; ++i) core.Step(.01f);
    Fvector position, velocity;
    core.GetBodyPosition(body, position);
    core.GetBodyLinearVelocity(body, velocity);
    Near(position.y, .5f, .03f, "Resting floor contact");
    Near(velocity.y, 0, .03f, "Resting floor velocity");
    core.SetBodyCollideWithStatics(body, false);
    core.ActivateBody(body);
    for (int i = 0; i < 100; ++i) core.Step(.01f);
    core.GetBodyPosition(body, position);
    Require(position.y < -2, "Static collision filter");
}
void ContactFeedback(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(9.81f, 18);
    const auto shape = core.CreateBoxShape(V(50, .5f, 50));
    const auto floor = core.CreateStaticBody(shape, V(0, -.5f, 0));
    core.DestroyCDBModel(shape);
    const auto body = core.CreateBox(V(.5f, .5f, .5f), V(0, .5f, 0), 2);
    core.SetBodyDamping(body, 0, 0);
    core.SetBodyContactFeedback(body, true);
    core.SetBodyContactFeedback(floor, true);
    for (int step = 0; step < 100; ++step) {
        core.ApplyForce(body, V(1, 0, 0));
        core.Step(.01f);
    }
    float x = 0, y = 0, reactionY = 0;
    const auto& forces = core.GetBodyContactForces(body);
    Require(!forces.empty(), "Solved contact feedback is available after step");
    for (const auto& contact : forces) {
        Require(contact.geometry == 0, "Contact feedback identifies geometry");
        x += contact.force.x;
        y += contact.force.y;
    }
    for (const auto& contact : core.GetBodyContactForces(floor)) reactionY += contact.force.y;
    Near(y, 19.62f, .05f, "Solved normal impulses converted to force");
    Near(x, -1, .05f, "Solved friction impulse opposes external force");
    Near(y + reactionY, 0, 1e-5f, "Contact feedback reaction pair");
    policyCore = &core;
    core.SetBodyUserData(body, &core);
    core.SetRigidBodyContactCallback(ContactPolicy);
    overrideFriction = true;
    contactFriction = 0;
    for (int step = 0; step < 100; ++step) {
        core.ApplyForce(body, V(1, 0, 0));
        core.Step(.01f);
    }
    Fvector velocity;
    core.GetBodyLinearVelocity(body, velocity);
    Near(velocity.x, .5f, .03f, "Gameplay friction override permits sliding on cached contacts");
    contactFriction = .7f;
    for (int step = 0; step < 100; ++step) {
        core.ApplyForce(body, V(1, 0, 0));
        core.Step(.01f);
    }
    core.GetBodyLinearVelocity(body, velocity);
    Near(velocity.x, 0, .03f, "Changing gameplay friction stops sliding on cached contacts");
    overrideFriction = false;
    core.SetRigidBodyContactCallback(nullptr);
    core.SetBodyContactFeedback(body, false);
    Require(core.GetBodyContactForces(body).empty(), "Disabling feedback clears old contact forces");
}
void Joints(IPhysicsCore& core) {
    for (const float dt : {.01f, .025f}) {
        core.Clear();
        const auto body = core.CreateBox(V(.5f, .5f, .5f), V(0, 2, 0), 2);
        const auto joint = core.CreateJoint(0, INVALID_BODY_HANDLE, body, V(0, 2, 0),
            V(1, 0, 0), V(0, 1, 0), V(0, 0, 1), V(0, 0, 0), V(0, 0, 0));
        SPhysicsJointFeedback feedback;
        core.SetJointFeedback(joint, &feedback);
        for (int i = 0; i < 100; ++i) core.Step(dt);
        Near(feedback.f2.y, 19.62f, .05f, "Joint impulse converted to force");
        Near(feedback.f1.y + feedback.f2.y, 0, 1e-5f, "Joint reaction pair");
        Fvector position;
        core.GetBodyPosition(body, position);
        Near(position.y, 2, .01f, "Ball anchor stability");
        core.DestroyBody(body);
        Require(core.GetStatistics().constraints == 0, "Body retirement removes constraints");
        core.DestroyJoint(joint);
    }
    core.Clear();
    core.SetSimulationParameters(0, 18);
    const auto body = core.CreateBox(V(.5f, .5f, .5f), V(0, 0, 0), 2);
    const auto hinge = core.CreateJoint(1, INVALID_BODY_HANDLE, body, V(0, 0, 0),
        V(0, 1, 0), V(1, 0, 0), V(0, 0, 1), V(-.25f, 0, 0), V(.25f, 0, 0));
    core.SetJointMotor(hinge, 0, 5, 1);
    core.SetJointSpringDamping(hinge, 0, .2f, .00001f);
    for (int i = 0; i < 200; ++i) core.Step(.01f);
    Near(core.GetJointAxisAngle(hinge, 0), .25f, .06f, "Hinge motor respects limit");
    core.SetJointLimits(hinge, 0, -.1f, .1f);
    for (int i = 0; i < 100; ++i) core.Step(.01f);
    Near(core.GetJointAxisAngle(hinge, 0), .1f, .06f, "Runtime hinge limit");
    core.SetJointMotor(hinge, 0, 5, 0);
    for (int i = 0; i < 100; ++i) core.Step(.01f);
    Near(core.GetJointAxisAngleRate(hinge, 0), 0, .03f, "Powered zero-speed motor brake");
}
void MassCenterAndSuspension(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(0, 18);
    const auto body = core.CreateBox(V(.5f, .5f, .5f), V(0, 2, 0), 2);
    const auto anchor = core.CreateJoint(0, body, INVALID_BODY_HANDLE, V(0, 2, 0),
        V(1, 0, 0), V(0, 1, 0), V(0, 0, 1), V(0, 0, 0), V(0, 0, 0));
    core.RecenterBody(body, V(.5f, 0, 0));
    Fvector position;
    core.GetJointAnchor(anchor, position);
    Require(position.similar(V(0, 2, 0), .00001f), "Mass center shift preserves world joint anchor");
    for (int step = 0; step < 20; ++step) core.Step(.01f);
    core.GetBodyPosition(body, position);
    Near(position.x, .5f, .00001f, "Shifted mass center remains constrained without snapping back");
    for (const float dt : {.01f, .025f}) {
        core.Clear();
        core.SetSimulationParameters(9.81f, 18);
        const auto wheel = core.CreateBox(V(.3f, .3f, .3f), V(0, 2, 0), 2);
        const auto suspension = core.CreateJoint(2, INVALID_BODY_HANDLE, wheel, V(0, 2, 0),
            V(0, 1, 0), V(1, 0, 0), V(0, 0, 1), V(0, 0, 0), V(0, 0, 0));
        const float stiffness = 500, damping = 50;
        const float denominator = dt * stiffness + damping;
        core.SetJointSpringDamping(suspension, -1, dt * stiffness / denominator, 1 / denominator);
        SPhysicsJointFeedback feedback;
        core.SetJointFeedback(suspension, &feedback);
        for (int step = 0; step < 400; ++step) core.Step(dt);
        core.GetBodyPosition(wheel, position);
        Near(position.y, 2 - 19.62f / stiffness, .002f, "Wheel suspension supports weight with spring compression");
        Near(position.x, 0, .0001f, "Wheel suspension locks lateral translation");
        Near(feedback.f2.y, 19.62f, .05f, "Suspension reaction uses world coordinates");
    }
}
void AnchoredDoor(IPhysicsCore& core) {
    for (bool staticRoot : {false, true}) {
        core.Clear();
        core.SetSimulationParameters(19.62f, 18);
        const auto position = V(256, 19, 548);
        auto child = core.CreateBoxShape(V(.4f, 1, .05f));
        Fmatrix offset = Fidentity;
        offset.c = V(.4f, 0, 0);
        auto compound = core.CreateCompoundShape(&child, &offset, 1);
        auto body = core.CreateBodyFromShape(compound, position, 20);
        core.DestroyCDBModel(child);
        core.DestroyCDBModel(compound);
        Fmatrix transform = Fidentity;
        transform.i = V(0, 0, -1);
        transform.k = V(1, 0, 0);
        transform.c = position;
        core.SetBodyTransform(body, transform);
        BodyHandle root = INVALID_BODY_HANDLE;
        if (staticRoot) {
            root = core.CreateBox(V(.01f, .01f, .01f), position, 1);
            core.SetBodyMotionType(root, 0);
            core.SetBodyObjectLayer(root, 4);
        }
        const auto joint = core.CreateJoint(1, root, body, position,
            V(0, 1, 0), V(1, 0, 0), V(0, 0, 1), V(-1, 0, 0), V(1, 0, 0));
        Require(joint != INVALID_JOINT_HANDLE, "Door joint creation");
        core.SetBodyInWorld(body, false);
        if (staticRoot) core.SetBodyInWorld(root, false);
        core.Step(.01f);
        core.SetBodyInWorld(body, true);
        if (staticRoot) core.SetBodyInWorld(root, true);
        for (int step = 0; step < 1000; ++step) core.Step(.01f);
        Fvector result, velocity;
        core.GetBodyPosition(body, result);
        core.GetBodyLinearVelocity(body, velocity);
        Near(result.y, position.y, .005f, "Translated door remains anchored");
        Near(velocity.y, 0, .005f, "Door has no falling velocity");
    }
}
void AnimatedTarget(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(19.62f, 18);
    const auto position = V(256, 19, 548);
    const auto target = core.CreateBox(V(.01f, .01f, .01f), position, 1);
    core.SetBodyMotionType(target, 1);
    core.SetBodyObjectLayer(target, 4);
    const auto body = core.CreateBox(V(.4f, 1, .05f), position, 20);
    const auto joint = core.CreateJoint(3, target, body, position,
        V(1, 0, 0), V(0, 1, 0), V(0, 0, 1), V(0, 0, 0), V(0, 0, 0));
    Require(joint != INVALID_JOINT_HANDLE, "Animation holding joint creation");
    for (int step = 0; step < 1000; ++step) core.Step(.01f);
    Fvector result;
    core.GetBodyPosition(body, result);
    Near(result.y, 19, .005f, "Animated prop held while animation is idle");
    Fmatrix transform = Fidentity;
    transform.c = V(256, 20, 548);
    transform.i = V(0, 0, -1);
    transform.k = V(1, 0, 0);
    core.SetBodyTransform(target, transform);
    for (int step = 0; step < 100; ++step) core.Step(.01f);
    Fmatrix actual;
    core.GetBodyTransform(body, actual);
    Near(actual.c.y, 20, .005f, "Animated prop follows translated target");
    Near(actual.i.z, -1, .005f, "Animated prop follows rotated target");
    core.DestroyBody(body);
    Require(core.GetStatistics().constraints == 0, "Animation constraint retires with controlled body");
    core.DestroyJoint(joint);
    core.DestroyBody(target);
}
void CaptureTarget(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(19.62f, 18);
    const auto anchor = V(256, 19, 548);
    const auto target = core.CreateBox(V(.01f, .01f, .01f), anchor, 1);
    core.SetBodyMotionType(target, 1);
    core.SetBodyObjectLayer(target, 4);
    const auto body = core.CreateBox(V(.5f, .5f, .5f), V(256.4f, 19, 548), 20);
    const auto joint = core.CreateJoint(3, target, body, anchor,
        V(1, 0, 0), V(0, 1, 0), V(0, 0, 1),
        V(-3.14159265f, -3.14159265f, -3.14159265f), V(3.14159265f, 3.14159265f, 3.14159265f));
    Require(joint != INVALID_JOINT_HANDLE, "Capture constraint creation");
    SPhysicsJointFeedback feedback;
    core.SetJointFeedback(joint, &feedback);
    for (int axis = 0; axis < 3; ++axis) core.SetJointMotor(joint, axis, 200, 0);
    for (int step = 0; step < 200; ++step) {
        core.SetBodyPosition(target, V(256 + .001f * step, 19, 548));
        core.Step(.01f);
    }
    Fvector position, velocity;
    core.GetBodyPosition(body, position);
    Near(position.x, 256.599f, .01f, "Captured body follows moving hand");
    Near(position.y, 19, .01f, "Captured body holds against gravity");
    Near(feedback.f2.y, 392.4f, 2.f, "Capture feedback measures supporting force");
    core.GetBodyAngularVelocity(body, velocity);
    Require(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z < .03f * .03f,
        "Capture angular motor brakes rotation");
    core.DestroyJoint(joint);
    core.DestroyBody(target);
    core.SetBodyLinearVelocity(body, V(0, 0, 0));
    for (int step = 0; step < 50; ++step) core.Step(.01f);
    core.GetBodyPosition(body, position);
    Require(position.y < 18, "Released capture falls freely");
}
void PlacementAndCharacters(IPhysicsCore& core) {
    core.Clear();
    const auto floorShape = core.CreateBoxShape(V(50, .5f, 50));
    const auto floor = core.CreateStaticBody(floorShape, V(0, -.5f, 0));
    core.DestroyCDBModel(floorShape);
    Near(core.GetBodyMass(floor), 0, 1e-5f, "Static body has no simulation mass");
    const auto box = core.CreateBoxShape(V(.5f, .5f, .5f));
    Fquaternion identity;
    identity.identity();
    Require(core.CheckShapePlacement(box, V(0, .5f, 0), identity), "Floor touching placement");
    Require(!core.CheckShapePlacement(box, V(0, .2f, 0), identity), "Penetrating placement rejected");
    Fvector free;
    Require(core.FindFreeShapePlacement(box, V(0, .2f, 0), identity, free, .5f), "Vertical placement correction");
    Require(free.y >= .498f && std::sqrt(free.x*free.x + (free.y-.2f)*(free.y-.2f) + free.z*free.z) <= .5001f,
        "Correction stays within displacement limit");
    Require(!core.FindFreeShapePlacement(box, V(0, .2f, 0), identity, free, .1f), "Bounded placement fails honestly");
    const auto capsule = core.CreateCapsuleShape(.3f, .5f);
    const auto character = core.CreateCharacterVirtual(capsule, V(4, 4, 0), 80);
    core.DestroyCDBModel(capsule);
    core.SetCharacterVirtualPosition(character, V(4, 4, 0));
    core.SetCharacterVirtualVelocity(character, V(1, 0, 0));
    core.DeactivateCharacterVirtual(character);
    core.UpdateCharacterVirtual(character, .1f, V(0, -9.81f, 0));
    Fvector position;
    core.GetCharacterVirtualPosition(character, position);
    Near(position.x, 4, 1e-5f, "Frozen character stays outside simulation");
    Require(core.GetStatistics().characters == 0, "Inactive character count");
    core.ActivateCharacterVirtual(character);
    core.SetCharacterVirtualGravityFactor(character, 0);
    Near(core.GetCharacterVirtualGravityFactor(character), 0, 1e-5f, "Character gravity accessor");
    core.UpdateCharacterVirtual(character, .1f, V(0, -9.81f, 0));
    core.GetCharacterVirtualPosition(character, position);
    Near(position.x, 4.1f, .001f, "Character velocity integration");
    Near(position.y, 4, .001f, "Character gravity disabled");
    core.SetCharacterVirtualGravityFactor(character, 1);
    core.SetCharacterVirtualVelocity(character, V(0, 0, 0));
    for (int step = 0; step < 400; ++step)
        core.UpdateCharacterVirtual(character, .01f, V(0, -9.81f, 0));
    core.GetCharacterVirtualPosition(character, position);
    Near(position.y, .8f, .05f, "Character lands on floor");
    IPhysicsCore::SJoltCharacterGroundState ground;
    core.GetCharacterVirtualGroundState(character, ground);
    Require(ground.on_ground && ground.ground_normal.y > .99f, "Character support normal");
    const float initialX = position.x;
    for (int step = 0; step < 100; ++step) {
        core.SetCharacterVirtualVelocity(character, V(1, 0, 0));
        core.UpdateCharacterVirtual(character, .01f, V(0, -9.81f, 0));
    }
    core.GetCharacterVirtualPosition(character, position);
    Near(position.x, initialX + 1, .03f, "Character walks on floor");
    Near(position.y, .8f, .05f, "Walking preserves floor support");
    core.DestroyCharacterVirtual(character);
    core.DestroyCDBModel(box);
}
int queryWall, queryOwner;
bool rejectQueryWall = false;
bool FilterQueryWall(void* object, u16, void* ignored, bool camera) {
    return object != &queryWall || (!camera && (!rejectQueryWall || ignored != &queryOwner));
}
void CharacterQueryFiltering(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(0, 18);
    const auto wallShape = core.CreateBoxShape(V(.5f, 5, 5));
    const auto wall = core.CreateStaticBody(wallShape, V(2, 4, 0));
    core.SetBodyUserData(wall, &queryWall);
    Fquaternion identity;
    identity.identity();
    Require(!core.CheckShapePlacement(wallShape, V(2, 4, 0), identity, false), "Ordinary placement respects a wall");
    Require(core.CheckShapePlacement(wallShape, V(2, 4, 0), identity, false, &queryWall), "Placement ignores its owner's body");
    core.SetQueryFilter(FilterQueryWall);
    Require(core.CheckShapePlacement(wallShape, V(2, 4, 0), identity, false, nullptr, true), "Camera placement uses the query filter");
    Require(!core.CheckShapePlacement(wallShape, V(2, 4, 0), identity, false), "Camera filtering does not suppress ordinary placement");
    core.DestroyCDBModel(wallShape);
    const auto capsule = core.CreateCapsuleShape(.3f, .5f);
    const auto character = core.CreateCharacterVirtual(capsule, V(0, 4, 0), 80);
    core.DestroyCDBModel(capsule);
    core.SetCharacterVirtualUserData(character, &queryOwner);
    core.SetCharacterVirtualVelocity(character, V(20, 0, 0));
    core.UpdateCharacterVirtual(character, .1f, V(0, 0, 0));
    Fvector position;
    core.GetCharacterVirtualPosition(character, position);
    Require(position.x < 1.3f, "Character collides with an accepted body");
    rejectQueryWall = true;
    core.SetCharacterVirtualPosition(character, V(0, 4, 0));
    core.SetCharacterVirtualVelocity(character, V(20, 0, 0));
    core.UpdateCharacterVirtual(character, .1f, V(0, 0, 0));
    core.GetCharacterVirtualPosition(character, position);
    Require(position.x > 1.9f, "Character ignores a body rejected for its owner");
    rejectQueryWall = false;
    core.SetQueryFilter(nullptr);
}
void CharacterPolicies(IPhysicsCore& core) {
    core.Clear();
    const auto shape = core.CreateCapsuleShape(.3f, .5f);
    const auto first = core.CreateCharacterVirtual(shape, V(0, 4, 0), 70);
    core.CreateCharacterVirtual(shape, V(.5f, 4, 0), 70);
    core.DestroyCDBModel(shape);
    core.SetCharacterVirtualGravityFactor(first, 0);
    core.SetCharacterVirtualContactCallback(first, nullptr, &core);
    policyCore = &core;
    contactCalls = 0;
    rejectContacts = true;
    core.SetRigidBodyContactCallback(ContactPolicy);
    for (int step = 0; step < 20; ++step) {
        core.SetCharacterVirtualVelocity(first, V(1, 0, 0));
        core.UpdateCharacterVirtual(first, .05f, V(0, 0, 0));
    }
    Fvector position;
    core.GetCharacterVirtualPosition(first, position);
    Require(contactCalls > 0 && position.x > .99f, "Character policy can suppress physical response");
    rejectContacts = false;
    core.SetRigidBodyContactCallback(nullptr);
}
PhysicsShapeHandle restrictionShape = nullptr;
int restrictedCharacter, otherCharacter;
PhysicsShapeHandle CharacterRestriction(void* character, void* other) {
    return character == &restrictedCharacter && other == &otherCharacter ? restrictionShape : nullptr;
}
void CharacterRestrictions(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(0, 18);
    const auto shape = core.CreateCapsuleShape(.3f, .5f);
    auto actor = core.CreateCharacterVirtual(shape, V(0, 4, 0), 80);
    const auto npc = core.CreateCharacterVirtual(shape, V(2, 4, 0), 80);
    core.DestroyCDBModel(shape);
    restrictionShape = core.CreateCylinderShape(1, 1);
    core.SetCharacterVirtualContactCallback(actor, nullptr, &restrictedCharacter);
    core.SetCharacterVirtualContactCallback(npc, nullptr, &otherCharacter);
    core.SetCharacterPairShapeCallback(CharacterRestriction);
    core.SetCharacterVirtualVelocity(actor, V(40, 0, 0));
    core.UpdateCharacterVirtual(actor, .1f, V(0, 0, 0));
    Fvector position;
    core.GetCharacterVirtualPosition(actor, position);
    Near(position.x, .65f, .15f, "Restriction cylinder stops fast actor before physical capsules overlap");
    core.SetCharacterVirtualPosition(actor, V(1, 4, 0));
    core.SetCharacterVirtualVelocity(actor, V(0, 0, 0));
    for (int index = 0; index < 20; ++index) core.UpdateCharacterVirtual(actor, .01f, V(0, 0, 0));
    core.GetCharacterVirtualPosition(actor, position);
    Require(position.x < .8f, "Restriction cylinder resolves an existing actor/NPC overlap");
    core.SetCharacterVirtualPosition(npc, V(100, 4, 0));
    core.SetCharacterVirtualPosition(actor, V(0, 4, 0));
    const auto wall = core.CreateBoxShape(V(.1f, 5, 5));
    core.CreateStaticBody(wall, V(1, 4, 0));
    core.DestroyCDBModel(wall);
    core.SetCharacterVirtualVelocity(actor, V(20, 0, 0));
    core.UpdateCharacterVirtual(actor, .1f, V(0, 0, 0));
    core.GetCharacterVirtualPosition(actor, position);
    Near(position.x, .58f, .12f, "World geometry uses physical capsule radius instead of NPC restriction radius");
    core.SetCharacterPairShapeCallback(nullptr);
    core.DestroyCDBModel(restrictionShape);
    restrictionShape = nullptr;
    core.Clear();
}
void CollisionOwners(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(0, 18);
    const auto first = core.CreateBox(V(.5f, .5f, .5f), V(0, 0, 0), 1);
    const auto second = core.CreateBox(V(.5f, .5f, .5f), V(.5f, 0, 0), 1);
    int owner1, owner2;
    core.SetBodyCollisionOwner(first, &owner1);
    core.SetBodyCollisionOwner(second, &owner1);
    core.SetBodyUserData(first, &owner1);
    core.SetBodyUserData(second, &owner1);
    policyCore = &core;
    contactCalls = 0;
    core.SetRigidBodyContactCallback(ContactPolicy);
    for (int step = 0; step < 20; ++step) core.Step(.01f);
    Require(contactCalls == 0 && core.GetStatistics().contact_points == 0, "Shell ownership rejects contacts before gameplay and solver");
    Fvector position;
    core.GetBodyPosition(second, position);
    Near(position.x, .5f, 1e-5f, "Overlapping elements of one shell remain unconstrained");
    core.SetBodyCollisionOwner(second, &owner2);
    for (int step = 0; step < 20; ++step) core.Step(.01f);
    Require(contactCalls > 0, "Transferred element collides with its former shell");
    core.GetBodyPosition(second, position);
    Require(position.x > .6f, "Different shell ownership permits collision response");
    core.DestroyBody(first);
    core.DestroyBody(second);
    core.SetRigidBodyContactCallback(nullptr);
}
void StaticEnvironmentPolicy(IPhysicsCore& core) {
    for (bool reverse : {false, true}) {
        core.Clear();
        core.SetSimulationParameters(0, 18);
        BodyHandle moving, obstacle;
        if (reverse) {
            obstacle = core.CreateBox(V(.5f, .5f, .5f), V(0, 0, 0), 1);
            moving = core.CreateBox(V(.5f, .5f, .5f), V(-.8f, 0, 0), 1);
        } else {
            moving = core.CreateBox(V(.5f, .5f, .5f), V(-.8f, 0, 0), 1);
            obstacle = core.CreateBox(V(.5f, .5f, .5f), V(0, 0, 0), 1);
        }
        policyCore = &core;
        core.SetBodyUserData(moving, policyCore);
        core.SetRigidBodyContactCallback(ContactPolicy);
        staticEnvironment = true;
        core.SetBodyLinearVelocity(moving, V(2, 0, 0));
        for (int step = 0; step < 20; ++step) core.Step(.01f);
        Fvector position;
        core.GetBodyPosition(obstacle, position);
        Near(position.x, 0, .00001f, "Static environment contact does not push a dynamic obstacle");
        core.GetBodyPosition(moving, position);
        Require(position.x < -.9f, "Static environment contact still corrects the moving shell");
        staticEnvironment = false;
        core.SetRigidBodyContactCallback(nullptr);
    }
}
void CollisionPolicies(IPhysicsCore& core) {
    core.Clear();
    core.SetSimulationParameters(9.81f, 18);
    policyCore = &core;
    contactCalls = geometryMask = 0;
    core.SetRigidBodyContactCallback(ContactPolicy);
    const auto floor = core.CreateBoxShape(V(50, .5f, 50));
    core.CreateStaticBody(floor, V(0, -.5f, 0));
    core.DestroyCDBModel(floor);
    const auto child = core.CreateBoxShape(V(.5f, .5f, .5f));
    PhysicsShapeHandle shapes[] = {child, child};
    Fmatrix offsets[] = {Fidentity, Fidentity};
    offsets[0].c = V(-2, 0, 0);
    offsets[1].c = V(2, 0, 0);
    const auto compound = core.CreateCompoundShape(shapes, offsets, 2);
    const auto body = core.CreateBodyFromShape(compound, V(0, .45f, 0), 4);
    core.DestroyCDBModel(compound);
    core.DestroyCDBModel(child);
    core.SetBodyUserData(body, &core);
    rejectContacts = false;
    for (int index = 0; index < 100; ++index) core.Step(.01f);
    Require(contactCalls > 0 && geometryMask == 3, "Callbacks preserve compound child identities");
    rejectContacts = true;
    core.ActivateBody(body);
    for (int index = 0; index < 100; ++index) core.Step(.01f);
    Fvector position;
    core.GetBodyPosition(body, position);
    Require(position.y < -2, "Callback suppresses cached physical response");
    core.Clear();
    core.SetSimulationParameters(0, 18);
    const auto wall = core.CreateBoxShape(V(.05f, 10, 10));
    core.CreateStaticBody(wall, V(0, 0, 0));
    core.DestroyCDBModel(wall);
    const auto missile = core.CreateBox(V(.1f, .1f, .1f), V(-2, 0, 0), 1);
    core.SetBodyUserData(missile, &core);
    core.SetBodyContinuousCollision(missile, true);
    core.SetBodyLinearVelocity(missile, V(500, 0, 0));
    contactCalls = 0;
    stopImpact = true;
    core.Step(.01f);
    core.GetBodyPosition(missile, position);
    Require(contactCalls > 0 && position.x < -.1f, "Fast impact callback precedes native integration");
    stopImpact = rejectContacts = false;
    core.SetRigidBodyContactCallback(nullptr);
}
bool immediateFixture = false;
bool fluidFixture = false;
int fluidMarker = 0;
u32 deferredCalls = 0;
std::thread::id callerThread;
NativeBodyContactPolicy BodyPolicy(void* data, u16) {
    if (data == &fluidMarker) return {false, false, -1, true};
    return {data && immediateFixture, data && fluidFixture, data ? contactFriction : -1.f};
}
void DeferredPolicy(const NativePhysicsContact& contact) {
    Require(std::this_thread::get_id() == callerThread, "Deferred effects run on the caller thread");
    Require(contact.has_snapshot && contact.mass1 + contact.mass2 > 0, "Deferred effects retain impact mass/velocity");
    Fvector velocity;
    policyCore->GetBodyLinearVelocity(contact.object1 ? contact.body1 : contact.body2, velocity);
    Require(std::isfinite(contact.relative_velocity), "Deferred contact snapshot is finite");
    ++deferredCalls;
}
void OptimizedContactsAndSleeping(IPhysicsCore& core) {
    policyCore = &core;
    callerThread = std::this_thread::get_id();
    core.Clear();
    core.SetSimulationParameters(9.81f, 18);
    const auto floorShape = core.CreateBoxShape(V(50, .5f, 50));
    const auto floor = core.CreateStaticBody(floorShape, V(0, -.5f, 0));
    core.DestroyCDBModel(floorShape);
    const auto box = core.CreateBox(V(.5f, .5f, .5f), V(0, 2, 0), 2);
    core.SetBodyUserData(box, &core);
    core.SetRigidBodyContactCallback(ContactPolicy);
    core.SetBodyContactPolicyCallback(BodyPolicy);
    core.SetDeferredRigidBodyContactCallback(DeferredPolicy);
    contactFriction = 1;
    contactCalls = deferredCalls = 0;
    immediateFixture = false;
    for (int step = 0; step < 100; ++step) core.Step(.01f);
    Require(contactCalls == 0 && deferredCalls > 0, "Pure contacts skip gameplay preparation and retain effects");
    core.SetBodyUserData(floor, &fluidMarker);
    fluidFixture = true;
    contactCalls = deferredCalls = 0;
    core.ActivateBody(box);
    core.Step(.01f);
    Require(contactCalls > 0 && deferredCalls == 0, "Fluid effects retain their pre-integration callback timing");
    fluidFixture = false;
    core.SetBodyUserData(floor, nullptr);
    core.SetBodyLinearVelocity(box, V(4, 0, 0));
    for (int step = 0; step < 60; ++step) core.Step(.01f);
    Fvector velocity;
    core.GetBodyLinearVelocity(box, velocity);
    Require(std::abs(velocity.x) < 1, "Native body friction policy slows a sliding box");
    contactFriction = 0;
    core.SetBodyLinearVelocity(box, V(4, 0, 0));
    for (int step = 0; step < 60; ++step) core.Step(.01f);
    core.GetBodyLinearVelocity(box, velocity);
    Require(velocity.x > 3, "Changed native friction policy applies to persistent contacts");
    immediateFixture = rejectContacts = true;
    core.ActivateBody(box);
    for (int step = 0; step < 100; ++step) core.Step(.01f);
    Fvector position;
    core.GetBodyPosition(box, position);
    Require(contactCalls > 0 && position.y < -2, "Immediate callbacks retain rejection before integration");
    rejectContacts = immediateFixture = false;
    core.SetBodyContactPolicyCallback(nullptr);
    core.SetDeferredRigidBodyContactCallback(nullptr);
    core.SetRigidBodyContactCallback(nullptr);

    core.Clear();
    core.SetSimulationParameters(0, 18);
    const auto hingeBody = core.CreateBox(V(.5f, .5f, .5f), V(0, 0, 0), 2);
    const auto hinge = core.CreateJoint(1, INVALID_BODY_HANDLE, hingeBody, V(0, 0, 0),
        V(0, 1, 0), V(1, 0, 0), V(0, 0, 1), V(-3, 0, 0), V(3, 0, 0));
    core.SetJointMotor(hinge, 0, 10, 0);
    for (int step = 0; step < 200; ++step) {
        core.SetJointMotor(hinge, 0, 10, 0);
        core.SetJointLimits(hinge, 0, -3, 3);
        core.SetBodyTransform(hingeBody, Fidentity);
        core.Step(.01f);
    }
    Require(!core.IsBodyActive(hingeBody), "Unchanged motor/limits/transform permit sleeping");
    core.SetJointMotor(hinge, 0, 10, 1);
    Require(core.IsBodyActive(hingeBody), "A changed motor wakes the body");
    core.Step(.01f);
    core.GetBodyAngularVelocity(hingeBody, velocity);
    Require(std::abs(velocity.y) > .01f, "Changed motor drives its constraint");
}
void ChangingPassiveResistance(IPhysicsCore& core) {
    for (const int type : {1, 3, 4}) { // hinge, full-control and slider
        core.Clear();
        core.SetSimulationParameters(0, 18);
        const auto body = core.CreateBox(V(.5f, .5f, .5f), V(0, 0, 0), 2);
        const auto joint = core.CreateJoint(type, INVALID_BODY_HANDLE, body, V(0, 0, 0),
            V(1, 0, 0), V(0, 1, 0), V(0, 0, 1), V(-3, -3, -3), V(3, 3, 3));
        Require(joint != INVALID_JOINT_HANDLE, "Passive resistance fixture joint");
        for (int step = 0; step < 200; ++step) {
            core.SetJointMotor(joint, 0, 1.f + step * .01f, 0);
            core.Step(.01f);
        }
        Require(!core.IsBodyActive(body), "Changing passive resistance permits native sleeping");
        core.SetJointMotor(joint, 0, 10, 0);
        Require(!core.IsBodyActive(body), "A changed brake cap does not wake a resting body");
        if (type == 4) core.SetBodyLinearVelocity(body, V(2, 0, 0));
        else core.SetBodyAngularVelocity(body, V(2, 0, 0));
        Require(core.IsBodyActive(body), "External motion wakes a body with passive resistance");
        core.Step(.01f);
        Fvector velocity;
        if (type == 4) core.GetBodyLinearVelocity(body, velocity);
        else core.GetBodyAngularVelocity(body, velocity);
        Require(velocity.x < 1.99f && velocity.x >= -.01f, "Native passive friction brakes imposed motion");
        for (int step = 0; step < 200; ++step) core.Step(.01f);
        Require(!core.IsBodyActive(body), "A braked body returns to rest");
        core.SetJointMotor(joint, 0, 10, 1);
        Require(core.IsBodyActive(body), "A powered drive still wakes a resting body");
        core.Step(.01f);
        if (type == 4) core.GetBodyLinearVelocity(body, velocity);
        else core.GetBodyAngularVelocity(body, velocity);
        Require(velocity.x > .01f, "Powered drive replaces passive friction");
        core.SetJointMotor(joint, 0, 10, 0);
        for (int step = 0; step < 200; ++step) core.Step(.01f);
        Require(!core.IsBodyActive(body), "Stopping a powered drive restores sleeping passive resistance");
    }
}
int preparedDryHits, preparedWetHits, deferredDryHits, deferredWetHits;
bool TrackFluidPreparation(const NativePhysicsContact& contact) {
    if (contact.material1 == 1 || contact.material2 == 1) ++preparedWetHits;
    else ++preparedDryHits;
    return ContactPolicy(contact);
}
void TrackFluidEffects(const NativePhysicsContact& contact) {
    if (contact.material1 == 1 || contact.material2 == 1) ++deferredWetHits;
    else ++deferredDryHits;
    DeferredPolicy(contact);
}
void FluidExclusionRegions(IPhysicsCore& core) {
    struct Fixture {
        IPhysicsCore& core;
        SGameMtl dry, wet;
        xr_vector<SGameMtl*>& materials;
        Fixture(IPhysicsCore& physics) : core(physics),
            materials(const_cast<xr_vector<SGameMtl*>&>(GMLib.Materials())) {
            Require(materials.empty(), "Standalone fluid fixture owns an empty material library");
            core.Clear();
            materials.reserve(2);
            dry.m_Name = "fixture_dry";
            wet.m_Name = "fixture_wet";
            wet.Flags.set(SGameMtl::flSlowDown, true);
            materials.push_back(&dry);
            materials.push_back(&wet);
        }
        ~Fixture() {
            core.SetBodyContactPolicyCallback(nullptr);
            core.SetDeferredRigidBodyContactCallback(nullptr);
            core.SetRigidBodyContactCallback(nullptr);
            core.Clear();
            materials.clear();
            fluidFixture = false;
        }
    } fixture(core);
    NativePhysicsContact materialContact{};
    materialContact.material1 = 1;
    materialContact.material2 = 0;
    Require(NativeContactMaterialIndex(&fixture.dry, &materialContact) == 0 &&
        NativeContactMaterialIndex(&fixture.wet, &materialContact) == 1, "Native effect material indices follow resolved pointers");
    materialContact.material1 = materialContact.material2 = 1;
    Require(NativeContactMaterialIndex(&fixture.dry, &materialContact) == 0, "Changed callback materials use the name fallback");
    materialContact.material1 = materialContact.material2 = u16(-1);
    Require(NativeContactMaterialIndex(&fixture.wet, &materialContact) == 1 &&
        NativeContactMaterialIndex(&fixture.dry, nullptr) == 0, "Untagged effect materials use the name fallback");
    policyCore = &core;
    callerThread = std::this_thread::get_id();
    immediateFixture = false;
    fluidFixture = true;
    contactFriction = 0;
    core.SetSimulationParameters(0, 18);
    xr_vector<Fvector> vertices;
    xr_vector<CDB::TRI> triangles;
    auto patch = [&](int firstX, int lastX, u16 material) {
        for (int x = firstX; x < lastX; ++x) for (int z = -4; z < 4; ++z) {
            const auto base = static_cast<u32>(vertices.size());
            vertices.push_back(V(float(x), 0, float(z)));
            vertices.push_back(V(float(x + 1), 0, float(z)));
            vertices.push_back(V(float(x), 0, float(z + 1)));
            vertices.push_back(V(float(x + 1), 0, float(z + 1)));
            CDB::TRI triangle{};
            triangle.material = material;
            triangle.verts[0] = base; triangle.verts[1] = base + 2; triangle.verts[2] = base + 1;
            triangles.push_back(triangle);
            triangle.verts[0] = base + 1; triangle.verts[1] = base + 2; triangle.verts[2] = base + 3;
            triangles.push_back(triangle);
        }
    };
    // Separate leaves are essential: a leaf containing both materials would
    // conservatively request preparation and never establish a dry region.
    patch(-8, 0, 0);
    patch(8, 12, 1);
    const auto shape = core.BuildCDBModel(vertices.data(), static_cast<u32>(vertices.size()),
        triangles.data(), static_cast<u32>(triangles.size()));
    Require(shape != nullptr, "Fluid fixture mesh");
    const auto mesh = core.CreateStaticBody(shape, V(0, 0, 0));
    core.DestroyCDBModel(shape);
    const auto box = core.CreateBox(V(.5f, .5f, .5f), V(-6, .49f, 0), 2);
    core.SetBodyUserData(box, &core);
    core.SetRigidBodyContactCallback(ContactPolicy);
    core.SetBodyContactPolicyCallback(BodyPolicy);
    core.SetDeferredRigidBodyContactCallback(DeferredPolicy);
    auto place = [&](BodyHandle handle, const Fvector& position) {
        Fmatrix transform = Fidentity;
        transform.c = position;
        core.SetBodyTransform(handle, transform);
        if (handle == box) core.SetBodyLinearVelocity(box, V(0, 0, 0));
    };
    auto drySteps = [&] {
        contactCalls = deferredCalls = 0;
        for (int step = 0; step < 4; ++step) { core.ActivateBody(box); core.Step(.01f); }
        Require(contactCalls == 0 && deferredCalls > 0, "Dry mesh contacts bypass fluid preparation across steps");
    };
    auto wetStep = [&](const char* message) {
        contactCalls = deferredCalls = 0;
        core.ActivateBody(box);
        core.Step(.01f);
        Require(contactCalls > 0 && deferredCalls == 0, message);
    };
    drySteps();
    place(box, V(9, .49f, 0));
    wetStep("Moving out of a cached dry region retains fluid preparation");
    place(box, V(-6, .49f, 0));
    drySteps();
    place(mesh, V(-15, 0, 0));
    wetStep("Moving a mesh beneath a body retains fluid preparation");
    place(mesh, V(0, 0, 0));
    place(box, V(-6, .49f, 0));
    drySteps();
    fixture.dry.Flags.set(SGameMtl::flSlowDown, true);
    wetStep("Changing material flags invalidates cached dry regions without a count change");

    // Both materials share one BVH leaf. Broad leaf bounds cannot distinguish
    // the dry floor from the wet wall; filtering must happen before the exact
    // triangle collision and dry effects must still arrive from the solver.
    fixture.dry.Flags.set(SGameMtl::flSlowDown, false);
    const Fvector mixedVertices[]{V(-4, 0, -4), V(4, 0, -4), V(-4, 0, 4), V(4, 0, 4),
        V(0, 0, -4), V(0, 4, -4), V(0, 0, 4), V(0, 4, 4)};
    CDB::TRI mixedTriangles[4]{};
    const u32 indices[4][3]{{0, 2, 1}, {1, 2, 3}, {4, 6, 5}, {5, 6, 7}};
    for (u32 index = 0; index < 4; ++index) {
        for (u32 vertex = 0; vertex < 3; ++vertex) mixedTriangles[index].verts[vertex] = indices[index][vertex];
        mixedTriangles[index].material = index < 2 ? 0 : 1;
    }
    for (bool sphere : {false, true}) {
        core.Clear();
        core.SetSimulationParameters(0, 18);
        const auto mixed = core.BuildCDBModel(mixedVertices, 8, mixedTriangles, 4);
        core.CreateStaticBody(mixed, V(0, 0, 0));
        core.DestroyCDBModel(mixed);
        const auto movingShape = sphere ? core.CreateSphereShape(.5f) : core.CreateBoxShape(V(.5f, .5f, .5f));
        const auto moving = core.CreateBodyFromShape(movingShape, V(-.49f, .49f, 0), 2);
        core.DestroyCDBModel(movingShape);
        core.SetBodyUserData(moving, &core);
        core.SetBodyContactPolicyCallback(BodyPolicy);
        core.SetRigidBodyContactCallback(TrackFluidPreparation);
        core.SetDeferredRigidBodyContactCallback(TrackFluidEffects);
        auto resetCounts = [&] { preparedDryHits = preparedWetHits = deferredDryHits = deferredWetHits = 0; };
        resetCounts();
        core.Step(.01f);
        Require(preparedWetHits > 0 && preparedDryHits == 0, "Mixed-leaf preparation retains only fluid triangles");
        Require(deferredDryHits > 0 && deferredWetHits == 0, "Mixed-leaf dry effects survive without duplicate fluid effects");
        Fmatrix transform = Fidentity;
        transform.c = V(-.49f, .49f, 0);
        core.SetBodyTransform(moving, transform);
        core.SetBodyLinearVelocity(moving, V(0, 0, 0));
        immediateFixture = true;
        resetCounts();
        core.ActivateBody(moving);
        core.Step(.01f);
        Require(preparedWetHits > 0 && preparedDryHits > 0 && deferredDryHits == 0 && deferredWetHits == 0,
            "Response-changing policies retain every mixed-leaf preparatory contact");
        immediateFixture = false;
        transform.c = V(-2, .49f, 0);
        core.SetBodyTransform(moving, transform);
        core.SetBodyLinearVelocity(moving, V(200, 0, 0));
        core.SetBodyContinuousCollision(moving, true);
        resetCounts();
        core.Step(.01f);
        Require(preparedWetHits > 0 && preparedDryHits == 0, "Mixed-leaf fluid sweeps retain first-step preparation");
    }
}
}
int main() {
    Core.Initialize("NativePhysicsTests", "", false);
    int result = 0;
    auto* core = GetPhysicsCore();
    try {
        Motion(*core);
        CompoundTransforms(*core);
        ContactEffectEligibility();
        Contact(*core);
        ContactFeedback(*core);
        Joints(*core);
        MassCenterAndSuspension(*core);
        AnchoredDoor(*core);
        AnimatedTarget(*core);
        CaptureTarget(*core);
        PlacementAndCharacters(*core);
        CharacterQueryFiltering(*core);
        CharacterPolicies(*core);
        CharacterRestrictions(*core);
        CollisionOwners(*core);
        StaticEnvironmentPolicy(*core);
        CollisionPolicies(*core);
        OptimizedContactsAndSleeping(*core);
        ChangingPassiveResistance(*core);
        FluidExclusionRegions(*core);
        core->Clear();
        Require(core->GetStatistics().bodies == 0 && core->GetStatistics().characters == 0 &&
            core->GetStatistics().constraints == 0, "World teardown");
        std::printf("Native physics checks passed (workers=%u)\n", core->GetStatistics().workers);
    } catch (const std::exception& exception) {
        std::fprintf(stderr, "FAIL: %s\n", exception.what());
        result = 1;
    }
    core->Destroy();
    Core._destroy();
    return result;
}
