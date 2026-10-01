#include "StdAfx.h"
#include "GameplayBenchmark.h"
#include "PHWorld.h"
#include "PhysicsShell.h"
#include "PHShell.h"
#include "PHElement.h"
#include "PHJoint.h"
#include "PHActorCharacter.h"
#include "PHAICharacter.h"
#include "IClimableObject.h"
#include "IPhysicsShellHolder.h"
#include "xrEngine/device.h"
#include "xrEngine/defines.h"
#include "xrCDB/xr_area.h"
#include "xrCore/Animation/Bone.hpp"
#include "console_vars.h"
#include "icollisiondamagereceiver.h"
#include <array>
#include <random>

namespace
{
#ifdef XRAY_NATIVE_JOLT_PHYSICS
constexpr pcstr dynamics = "JoltNative";
#elif defined(XRAY_USE_JOLT_PHYSICS)
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
    output->w_string("kind,phase,collision_backend,dynamics_backend,index,dt_ms,total_ms,collision_ms,solver_ms,islands,bodies,joints,contacts,native_preparation_ms,native_integration_ms,native_feedback_ms");
    for (const auto& sample : steps)
        output->w_printf("step,%s,%s,%s,%llu,%.6f,%.6f,%.6f,%.6f,%u,%u,%u,%u,%.6f,%.6f,%.6f\n",
            phase.c_str(), collision, dynamics, static_cast<unsigned long long>(sample.step),
            sample.dt * 1000., sample.total, sample.collision, sample.solver,
            sample.islands, sample.bodies, sample.joints, sample.contacts,
            sample.nativePreparation, sample.nativeIntegration, sample.nativeFeedback);
    for (const auto& sample : frames)
        output->w_printf("frame,%s,%s,%s,%u,0,%.6f,0,0,0,0,0,0,0,0,0\n",
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
    for (auto* object : m_objects)
    {
        if (object->CastType() != CPHObject::tpShell) continue;
        auto* shell = static_cast<CPHShell*>(object);
        auto* holder = shell->PhysicsRefObject();
        u32 active = 0;
        float linearSpeed = 0, angularSpeed = 0;
        for (u16 index = 0; index < shell->get_ElementsNumber(); ++index) {
            auto* element = static_cast<CPHElement*>(shell->get_ElementByStoreOrder(index));
            const auto body = element->get_body();
            if (GetPhysicsCore()->IsBodyActive(body)) ++active;
            Fvector velocity;
            GetPhysicsCore()->GetBodyLinearVelocity(body, velocity);
            linearSpeed = std::max(linearSpeed, velocity.magnitude());
            GetPhysicsCore()->GetBodyAngularVelocity(body, velocity);
            angularSpeed = std::max(angularSpeed, velocity.magnitude());
            Fvector position;
            GetPhysicsCore()->GetBodyPosition(body, position);
            Msg("GAMEPLAY_BENCHMARK ELEMENT phase=%s name=%s index=%u fixed=%u body=%u position=%f,%f,%f",
                phase, holder ? holder->ObjectName() : "none", index, element->isFixed(), body,
                position.x, position.y, position.z);
        }
        for (u16 index = 0; index < shell->get_JointsNumber(); ++index) {
            auto* joint = static_cast<CPHJoint*>(shell->get_JointByStoreOrder(index));
            Fvector anchor;
            joint->GetAnchorDynamic(anchor);
            Msg("GAMEPLAY_BENCHMARK JOINT phase=%s name=%s index=%u handle=%u anchor=%f,%f,%f",
                phase, holder ? holder->ObjectName() : "none", index, joint->GetJointHandle(),
                anchor.x, anchor.y, anchor.z);
        }
        Msg("GAMEPLAY_BENCHMARK SHELL phase=%s name=%s section=%s elements=%u active=%u max_v=%f max_w=%f",
            phase, holder ? holder->ObjectName() : "none", holder ? holder->ObjectNameSect() : "none",
            shell->get_ElementsNumber(), active, linearSpeed, angularSpeed);
    }
}
void CPHWorld::BenchmarkEnd() { m_benchmark->End(); }
void CPHWorld::BenchmarkForceShell(CPhysicsShell* shell) { m_benchmark->ForceShell(shell); }
void CPHWorld::BenchmarkQueries(const Fvector& origin)
{
    R_ASSERT2(!m_benchmark->Active(), "Run query batches outside frame/physics capture");
    GameplayBenchmark::Queries(ObjectSpace(), origin);
}

bool CPHWorld::BenchmarkNativeChecks(const Fvector& origin)
{
    R_ASSERT2(!psDeviceFlags.test(mtPhysics), "Native gameplay checks require mt_physics off");
    bool passed = true;
    auto check = [&](bool condition, pcstr name) {
        Msg("NATIVE_GAMEPLAY_CHECK %s %s", condition ? "PASS" : "FAIL", name);
        passed = passed && condition;
    };
    auto* core = GetPhysicsCore();
    const auto initialBodies = core->GetStatistics().bodies;
    const auto initialCharacters = core->GetStatistics().characters;
    {
        CPHActorCharacter actor(true);
        CPHAICharacter npc;
        actor.Create(Fvector().set(.6f, 1.8f, .6f));
        npc.Create(Fvector().set(.6f, 1.8f, .6f));
        npc.SetRestrictionType(rtStalker);
        npc.SetObjectRadius(1.2f);
        actor.SetRestrictorRadius(rtStalker, 1);
        actor.SetRestrictorRadius(rtStalkerSmall, .4f);
        Fvector position = origin;
        position.y += 20;
        actor.SetPosition(position);
        position.x += 2;
        npc.SetPosition(position);
        auto* restriction = actor.NativeRestrictionGeometry(&npc);
        check(restriction && std::abs(restriction->radius() - 1.f) < .001f,
            "actor owns native NPC restriction cylinder");
        const float initialX = origin.x;
        core->SetCharacterVirtualVelocity(actor.get_body(), Fvector().set(40, 0, 0));
        core->UpdateCharacterVirtual(actor.get_body(), .1f, Fvector().set(0, 0, 0));
        actor.GetPosition(position);
        check(position.x > initialX + .5f && position.x < initialX + .8f,
            "gameplay character policy preserves actor NPC spacing");
        actor.SetRestrictorRadius(rtStalker, .8f);
        restriction = actor.NativeRestrictionGeometry(&npc);
        check(restriction && std::abs(restriction->radius() - .8f) < .001f,
            "actor restriction radius updates native shape");
        npc.SetRestrictionType(rtNone);
        check(!actor.NativeRestrictionGeometry(&npc), "unrestricted NPC uses physical capsule");
        Fsphere characterSphere;
        characterSphere.P = origin;
        characterSphere.R = .1f;
        CSphereGeom characterGeometry(characterSphere);
        characterGeometry.ph_object = &npc;
        core->SetCharacterVirtualVelocity(npc.get_body(), Fvector().set(10, 0, 0));
        check(std::abs(NativeCollisionEnergy(&characterGeometry, nullptr, Fvector().set(-1, 0, 0)) -
            50 * npc.Mass()) < .01f, "virtual NPC collision damage uses character mass and velocity");
        npc.GetPosition(position);
        const float beforeForcedStep = position.x;
        npc.Enable();
        npc.SetVelocity(Fvector().set(0, 0, 0));
        npc.setForce(Fvector().set(npc.Mass() * 1000, 0, 0));
        static_cast<CPHCharacter&>(npc).step(.01f);
        npc.GetPosition(position);
        check(std::abs(position.x - beforeForcedStep - .1f) < .002f,
            "movement controller forced step advances only its character");
        SPHNetState saved, restored;
        npc.get_State(saved);
        NET_Packet packet;
        saved.net_Save(packet);
        packet.r_seek(0);
        restored.net_Load(packet);
        check(restored.position.similar(saved.position, .0001f) && restored.linear_vel.similar(saved.linear_vel, .0001f) &&
            restored.enabled == saved.enabled, "native character state preserves legacy save encoding");
        npc.SetPosition(Fvector().set(position).add(Fvector().set(4, 0, 0)));
        npc.set_State(restored);
        npc.GetPosition(position);
        check(position.similar(saved.position, .0001f), "native character state restores saved position");
        class Ladder final : public IClimableObject {
            Fvector normal = {0, 0, -1};
        public:
            float DDAxis(Fvector& direction) const override { direction.set(0, 1, 0); return 10; }
            float DDSide(Fvector& direction) const override { direction.set(1, 0, 0); return 1; }
            const Fvector& Norm() const override { return normal; }
            float DDNorm(Fvector& direction) const override { direction = normal; return 1; }
            bool BeforeLadder(CPHCharacter*, float) const override { return true; }
            float DDLowerP(CPHCharacter*, Fvector& direction) const override { direction.set(0, -5, 0); return 5; }
            float DDUpperP(CPHCharacter*, Fvector& direction) const override { direction.set(0, 5, 0); return 5; }
            float DDToAxis(CPHCharacter*, Fvector& direction) const override { direction.set(0, 0, .1f); return .1f; }
            float AxDistToUpperP(CPHCharacter*) const override { return 5; }
            float AxDistToLowerP(CPHCharacter*) const override { return 5; }
            void DToPlain(CPHCharacter*, Fvector& direction) const override { direction.set(0, 0, .1f); }
            float DDToPlain(CPHCharacter*, Fvector& direction) const override { direction.set(0, 0, .1f); return .1f; }
            bool InTouch(CPHCharacter*) const override { return true; }
            u16 Material() const override { return 0; }
            IPhysicsShellHolder* cast_IPhysicsShellHolder() override { return nullptr; }
        } ladder;
        actor.GetPosition(position);
        position.x -= 5;
        actor.SetPosition(position);
        actor.SetAcceleration(Fvector().set(0, 0, 1));
        actor.SetCamDir(Fvector().set(0, 0, 1));
        actor.SetElevator(&ladder);
        position.x += .01f;
        actor.SetPosition(position);
        auto* elevator = actor.ElevatorState();
        elevator->PhTune(.01f);
        check(elevator->ClimbingState() && core->GetCharacterVirtualGravityFactor(actor.get_body()) == 0,
            "entering ladder climb disables native character gravity");
        position.x += .01f;
        actor.SetPosition(position);
        elevator->Depart();
        check(!elevator->ClimbingState() && core->GetCharacterVirtualGravityFactor(actor.get_body()) == 1,
            "leaving ladder climb restores native character gravity");
        actor.Destroy();
        npc.Destroy();
    }
    check(core->GetStatistics().characters == initialCharacters, "temporary gameplay characters retired");
    {
        CPHShell shell;
        CPHElement element;
        element.SetShell(&shell);
        Fobb box;
        box.m_rotate.identity();
        box.m_halfsize.set(.5f, .5f, .5f);
        box.m_translate.set(-1, 0, 0);
        element.add_Box(box);
        element.setMassMC(2, box.m_translate);
        box.m_translate.x = 1;
        element.add_Box(box);
        SBoneShape shape;
        shape.type = SBoneShape::stNone;
        element.add_Mass(shape, Fidentity, box.m_translate, 8, nullptr);
        check(std::abs(element.local_mass_Center().x - .6f) < 1e-5f, "bone-weighted center of mass");
        Fvector center;
        const auto first = element.CalculateMassProperties(0, 1, true, center);
        check(std::abs(first.mass - 2) < 1e-5f && std::abs(center.x + 1.6f) < 1e-5f, "first fracture mass and center");
        const auto second = element.CalculateMassProperties(1, 2, true, center);
        check(std::abs(second.mass - 8) < 1e-5f && std::abs(center.x - .4f) < 1e-5f, "second fracture mass and center");
        element.build();
        const auto body = element.get_body();
        const auto mass = core->GetBodyMassProperties(body);
        check(std::abs(mass.mass - 10) < 1e-4f && std::abs(mass.inertia.j.y - 8.0666667f) < .001f,
            "bone-weighted native inertia includes parallel-axis contribution");
        Fmatrix transform = Fidentity;
        transform.c = origin;
        transform.c.y += 5;
        core->SetBodyTransform(body, transform);
        Fsphere sphere;
        sphere.P = transform.c;
        sphere.R = .1f;
        CSphereGeom other(sphere);
        CPhysicsGeom* measured = nullptr;
        float impact = -1;
        bool invert = false;
        core->SetBodyLinearVelocity(body, Fvector().set(0, 0, 0));
        core->SetBodyAngularVelocity(body, Fvector().set(0, 0, 0));
        core->SetBodyLinearVelocity(body, Fvector().set(10, 0, 0));
        check(std::abs(NativeCollisionEnergy(element.Geom(0), &other, Fvector().set(-1, 0, 0)) - 500) < .01f,
            "collision damage uses actual mass and normal impact energy");
        check(NativeCollisionEnergy(element.Geom(0), &other, Fvector().set(0, 1, 0)) == 0,
            "tangential motion does not cause collision damage");
        check(NativeCollisionEnergy(element.Geom(0), &other, Fvector().set(1, 0, 0)) == 0,
            "separating contact does not cause collision damage");
        core->SetBodyLinearVelocity(body, Fvector().set(0, 0, 0));
        check(ContactShotMarkGetEffectPars(transform.c, Fvector().set(0, 1, 0), element.Geom(0), &other,
            measured, impact, invert) && impact == 0, "resting contact has zero impact criterion");
        Fvector position = transform.c;
        position.x += 1;
        core->SetBodyAngularVelocity(body, Fvector().set(0, 0, 2));
        check(ContactShotMarkGetEffectPars(position, Fvector().set(0, 1, 0), element.Geom(0), &other,
            measured, impact, invert) && std::abs(impact - 2 * std::sqrt(10.f)) < .001f,
            "impact criterion includes contact-point angular velocity");
        for (int rotated = 0; rotated < 2; ++rotated) {
            CPHFracture fracture;
            fracture.m_start_geom_num = 1;
            fracture.m_end_geom_num = 2;
            fracture.m_break_force = 15 * ph_console::phBreakCommonFactor * ph_console::phRigidBreakWeaponFactor;
            fracture.m_break_torque = 1000000;
            const auto fractureIndex = element.setGeomFracturable(fracture);
            transform.rotateZ(rotated * PI_DIV_2);
            transform.c = origin;
            transform.c.y += 5;
            core->SetBodyTransform(body, transform);
            Fvector force;
            transform.transform_dir(force, Fvector().set(0, -100, 0));
            element.FracturesHolder()->Impacts().clear();
            element.FracturesHolder()->AddImpact(force, Fvector().set(.4f, 0, 0), 1);
            element.FracturesHolder()->Impacts().back().force.mul(.1f);
            check(!element.Fracture(fractureIndex).Update(&element),
                rotated ? "rotated subthreshold impact preserves fracture" : "subthreshold impact preserves fracture");
            element.FracturesHolder()->Impacts().clear();
            element.FracturesHolder()->AddImpact(force, Fvector().set(.4f, 0, 0), 1);
            check(element.Fracture(fractureIndex).Update(&element),
                rotated ? "rotated body fracture from localized impact" : "fracture from localized impact");
            element.DeleteFracturesHolder();
        }
        CPHFracture split;
        split.m_start_geom_num = 1;
        split.m_end_geom_num = 2;
        split.m_bone_id = 1;
        split.m_start_el_num = split.m_end_el_num = 0;
        split.m_start_jt_num = split.m_end_jt_num = 0;
        split.m_breaked = true;
        element.setGeomFracturable(split);
        core->SetBodyLinearVelocity(body, Fvector().set(3, 1, 0));
        core->SetBodyAngularVelocity(body, Fvector().set(0, 0, 2));
        Fmatrix originalGeometry[2];
        element.Geom(0)->get_xform(originalGeometry[0]);
        element.Geom(1)->get_xform(originalGeometry[1]);
        Fvector expectedVelocity[2];
        core->GetBodyPointVelocity(body, originalGeometry[0].c, expectedVelocity[0]);
        core->GetBodyPointVelocity(body, originalGeometry[1].c, expectedVelocity[1]);
        ELEMENT_PAIR_VECTOR pieces;
        element.SplitProcess(pieces);
        check(pieces.size() == 1, "fracture produces one detached element");
        if (pieces.size() == 1) {
            auto* detached = pieces.front().first;
            check(std::abs(element.getMass() - 2) < .001f && std::abs(detached->getMass() - 8) < .001f,
                "split preserves individual bone masses");
            CPHElement* parts[] = {&element, detached};
            for (int index = 0; index < 2; ++index) {
                Fmatrix geometry;
                parts[index]->Geom(0)->get_xform(geometry);
                check(geometry.c.similar(originalGeometry[index].c, .001f) &&
                    geometry.i.similar(originalGeometry[index].i, .001f),
                    index ? "detached geometry stays in world position" : "source geometry stays in world position");
                Fvector velocity;
                core->GetBodyLinearVelocity(parts[index]->get_body(), velocity);
                check(velocity.similar(expectedVelocity[index], .001f),
                    index ? "detached body inherits velocity at new mass center" : "source body inherits velocity at new mass center");
                const auto properties = core->GetBodyMassProperties(parts[index]->get_body());
                check(std::abs(properties.inertia.j.y - parts[index]->getMass() / 6) < .001f,
                    index ? "detached inertia is centered on its own mass center" : "source inertia is centered on its own mass center");
                check(parts[index]->Geom(0)->element_position() == 0 && parts[index]->Geom(0)->owner == parts[index],
                    index ? "detached geometry owner and index updated" : "source geometry owner and index updated");
            }
            xr_delete(detached);
        }
        element.destroy();
    }
    check(core->GetStatistics().bodies == initialBodies, "temporary gameplay bodies retired");
    return passed;
}
