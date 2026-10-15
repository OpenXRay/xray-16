#include "stdafx.h"
#include "JoltPhysicsCore.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "Jolt-proj/XRayContactFeedback.h"
#include "Jolt-proj/XRayMeshQuery.h"
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/TransformedShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <chrono>
#include <cstdlib>

namespace {
u16 Geometry(const JPH::Shape* shape, JPH::SubShapeID id) {
    while (shape->GetType() == JPH::EShapeType::Decorated)
        shape = static_cast<const JPH::DecoratedShape*>(shape)->GetInnerShape();
    if (shape->GetType() != JPH::EShapeType::Compound) return 0;
    auto* compound = static_cast<const JPH::CompoundShape*>(shape);
    JPH::SubShapeID remainder;
    return static_cast<u16>(compound->GetSubShape(compound->GetSubShapeIndexFromID(id, remainder)).mUserData);
}
u16 Material(const JPH::Shape* shape, JPH::SubShapeID id) {
    JPH::SubShapeID remainder;
    const auto* leaf = shape->GetLeafShape(id, remainder);
    if (leaf->GetSubType() == JPH::EShapeSubType::Mesh) {
        return JPH::XRayTriangleMaterial(shape, id);
    }
    const auto data = leaf->GetUserData();
    return data & (u64(1) << 63) ? static_cast<u16>(data) : u16(-1);
}
struct OtherBodyFilter : JPH::BodyFilter {
    JPH::BodyID self;
    JPH::CollisionGroup group;
    const NativePhysicsContact& first;
    IPhysicsCore::CollisionFilterFun callback;
    const std::vector<JPH::BodyID>* connected;
    OtherBodyFilter(JPH::BodyID id, const JPH::CollisionGroup& collisionGroup,
        const NativePhysicsContact& contact, IPhysicsCore::CollisionFilterFun filter,
        const std::vector<JPH::BodyID>* excluded)
        : self(id), group(collisionGroup), first(contact), callback(filter), connected(excluded) {}
    bool ShouldCollide(const JPH::BodyID& id) const override {
        return id != self && (!connected || std::find(connected->begin(), connected->end(), id) == connected->end());
    }
    bool ShouldCollideLocked(const JPH::Body& body) const override {
        return group.CanCollide(body.GetCollisionGroup()) && (!callback ||
            callback(first.object1, first.kind1, reinterpret_cast<void*>(body.GetUserData()), body.GetObjectLayer()));
    }
};
void Store(JPH::Vec3Arg value, Fvector& target) { target.set(value.GetX(), value.GetY(), value.GetZ()); }
}

JoltPhysicsCore::ContactKey JoltPhysicsCore::Key(BodyHandle first, BodyHandle second, u32 shape1, u32 shape2) {
    if (first > second) { std::swap(first, second); std::swap(shape1, shape2); }
    return {(u64(first) << 32) | second, (u64(shape1) << 32) | shape2};
}
void JoltPhysicsCore::SetShapeMaterial(PhysicsShapeHandle shape, u16 material) {
    if (shape) static_cast<JPH::Shape*>(shape)->SetUserData((u64(1) << 63) | material);
}

void JoltPhysicsCore::SetBodyCollisionOwner(BodyHandle body, void* owner) {
    if (!m_physics_system || body == INVALID_BODY_HANDLE) return;
    auto previous = m_body_owners.find(body);
    if (previous != m_body_owners.end()) {
        if (previous->second == owner) return;
        auto group = m_collision_owners.find(previous->second);
        if (--group->second.references == 0) m_collision_owners.erase(group);
        m_body_owners.erase(previous);
    }
    JPH::CollisionGroup group;
    if (owner) {
        R_ASSERT(m_next_owner_group != JPH::CollisionGroup::cInvalidGroup);
        auto [entry, inserted] = m_collision_owners.try_emplace(owner, CollisionOwner{m_next_owner_group, 0});
        if (inserted) ++m_next_owner_group;
        ++entry->second.references;
        m_body_owners.emplace(body, owner);
        group = JPH::CollisionGroup(m_owner_filter, entry->second.group, 0);
    }
    m_physics_system->GetBodyInterface().SetCollisionGroup(JPH::BodyID(body), group);
}

