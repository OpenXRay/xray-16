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
void Describe(NativePhysicsContact& contact, bool first, const JPH::Shape* shape, JPH::SubShapeID id) {
    (first ? contact.geometry1 : contact.geometry2) = Geometry(shape, id);
    auto& material = first ? contact.material1 : contact.material2;
    auto& triangle = first ? contact.triangle1 : contact.triangle2;
    JPH::SubShapeID remainder;
    const auto* leaf = shape->GetLeafShape(id, remainder);
    if (leaf->GetSubType() == JPH::EShapeSubType::Mesh && leaf->GetUserData() == JPH::XRayMeshMaterial::Tag) {
        const auto* mesh = static_cast<const JPH::MeshShape*>(leaf);
        triangle = mesh->GetTriangleUserData(remainder);
        const auto* metadata = static_cast<const JPH::XRayMeshMaterial*>(mesh->GetMaterial(remainder));
        material = triangle < metadata->materials.size() ? metadata->materials[triangle] : u16(-1);
    } else {
        const auto data = leaf->GetUserData();
        material = data & (u64(1) << 63) ? static_cast<u16>(data) : u16(-1);
    }
}
struct OtherBodyFilter : JPH::BodyFilter {
    JPH::BodyID self;
    JPH::CollisionGroup group;
    const NativePhysicsContact& first;
    IPhysicsCore::CollisionFilterFun callback;
    const std::vector<JPH::BodyID>* connected;
    void* context;
    bool (*prepare)(void*, JPH::BodyID, JPH::BodyID);
    bool continuous;
    OtherBodyFilter(JPH::BodyID id, const JPH::CollisionGroup& collisionGroup,
        const NativePhysicsContact& contact, IPhysicsCore::CollisionFilterFun filter,
        const std::vector<JPH::BodyID>* excluded, void* state,
        bool (*needsPreparation)(void*, JPH::BodyID, JPH::BodyID), bool sweep)
        : self(id), group(collisionGroup), first(contact), callback(filter), connected(excluded),
          context(state), prepare(needsPreparation), continuous(sweep) {}
    bool ShouldCollide(const JPH::BodyID& id) const override {
        return id != self && (!connected || std::find(connected->begin(), connected->end(), id) == connected->end());
    }
    bool ShouldCollideLocked(const JPH::Body& body) const override {
        // Query ordinary active pairs once. CCD pairs retain both sweeps.
        if (body.IsActive() && self > body.GetID() && !continuous &&
            body.GetMotionProperties()->GetMotionQuality() != JPH::EMotionQuality::LinearCast) return false;
        return prepare(context, self, body.GetID()) && group.CanCollide(body.GetCollisionGroup()) && (!callback ||
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
        Describe(contact, false, body.GetShape(), hit.mSubShapeIDB);
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
    auto& pending = m_prepared_contacts;
    pending.clear();
    m_query_bodies.clear();
    m_preparation_targets.clear();
    m_physics_system->GetBodies(m_query_bodies);
    for (const auto id : m_query_bodies) {
        if (m_contact_bodies.size() <= id.GetIndex()) m_contact_bodies.resize(size_t(id.GetIndex()) + 1);
        auto& snapshot = m_contact_bodies[id.GetIndex()];
        JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), id);
        if (!lock.SucceededAndIsInBroadPhase()) { snapshot.id = JPH::BodyID(); continue; }
        const auto& body = lock.GetBody();
        snapshot.id = id;
        snapshot.bounds = body.GetWorldSpaceBounds();
        snapshot.bounds.ExpandBy(JPH::Vec3::sReplicate(m_physics_system->GetPhysicsSettings().mSpeculativeContactDistance));
        if (!body.IsStatic() && body.GetMotionProperties()->GetMotionQuality() == JPH::EMotionQuality::LinearCast) {
            const auto travel = body.GetLinearVelocity() * delta_time;
            snapshot.bounds.Encapsulate(JPH::AABox(snapshot.bounds.mMin + travel, snapshot.bounds.mMax + travel));
        }
        snapshot.fluid_bounds = JPH::AABox();
        snapshot.fluid_mesh = false;
        snapshot.policy = m_body_contact_policy_callback ? m_body_contact_policy_callback(
            reinterpret_cast<void*>(body.GetUserData()), body.GetObjectLayer()) : NativeBodyContactPolicy{};
        if (!m_body_contact_policy_callback) continue;
        const auto* shape = body.GetShape();
        if (shape->GetSubType() == JPH::EShapeSubType::Mesh && shape->GetUserData() == JPH::XRayMeshMaterial::Tag) {
            snapshot.fluid_mesh = true;
            const auto* mesh = static_cast<const JPH::MeshShape*>(shape);
            const auto* metadata = static_cast<const JPH::XRayMeshMaterial*>(mesh->GetMaterialList().front().GetPtr());
            if (metadata->materialBounds.empty()) snapshot.fluid_bounds = snapshot.bounds;
            else for (u16 material = 0; material < metadata->materialBounds.size() && material < GMLib.CountMaterial(); ++material)
                if (GMLib.GetMaterialByIdx(material)->Flags.test(SGameMtl::flSlowDown) && metadata->materialBounds[material].IsValid())
                    snapshot.fluid_bounds.Encapsulate(metadata->materialBounds[material].Transformed(body.GetCenterOfMassTransform()));
        } else {
            // Game policies cover all compound children; plain shapes also
            // expose their material directly through the native shape tag.
            const auto material = static_cast<u16>(shape->GetUserData());
            if (snapshot.policy.slowdown_material ||
                ((shape->GetUserData() & (u64(1) << 63)) && material < GMLib.CountMaterial() &&
                 GMLib.GetMaterialByIdx(material)->Flags.test(SGameMtl::flSlowDown)))
                snapshot.fluid_bounds = snapshot.bounds;
        }
        if (snapshot.policy.immediate || snapshot.fluid_bounds.IsValid()) m_preparation_targets.push_back(id);
    }
    static const bool profile = std::getenv("XRAY_JOLT_CONTACT_PROFILE") != nullptr;
    using Clock = std::chrono::steady_clock;
    double queryMilliseconds = 0, collectMilliseconds = 0;
    size_t hitCount = 0;
    auto& active = m_active_query_bodies;
    active.clear();
    m_physics_system->GetActiveBodies(JPH::EBodyType::RigidBody, active);
    m_fluid_preparation_pairs.clear();
    // Material-wide level bounds can span several disconnected ponds. Test
    // nearby mesh leaves before running expensive shape/triangle collision.
    // Cache the decision once on the caller thread for native listener jobs.
    for (const auto targetID : m_query_bodies) {
        const auto& target = m_contact_bodies[targetID.GetIndex()];
        if (target.id != targetID || !target.fluid_bounds.IsValid()) continue;
        JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), targetID);
        if (!lock.SucceededAndIsInBroadPhase()) continue;
        const auto* shape = lock.GetBody().GetShape();
        if (shape->GetSubType() != JPH::EShapeSubType::Mesh || shape->GetUserData() != JPH::XRayMeshMaterial::Tag) continue;
        const auto* mesh = static_cast<const JPH::MeshShape*>(shape);
        const auto inverse = lock.GetBody().GetCenterOfMassTransform().InversedRotationTranslation();
        const auto* metadata = static_cast<const JPH::XRayMeshMaterial*>(mesh->GetMaterialList().front().GetPtr());
        for (const auto sourceID : active) {
            const auto& source = m_contact_bodies[sourceID.GetIndex()];
            if (!source.policy.fluids || source.policy.immediate || target.policy.immediate ||
                !source.bounds.Overlaps(target.fluid_bounds)) continue;
            const auto bounds = source.bounds.Transformed(inverse);
            if (JPH::XRayVisitMeshBox(*mesh, const_cast<JPH::XRayMeshMaterial*>(metadata), bounds.mMin, bounds.mMax,
                +[](void* state, u32 triangle) {
                    const auto* data = static_cast<const JPH::XRayMeshMaterial*>(state);
                    const auto material = data->materials[triangle];
                    return material < GMLib.CountMaterial() && GMLib.GetMaterialByIdx(material)->Flags.test(SGameMtl::flSlowDown);
                })) m_fluid_preparation_pairs.push_back(Key(sourceID.GetIndexAndSequenceNumber(),
                    targetID.GetIndexAndSequenceNumber(), 0, 0).bodies);
        }
    }
    std::sort(m_fluid_preparation_pairs.begin(), m_fluid_preparation_pairs.end());
    auto& collector = m_preparation_hits;
    auto& sweep = m_preparation_sweeps;
    for (const auto firstID : active) {
        if (m_body_contact_policy_callback) {
            const auto& source = m_contact_bodies[firstID.GetIndex()];
            bool needed = source.policy.immediate || source.fluid_bounds.IsValid();
            if (!needed) for (const auto targetID : m_preparation_targets) {
                const auto& target = m_contact_bodies[targetID.GetIndex()];
                if (source.bounds.Overlaps(target.bounds) && NeedsContactPreparation(firstID, targetID)) {
                    needed = true; break;
                }
            }
            if (!needed) continue;
        }
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
        collector.Reset();
        JPH::CollideShapeSettings settings;
        settings.mMaxSeparationDistance = m_physics_system->GetPhysicsSettings().mSpeculativeContactDistance;
        const auto connectedBodies = m_connected_bodies.find(firstID);
        const OtherBodyFilter filter(firstID, group, initial, m_collision_filter,
            connectedBodies == m_connected_bodies.end() ? nullptr : &connectedBodies->second, this,
            +[](void* state, JPH::BodyID first, JPH::BodyID second) {
                return static_cast<JoltPhysicsCore*>(state)->NeedsContactPreparation(first, second);
            }, continuous);
        const auto queryStart = profile ? Clock::now() : Clock::time_point{};
        m_physics_system->GetNarrowPhaseQuery().CollideShape(firstShape.mShape, firstShape.GetShapeScale(),
            firstShape.GetCenterOfMassTransform(), settings, JPH::RVec3::sZero(), collector,
            m_physics_system->GetDefaultBroadPhaseLayerFilter(initial.kind1),
            m_physics_system->GetDefaultLayerFilter(initial.kind1), filter);
        if (continuous && travel.LengthSq() > 1e-8f) {
            sweep.Reset();
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
            Describe(contact, true, firstShape.mShape, hit.mSubShapeID1);
            Describe(contact, false, second.GetShape(), hit.mSubShapeID2);
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
    // Flat retained storage avoids allocating hash nodes every frame. Preserve
    // the first hit for each key, including overlap/sweep duplicates.
    std::stable_sort(pending.begin(), pending.end(), [](const auto& first, const auto& second) {
        const auto a = Key(first.body1, first.body2, first.subshape1, first.subshape2);
        const auto b = Key(second.body1, second.body2, second.subshape1, second.subshape2);
        return a.bodies < b.bodies || (a.bodies == b.bodies && a.shapes < b.shapes);
    });
    pending.erase(std::unique(pending.begin(), pending.end(), [](const auto& first, const auto& second) {
        return Key(first.body1, first.body2, first.subshape1, first.subshape2) ==
            Key(second.body1, second.body2, second.subshape1, second.subshape2);
    }), pending.end());
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
        Msg("NATIVE_CONTACT_PROFILE active=%zu immediate=%zu hits=%zu pending=%zu query_ms=%.6f collect_ms=%.6f dispatch_ms=%.6f",
            active.size(), static_cast<size_t>(std::count_if(active.begin(), active.end(), [this](const auto id) { return m_contact_bodies[id.GetIndex()].policy.immediate; })),
            hitCount, pending.size(), queryMilliseconds, collectMilliseconds,
            std::chrono::duration<double, std::milli>(Clock::now() - dispatchStart).count());
    if (profile && profileStep == 100) {
        for (const auto id : m_query_bodies) {
            JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), id);
            if (!lock.SucceededAndIsInBroadPhase() || lock.GetBody().GetShape()->GetSubType() != JPH::EShapeSubType::Mesh) continue;
            const auto& bounds = m_contact_bodies[id.GetIndex()].fluid_bounds;
            Msg("NATIVE_FLUID_REGION valid=%d min=(%g,%g,%g) max=(%g,%g,%g)", bounds.IsValid(),
                bounds.mMin.GetX(), bounds.mMin.GetY(), bounds.mMin.GetZ(), bounds.mMax.GetX(), bounds.mMax.GetY(), bounds.mMax.GetZ());
        }
    }
}