void JoltPhysicsCore::SetBodyContactFeedback(BodyHandle body, bool enabled) {
    if (body == INVALID_BODY_HANDLE) return;
    if (enabled) m_contact_feedback_bodies.insert(body);
    else {
        m_contact_feedback_bodies.erase(body);
        m_contact_forces.erase(body);
    }
    JPH::SetXRayContactImpulseCallback(this, m_contact_feedback_bodies.empty() ? nullptr : CollectContactImpulse);
}
const std::vector<PhysicsContactForce>& JoltPhysicsCore::GetBodyContactForces(BodyHandle body) const {
    const static std::vector<PhysicsContactForce> empty;
    const auto entry = m_contact_forces.find(body);
    return entry == m_contact_forces.end() ? empty : entry->second;
}
void JoltPhysicsCore::CollectContactImpulse(void* context, const JPH::Body& first, const JPH::SubShapeID& shape1,
    const JPH::Body& second, const JPH::SubShapeID& shape2, JPH::RVec3Arg position1,
    JPH::RVec3Arg position2, JPH::Vec3Arg impulse2, JPH::Vec3Arg angularImpulse2) {
    auto& core = *static_cast<JoltPhysicsCore*>(context);
    if (impulse2.IsNearZero() && angularImpulse2.IsNearZero()) return;
    auto collect = [&core](const JPH::Body& body, const JPH::SubShapeID& shape, JPH::RVec3Arg position,
        JPH::Vec3Arg impulse, JPH::Vec3Arg angularImpulse) {
        const auto handle = body.GetID().GetIndexAndSequenceNumber();
        if (!core.m_contact_feedback_bodies.contains(handle)) return;
        PhysicsContactForce force;
        force.geometry = Geometry(body.GetShape(), shape);
        Store(JPH::Vec3(position), force.position);
        Store(impulse / core.m_step_time, force.force);
        Store(angularImpulse / core.m_step_time, force.torque);
        const std::lock_guard guard(core.m_contact_force_mutex);
        core.m_contact_forces[handle].push_back(force);
    };
    collect(first, shape1, position1, -impulse2, -angularImpulse2);
    collect(second, shape2, position2, impulse2, angularImpulse2);
}

bool JoltPhysicsCore::MyCharacterContactListener::OnContactValidate(const JPH::CharacterVirtual* character,
    const JPH::CharacterContact& hit)
{
    if (hit.mBodyB.IsInvalid() || !m_core->m_physics_system) return true;
    NativePhysicsContact contact;
    contact.kind1 = 6;
    contact.body2 = hit.mBodyB.GetIndexAndSequenceNumber();
    contact.depth = -hit.mDistance;
    Store(JPH::Vec3(hit.mPosition), contact.position);
    Store(hit.mContactNormal, contact.normal);
    contact.relative_velocity = (character->GetLinearVelocity() - hit.mLinearVelocity).Length();
    {
        JPH::BodyLockRead lock(m_core->m_physics_system->GetBodyLockInterface(), hit.mBodyB);
        if (!lock.Succeeded()) return false;
        const auto& body = lock.GetBody();
        contact.object2 = reinterpret_cast<void*>(body.GetUserData());
        contact.kind2 = body.GetObjectLayer();
        contact.geometry2 = Geometry(body.GetShape(), hit.mSubShapeIDB);
        contact.material2 = Material(body.GetShape(), hit.mSubShapeIDB);
        contact.triangle2 = JPH::XRayTriangleIndex(body.GetShape(), hit.mSubShapeIDB);
    }
    for (const auto& [handle, candidate] : m_core->m_characters) {
        if (candidate.GetPtr() != character) continue;
        const auto callback = m_core->m_character_callbacks.find(handle);
        if (callback != m_core->m_character_callbacks.end()) contact.object1 = callback->second.user_data;
        break;
    }
    if (m_core->m_collision_filter && !m_core->m_collision_filter(contact.object1, 6, contact.object2, contact.kind2)) return false;
    const bool allow = !m_core->m_rb_contact_callback || m_core->m_rb_contact_callback(contact);
    if (contact.material2 < GMLib.CountMaterial()) {
        const auto* material = GMLib.GetMaterialByIdx(contact.material2);
        if (material->Flags.test(SGameMtl::flPassable) && !material->Flags.test(SGameMtl::flActorObstacle)) {
            ProcessContact(character, hit);
            return false;
        }
    }
    return allow;
}

bool JoltPhysicsCore::MyCharacterContactListener::OnCharacterContactValidate(
    const JPH::CharacterVirtual* character, const JPH::CharacterContact& hit)
{
    if (!hit.mCharacterB) return true;
    NativePhysicsContact contact;
    contact.kind1 = contact.kind2 = 6;
    auto characterObject = [this](const JPH::CharacterVirtual* pointer) -> void* {
        for (const auto& [handle, candidate] : m_core->m_characters) {
            if (candidate.GetPtr() != pointer) continue;
            const auto callback = m_core->m_character_callbacks.find(handle);
            return callback != m_core->m_character_callbacks.end() ? callback->second.user_data : nullptr;
        }
        return nullptr;
    };
    // Native character user data identifies its holder for query filtering;
    // engine contact callbacks require the CPHCharacter from this registry.
    contact.object1 = characterObject(character);
    contact.object2 = characterObject(hit.mCharacterB);
    contact.depth = -hit.mDistance;
    Store(JPH::Vec3(hit.mPosition), contact.position);
    Store(hit.mContactNormal, contact.normal);
    contact.relative_velocity = (character->GetLinearVelocity() - hit.mLinearVelocity).Length();
    if (m_core->m_collision_filter && !m_core->m_collision_filter(contact.object1, 6, contact.object2, 6)) return false;
    return !m_core->m_rb_contact_callback || m_core->m_rb_contact_callback(contact);
}

bool JoltPhysicsCore::CharacterHasDynamicContact(CharacterVirtualHandle handle) const {
    const auto found = m_characters.find(handle);
    if (found == m_characters.end()) return false;
    for (const auto& contact : found->second->GetActiveContacts()) {
        if (contact.mDistance > .02f || contact.mIsSensorB) continue;
        if (!contact.mBodyB.IsInvalid()) {
            JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), contact.mBodyB);
            if (lock.Succeeded() && lock.GetBody().GetObjectLayer() != Layers::NON_MOVING) return true;
        }
        else if (contact.mCharacterB) return true;
    }
    return false;
}
void JoltPhysicsCore::SetBodyContinuousCollision(BodyHandle handle, bool enabled) {
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return;
    m_physics_system->GetBodyInterface().SetMotionQuality(JPH::BodyID(handle),
        enabled ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete);
}

void JoltPhysicsCore::PrepareContacts(float delta_time) {
    m_rejected_contacts.clear();
    m_contact_friction.clear();
    if (!m_rb_contact_callback) return;
    // The game callbacks can destroy shells, change velocities and wake bodies.
    // Collect contacts first and dispatch after all native locks are released.
    std::vector<NativePhysicsContact> pending;
    static const bool profile = std::getenv("XRAY_JOLT_CONTACT_PROFILE") != nullptr;
    using Clock = std::chrono::steady_clock;
    double queryMilliseconds = 0, collectMilliseconds = 0;
    size_t hitCount = 0;
    std::unordered_set<ContactKey, ContactKeyHash> seen;
    JPH::BodyIDVector active;
    m_physics_system->GetActiveBodies(JPH::EBodyType::RigidBody, active);
    for (const auto firstID : active) {
        JPH::TransformedShape firstShape;
        JPH::Vec3 travel = JPH::Vec3::sZero();
        bool continuous = false;
        NativePhysicsContact initial;
        JPH::CollisionGroup group;
        {
            JPH::BodyLockRead firstLock(m_physics_system->GetBodyLockInterface(), firstID);
            if (!firstLock.SucceededAndIsInBroadPhase()) continue;
            const auto& first = firstLock.GetBody();
            firstShape = first.GetTransformedShape();
            initial.body1 = firstID.GetIndexAndSequenceNumber();
            initial.object1 = reinterpret_cast<void*>(first.GetUserData());
            initial.kind1 = first.GetObjectLayer();
            group = first.GetCollisionGroup();
            continuous = first.GetMotionProperties()->GetMotionQuality() == JPH::EMotionQuality::LinearCast;
            travel = first.GetLinearVelocity() * delta_time;
        }
        JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
        JPH::CollideShapeSettings settings;
        settings.mMaxSeparationDistance = m_physics_system->GetPhysicsSettings().mSpeculativeContactDistance;
        const auto connectedBodies = m_connected_bodies.find(firstID);
        const OtherBodyFilter filter(firstID, group, initial, m_collision_filter,
            connectedBodies == m_connected_bodies.end() ? nullptr : &connectedBodies->second);
        const auto queryStart = profile ? Clock::now() : Clock::time_point{};
        m_physics_system->GetNarrowPhaseQuery().CollideShape(firstShape.mShape, firstShape.GetShapeScale(),
            firstShape.GetCenterOfMassTransform(), settings, JPH::RVec3::sZero(), collector,
            m_physics_system->GetDefaultBroadPhaseLayerFilter(initial.kind1),
            m_physics_system->GetDefaultLayerFilter(initial.kind1), filter);
        if (continuous && travel.LengthSq() > 1e-8f) {
            JPH::AllHitCollisionCollector<JPH::CastShapeCollector> sweep;
            JPH::ShapeCastSettings castSettings;
            const JPH::RShapeCast cast(firstShape.mShape, firstShape.GetShapeScale(), firstShape.GetCenterOfMassTransform(), travel);
            m_physics_system->GetNarrowPhaseQuery().CastShape(cast, castSettings, JPH::RVec3::sZero(), sweep,
                m_physics_system->GetDefaultBroadPhaseLayerFilter(initial.kind1),
                m_physics_system->GetDefaultLayerFilter(initial.kind1), filter);
            sweep.Sort();
            for (const auto& hit : sweep.mHits) collector.mHits.push_back(hit);
        }
        const auto collectStart = profile ? Clock::now() : Clock::time_point{};
        if (profile) {
            queryMilliseconds += std::chrono::duration<double, std::milli>(collectStart - queryStart).count();
            hitCount += collector.mHits.size();
        }
        for (const auto& hit : collector.mHits) {
            JPH::BodyLockRead secondLock(m_physics_system->GetBodyLockInterface(), hit.mBodyID2);
            if (!secondLock.SucceededAndIsInBroadPhase()) continue;
            const auto& second = secondLock.GetBody();
            auto contact = initial;
            contact.body2 = hit.mBodyID2.GetIndexAndSequenceNumber();
            contact.object2 = reinterpret_cast<void*>(second.GetUserData());
            contact.kind2 = second.GetObjectLayer();
            if (!contact.object1 && !contact.object2) continue;
            if (m_collision_filter && !m_collision_filter(contact.object1, contact.kind1, contact.object2, contact.kind2)) continue;
            const auto connected = m_connected_bodies.find(firstID);
            if (connected != m_connected_bodies.end() && std::find(connected->second.begin(),
                connected->second.end(), hit.mBodyID2) != connected->second.end()) continue;
            contact.subshape1 = hit.mSubShapeID1.GetValue();
            contact.subshape2 = hit.mSubShapeID2.GetValue();
            const auto key = Key(contact.body1, contact.body2, contact.subshape1, contact.subshape2);
            if (!seen.insert(key).second) continue;
            contact.geometry1 = Geometry(firstShape.mShape, hit.mSubShapeID1);
            contact.geometry2 = Geometry(second.GetShape(), hit.mSubShapeID2);
            contact.material1 = Material(firstShape.mShape, hit.mSubShapeID1);
            contact.material2 = Material(second.GetShape(), hit.mSubShapeID2);
            contact.triangle1 = JPH::XRayTriangleIndex(firstShape.mShape, hit.mSubShapeID1);
            contact.triangle2 = JPH::XRayTriangleIndex(second.GetShape(), hit.mSubShapeID2);
            Store(hit.mContactPointOn2, contact.position);
            Store(-hit.mPenetrationAxis.NormalizedOr(JPH::Vec3::sAxisY()), contact.normal);
            contact.depth = hit.mPenetrationDepth;
            JPH::BodyLockRead firstLock(m_physics_system->GetBodyLockInterface(), firstID);
            if (!firstLock.SucceededAndIsInBroadPhase()) continue;
            contact.relative_velocity = (firstLock.GetBody().GetPointVelocity(JPH::RVec3(hit.mContactPointOn2)) -
                second.GetPointVelocity(JPH::RVec3(hit.mContactPointOn2))).Length();
            pending.push_back(contact);
        }
        if (profile) collectMilliseconds += std::chrono::duration<double, std::milli>(Clock::now() - collectStart).count();
    }
    const auto dispatchStart = profile ? Clock::now() : Clock::time_point{};
    for (const auto& contact : pending) {
        bool valid = true;
        for (const auto [handle, data] : {std::pair{contact.body1, contact.object1}, std::pair{contact.body2, contact.object2}}) {
            JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(handle));
            if (!lock.SucceededAndIsInBroadPhase() || lock.GetBody().GetUserData() != reinterpret_cast<u64>(data)) valid = false;
        }
        if (valid) {
            const auto key = Key(contact.body1, contact.body2, contact.subshape1, contact.subshape2);
            if (!m_rb_contact_callback(contact)) m_rejected_contacts.insert(key);
            if (contact.friction >= 0 || contact.friction_scale != 1 || contact.static_body1 || contact.static_body2) {
                R_ASSERT(_valid(contact.friction) && _valid(contact.friction_scale) && contact.friction_scale >= 0);
                const bool ordered = contact.body1 < contact.body2;
                m_contact_friction.emplace(key, ContactPolicy{contact.friction, contact.friction_scale,
                    ordered ? contact.static_body1 : contact.static_body2, ordered ? contact.static_body2 : contact.static_body1});
            }
        }
    }
    static unsigned profileStep = 0;
    if (profile && ++profileStep % 100 == 0)
        Msg("NATIVE_CONTACT_PROFILE active=%zu hits=%zu pending=%zu query_ms=%.6f collect_ms=%.6f dispatch_ms=%.6f",
            active.size(), hitCount, pending.size(), queryMilliseconds, collectMilliseconds,
            std::chrono::duration<double, std::milli>(Clock::now() - dispatchStart).count());
}