bool JoltPhysicsCore::NeedsContactPreparation(JPH::BodyID first, JPH::BodyID second) const {
    if (!m_body_contact_policy_callback) return true;
    if (first.GetIndex() >= m_contact_bodies.size() || second.GetIndex() >= m_contact_bodies.size()) return true;
    const auto& a = m_contact_bodies[first.GetIndex()];
    const auto& b = m_contact_bodies[second.GetIndex()];
    if (a.id != first || b.id != second) return true;
    if (a.policy.immediate || b.policy.immediate) return true;
    auto fluid = [this, first, second](const auto& source, const auto& target) {
        if (!source.policy.fluids || !target.fluid_bounds.IsValid() || !source.bounds.Overlaps(target.fluid_bounds)) return false;
        if (!target.fluid_mesh) return true;
        return std::binary_search(m_fluid_preparation_pairs.begin(), m_fluid_preparation_pairs.end(),
            Key(first.GetIndexAndSequenceNumber(), second.GetIndexAndSequenceNumber(), 0, 0).bodies);
    };
    return fluid(a, b) || fluid(b, a);
}

void JoltPhysicsCore::QueueDeferredContact(const JPH::Body& first, const JPH::Body& second,
    JPH::SubShapeID shape1, JPH::SubShapeID shape2, JPH::RVec3Arg point, JPH::Vec3Arg normal, float depth)
{
    if (!m_deferred_rb_contact_callback || NeedsContactPreparation(first.GetID(), second.GetID())) return;
    NativePhysicsContact contact;
    contact.body1 = first.GetID().GetIndexAndSequenceNumber();
    contact.body2 = second.GetID().GetIndexAndSequenceNumber();
    contact.object1 = reinterpret_cast<void*>(first.GetUserData());
    contact.object2 = reinterpret_cast<void*>(second.GetUserData());
    contact.kind1 = first.GetObjectLayer(); contact.kind2 = second.GetObjectLayer();
    contact.subshape1 = shape1.GetValue(); contact.subshape2 = shape2.GetValue();
    Describe(contact, true, first.GetShape(), shape1);
    Describe(contact, false, second.GetShape(), shape2);
    Store(JPH::Vec3(point), contact.position); Store(normal, contact.normal);
    Store(first.GetPointVelocity(point), contact.point_velocity1);
    Store(second.GetPointVelocity(point), contact.point_velocity2);
    const auto mass = [](const JPH::Body& body) {
        return body.IsDynamic() && body.GetMotionProperties()->GetInverseMass() > 0 ?
            1.f / body.GetMotionProperties()->GetInverseMass() : 0.f;
    };
    contact.mass1 = mass(first); contact.mass2 = mass(second);
    contact.has_snapshot = true; contact.depth = depth;
    contact.relative_velocity = (first.GetPointVelocity(point) - second.GetPointVelocity(point)).Length();
    const std::lock_guard guard(m_deferred_rb_mutex);
    m_deferred_rb_contacts.push_back(contact);
}

void JoltPhysicsCore::FlushDeferredRigidBodyContacts() {
    // Worker jobs have joined. Check both IDs and user data before dereferencing
    // game objects, and release all body locks before dispatching effects.
    for (const auto& contact : m_deferred_rb_contacts) {
        bool valid = true;
        for (const auto [handle, data] : {std::pair{contact.body1, contact.object1}, std::pair{contact.body2, contact.object2}}) {
            JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(handle));
            if (!lock.SucceededAndIsInBroadPhase() || lock.GetBody().GetUserData() != reinterpret_cast<u64>(data)) valid = false;
        }
        if (valid && m_deferred_rb_contact_callback) m_deferred_rb_contact_callback(contact);
    }
    m_deferred_rb_contacts.clear();
}
