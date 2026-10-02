#include "stdafx.h"
#include "JoltPhysicsCore.h"
#include "xrMaterialSystem/GameMtlLib.h"
#include "xrPhysics/PHJointDestroyInfo.h"
#include "Jolt-proj/XRayMeshQuery.h"
#include <cstdlib>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <algorithm>
#include <chrono>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>

#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>

JoltPhysicsCore::~JoltPhysicsCore()
{
    Destroy();
}

static inline bool is_valid_pos(const Fvector& v) {
    return _valid(v) && (v.x * v.x + v.y * v.y + v.z * v.z) < 10000000000.0f;
}





// --- DEFERRED EVENT QUEUE (TLS) ---
struct SDeferredCharacterContact {
    IPhysicsCore::CharacterContactCallbackFun callback;
    void* char_user_data;
    Fvector contact_pos;
    Fvector contact_norm;
    Fvector contact_vel;
    u32 tri_user_data;
    BodyHandle other_body_handle;
    void* other_body_user_data;
    bool is_sensor;
};

struct SDeferredBodyActivation {
    BodyHandle body_handle;
    void* user_data;
    bool activated;
};

static const int JOLT_MAX_THREADS = 64;
static std::atomic<int> g_jolt_thread_counter{0};
static thread_local int g_jolt_thread_idx = -1;
static std::vector<SDeferredCharacterContact> g_deferred_contacts[JOLT_MAX_THREADS];
static std::vector<SDeferredBodyActivation> g_deferred_activations[JOLT_MAX_THREADS];
static std::mutex g_event_mutex;

template<class T> static void Enqueue(std::vector<T>& buffer, T event) {
    // A worker index can be reused after a job pool is recreated. Protect the
    // buffers even when old and new thread-local indices map to the same slot.
    const std::lock_guard lock(g_event_mutex);
    buffer.push_back(std::move(event));
}

static int GetJoltThreadIndex() {
    if (g_jolt_thread_idx == -1) {
        g_jolt_thread_idx = g_jolt_thread_counter.fetch_add(1) % JOLT_MAX_THREADS;
    }
    return g_jolt_thread_idx;
}

static void FlushDeferredContacts() {
    std::vector<SDeferredCharacterContact> pending;
    for (auto& buffer : g_deferred_contacts) {
        pending.insert(pending.end(), buffer.begin(), buffer.end());
        buffer.clear();
    }
    for (const auto& ev : pending) if (ev.callback) {
        ev.callback(ev.char_user_data, ev.contact_pos, ev.contact_norm, ev.contact_vel,
            ev.tri_user_data, ev.other_body_handle, ev.other_body_user_data, ev.is_sensor);
    }
}
static void FlushDeferredActivations(JPH::PhysicsSystem& system, IPhysicsCore::BodyActivationCallbackFun callback) {
    std::vector<SDeferredBodyActivation> pending;
    for (auto& buffer : g_deferred_activations) {
        pending.insert(pending.end(), buffer.begin(), buffer.end());
        buffer.clear();
    }
    for (const auto& ev : pending) {
        bool valid;
        {
            JPH::BodyLockRead lock(system.GetBodyLockInterface(), JPH::BodyID(ev.body_handle));
            valid = lock.SucceededAndIsInBroadPhase() &&
                lock.GetBody().GetUserData() == reinterpret_cast<u64>(ev.user_data) &&
                lock.GetBody().IsActive() == ev.activated;
        }
        if (valid && callback) callback(ev.body_handle, ev.user_data, ev.activated);
    }
}
// ----------------------------------



static void JoltTraceImpl(const char* inFMT, ...)
{
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
}

#ifdef JPH_ENABLE_ASSERTS
static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
{
    return true;
}
#endif

void JoltPhysicsCore::Initialize()
{
    if (m_physics_system) return;
    m_profile_enabled = std::getenv("XRAY_JOLT_CONTACT_PROFILE") != nullptr;

    JPH::Trace = JoltTraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = JoltAssertFailedImpl;
#endif

    if (!JPH::Factory::sInstance) {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
    }

    if (!m_temp_allocator) {
        m_temp_allocator = new JPH::TempAllocatorImpl(32 * 1024 * 1024);
    }
    if (!m_job_system) {
        const unsigned available = std::thread::hardware_concurrency();
        m_worker_count = std::min(4u, available > 1 ? available - 1 : 0u);
        if (const char* configured = std::getenv("XRAY_JOLT_WORKERS")) {
            char* end = nullptr;
            const unsigned long parsed = std::strtoul(configured, &end, 10);
            R_ASSERT2(end != configured && *end == '\0' && parsed <= 32, "XRAY_JOLT_WORKERS must be 0..32");
            m_worker_count = static_cast<u32>(parsed);
        }
        m_job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, m_worker_count);
        Msg("* Native Jolt physics workers: %u", m_worker_count);
    }

#ifdef JPH_DEBUG_RENDERER
    if (!m_debug_renderer) {
        m_debug_renderer = new CXRayJoltDebugRenderer();
    }
#endif

    m_physics_system = new JPH::PhysicsSystem();
    const uint32_t cMaxBodies = 32768;
    const uint32_t cNumBodyMutexes = 2048;
    const uint32_t cMaxBodyPairs = 65536;
    const uint32_t cMaxContactConstraints = 65536;

    m_physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
        m_broad_phase_layer_interface,
        m_object_vs_broadphase_layer_filter,
        m_object_vs_object_layer_filter);

    m_physics_system->SetContactListener(&m_contact_listener);
    m_physics_system->SetBodyActivationListener(&m_body_activation_listener);
}

void JoltPhysicsCore::Clear()
{
    if (!m_physics_system) return;
    for (const auto body : std::vector<BodyHandle>(m_contact_feedback_bodies.begin(), m_contact_feedback_bodies.end()))
        SetBodyContactFeedback(body, false);
    m_contact_forces.clear();
    int max_threads = std::min((int)g_jolt_thread_counter.load(), JOLT_MAX_THREADS);
    for (int i = 0; i < max_threads; ++i) {
        g_deferred_contacts[i].clear();
        g_deferred_activations[i].clear();
    }

    for (auto& pair : m_constraints) {
        if (pair.second) {
            m_physics_system->RemoveConstraint(pair.second);
        }
    }
    m_constraints.clear();
    m_joint_feedback.clear();
    m_joint_springs.clear();
    m_joint_motors.clear();
    m_joint_limits.clear();
    m_wheel_joints.clear();
    m_rejected_contacts.clear();
    m_contact_friction.clear();
    m_contact_bodies.clear();
    m_fluid_exclusions.clear();
    m_slowdown_materials.clear();
    m_current_slowdown_materials.clear();
    m_prepared_contacts.clear();
    m_deferred_rb_contacts.clear();
    m_feedback_frames.clear();
    m_connected_bodies.clear();
    m_ignore_static_bodies.clear();
    m_character_callbacks.clear();

    for (auto& pair : m_ragdolls) {
        if (pair.second) {
            pair.second->RemoveFromPhysicsSystem();
        }
    }
    m_ragdolls.clear();
    m_ragdoll_settings.clear();

    for (auto& pair : m_characters) {
        if (pair.second) {
            m_char_vs_char_collision.Remove(pair.second.GetPtr());
        }
    }
    m_characters.clear();
    m_inactive_characters.clear();
    m_stick_to_floor.clear();
    m_character_gravity_factors.clear();

    JPH::BodyInterface& bi = m_physics_system->GetBodyInterface();
    JPH::BodyIDVector all_bodies;
    m_physics_system->GetBodies(all_bodies);
    if (!all_bodies.empty()) {
        JPH::BodyIDVector added;
        for (auto body : all_bodies) if (bi.IsAdded(body)) added.push_back(body);
        if (!added.empty()) bi.RemoveBodies(added.data(), static_cast<int>(added.size()));
        bi.DestroyBodies(all_bodies.data(), (int)all_bodies.size());
    }

    m_next_joint_handle = 1;
    m_body_owners.clear();
    m_collision_owners.clear();
    m_next_owner_group = 0;
    m_next_character_handle = 1;
    m_next_ragdoll_handle = 1;

    m_physics_system->OptimizeBroadPhase();
}

void JoltPhysicsCore::Step(float delta_time)
{
    if (!m_physics_system || !m_temp_allocator || !m_job_system) return;

    // CPHWorld already advances fixed time steps.
    if (m_step_time != delta_time)
    {
        m_step_time = delta_time;
        const auto springs = m_joint_springs;
        for (const auto& [joint, axes] : springs)
            for (const auto& settings : axes)
                SetJointSpringDamping(joint, settings.axis, settings.erp, settings.cfm);
    }
    const int collision_steps = 1;
    m_contact_points.store(0, std::memory_order_relaxed);
    m_contact_forces.clear();
    const auto preparationStart = std::chrono::steady_clock::now();
    PrepareContacts(delta_time);
    const auto contactEnd = m_profile_enabled ? std::chrono::steady_clock::now() : preparationStart;
    if (m_pre_integration_callback) m_pre_integration_callback();
    const auto forcesEnd = m_profile_enabled ? std::chrono::steady_clock::now() : preparationStart;
    PrepareJointFeedback();
    const auto integrationStart = std::chrono::steady_clock::now();
    const auto result = m_physics_system->Update(delta_time, collision_steps, m_temp_allocator, m_job_system);
    R_ASSERT2(result == JPH::EPhysicsUpdateError::None, "Native Jolt simulation capacity exceeded");
    const auto feedbackStart = std::chrono::steady_clock::now();
    UpdateJointFeedback(delta_time);
    const auto jointsEnd = m_profile_enabled ? std::chrono::steady_clock::now() : feedbackStart;
    FlushDeferredRigidBodyContacts();
    const auto rigidEnd = m_profile_enabled ? std::chrono::steady_clock::now() : feedbackStart;
    FlushDeferredContacts();
    const auto characterEnd = m_profile_enabled ? std::chrono::steady_clock::now() : feedbackStart;
    FlushDeferredActivations(*m_physics_system, m_body_activation_callback);
    const auto finish = std::chrono::steady_clock::now();
    m_step_statistics.contact_preparation_ms = std::chrono::duration<double, std::milli>(integrationStart - preparationStart).count();
    m_step_statistics.integration_ms = std::chrono::duration<double, std::milli>(feedbackStart - integrationStart).count();
    m_step_statistics.feedback_ms = std::chrono::duration<double, std::milli>(finish - feedbackStart).count();
    if (m_profile_enabled) {
        const std::array boundaries{preparationStart, contactEnd, forcesEnd, integrationStart,
            feedbackStart, jointsEnd, rigidEnd, characterEnd, finish};
        for (size_t index = 0; index < m_profile_times.size(); ++index)
            m_profile_times[index] += std::chrono::duration<double, std::milli>(boundaries[index + 1] - boundaries[index]).count();
        if (++m_profile_steps % 100 == 0) {
            Msg("NATIVE_CORE_PROFILE contacts_ms=%.6f forces_ms=%.6f joints_prepare_ms=%.6f integration_ms=%.6f joints_feedback_ms=%.6f rigid_effects_ms=%.6f character_effects_ms=%.6f activations_ms=%.6f",
                m_profile_times[0] / 100, m_profile_times[1] / 100, m_profile_times[2] / 100, m_profile_times[3] / 100,
                m_profile_times[4] / 100, m_profile_times[5] / 100, m_profile_times[6] / 100, m_profile_times[7] / 100);
            Msg("NATIVE_WAKE_PROFILE explicit=%llu/%llu motor=%llu/%llu limit=%llu/%llu force=%llu/%llu torque=%llu/%llu linear=%llu/%llu angular=%llu/%llu",
                m_profile_wake_calls[0], m_profile_wake_sleeping[0], m_profile_wake_calls[1], m_profile_wake_sleeping[1],
                m_profile_wake_calls[2], m_profile_wake_sleeping[2], m_profile_wake_calls[3], m_profile_wake_sleeping[3],
                m_profile_wake_calls[4], m_profile_wake_sleeping[4], m_profile_wake_calls[5], m_profile_wake_sleeping[5],
                m_profile_wake_calls[6], m_profile_wake_sleeping[6]);
            m_profile_times.fill(0);
            Msg("NATIVE_CHARACTER_PROFILE calls=%llu update_ms=%.6f contacts_ms=%.6f", m_profile_character_calls,
                m_profile_character_update_ms / 100, m_profile_character_contacts_ms / 100);
            m_profile_character_calls = 0;
            m_profile_character_update_ms = m_profile_character_contacts_ms = 0;
            m_profile_wake_calls.fill(0);
            m_profile_wake_sleeping.fill(0);
            Msg("NATIVE_MOTOR_PROFILE brakes=%llu drives=%llu invalid=%llu", m_profile_motor_brakes, m_profile_motor_drives, m_profile_motor_invalid);
            m_profile_motor_brakes = m_profile_motor_drives = m_profile_motor_invalid = 0;
        }
    }
}

void JoltPhysicsCore::ProfileWake(WakeSource source, JPH::BodyID body)
{
    if (!m_profile_enabled) return;
    const auto index = static_cast<size_t>(source);
    ++m_profile_wake_calls[index];
    if (!m_physics_system->GetBodyInterface().IsActive(body)) ++m_profile_wake_sleeping[index];
}

void JoltPhysicsCore::SetRigidBodyContactCallback(RigidBodyContactCallbackFun callback)
{
    m_rb_contact_callback = callback;
}

void JoltPhysicsCore::Destroy()
{
    Clear();

    if (m_physics_system) {
        delete m_physics_system;
        m_physics_system = nullptr;
    }

#ifdef JPH_DEBUG_RENDERER
    if (m_debug_renderer) {
        delete m_debug_renderer;
        m_debug_renderer = nullptr;
    }
#endif

    if (m_job_system) {
        delete m_job_system;
        m_job_system = nullptr;
    }
    if (m_temp_allocator) {
        delete m_temp_allocator;
        m_temp_allocator = nullptr;
    }
    if (JPH::Factory::sInstance) {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }
}

#ifdef JPH_DEBUG_RENDERER
class DistanceBodyFilter : public JPH::BodyDrawFilter {
    JPH::Vec3 m_camera_pos;
    float m_max_dist_sq;
public:
    DistanceBodyFilter(JPH::Vec3 camera_pos, float max_dist)
        : m_camera_pos(camera_pos), m_max_dist_sq(max_dist * max_dist) {}

    virtual bool ShouldDraw(const JPH::Body& inBody) const override {
        return (inBody.GetCenterOfMassPosition() - m_camera_pos).LengthSq() <= m_max_dist_sq;
    }
};
#endif

void JoltPhysicsCore::DebugDraw(const Fvector& camera_pos)
{
#ifdef JPH_DEBUG_RENDERER
    if (m_physics_system && m_debug_renderer && m_debug_draw_flags > 0) {
        m_debug_renderer->SetCameraPos(JPH::Vec3(camera_pos.x, camera_pos.y, camera_pos.z));

        // Reset per-frame counters
        m_debug_renderer->m_draw_line_count = 0;
        m_debug_renderer->m_draw_tri_count = 0;

        // Extended diagnostic: count bodies actually in broadphase
        u32 total_bodies = m_physics_system->GetNumBodies();
        u32 active_bodies = m_physics_system->GetNumActiveBodies(JPH::EBodyType::RigidBody);

        DistanceBodyFilter filter(JPH::Vec3(camera_pos.x, camera_pos.y, camera_pos.z), m_debug_draw_distance);

        JPH::BodyManager::DrawSettings draw_settings;
        draw_settings.mDrawShape = (m_debug_draw_flags & 1) != 0;
        draw_settings.mDrawShapeColor = JPH::BodyManager::EShapeColor::SleepColor; // Желтый = Активен, Красный = Спит, Серый = Статика
        draw_settings.mDrawShapeWireframe = true; // Force wireframe for reliable rendering
        draw_settings.mDrawBoundingBox = (m_debug_draw_flags & 2) != 0;
        draw_settings.mDrawGetSupportFunction = false;
        draw_settings.mDrawMassAndInertia = (m_debug_draw_flags & 8) != 0;
        draw_settings.mDrawCenterOfMassTransform = (m_debug_draw_flags & 8) != 0;

        m_physics_system->DrawBodies(draw_settings, m_debug_renderer, &filter);

        if ((m_debug_draw_flags & 4) != 0) {
            m_physics_system->DrawConstraints(m_debug_renderer);
            m_physics_system->DrawConstraintLimits(m_debug_renderer);
            m_physics_system->DrawConstraintReferenceFrame(m_debug_renderer);
        }

        // Draw virtual characters
        for (const auto& [handle, character] : m_characters) {
            if ((character->GetPosition() - JPH::Vec3(camera_pos.x, camera_pos.y, camera_pos.z)).LengthSq() <= m_debug_draw_distance * m_debug_draw_distance) {
                if ((m_debug_draw_flags & 1) != 0) {
                    character->GetShape()->Draw(m_debug_renderer, character->GetCenterOfMassTransform(), JPH::Vec3::sOne(), JPH::Color::sYellow, false, true);
                }
            }
        }

        // Call NextFrame to release unused cached geometry
        m_debug_renderer->NextFrame();
    }
#endif
}


void JoltPhysicsCore::SetDebugDrawFlags(u32 flags)
{
    m_debug_draw_flags = flags;
}

void JoltPhysicsCore::SetDebugDrawDistance(float distance)
{
    m_debug_draw_distance = distance;
}

struct CDB_TRI_Mock {
    u32 verts[3];
    u16 material;
    u16 sector;
};

static inline u32 UnpackTriangleIndex(u32 user_data)
{
    return user_data;
}

static inline bool IsPassableMaterial(const SGameMtl* mtl) {
    if (!mtl) return false;
    return mtl->Flags.test(SGameMtl::flPassable) && !mtl->Flags.test(SGameMtl::flActorObstacle);
}

static inline u32 GetTriangleUserDataForSubShape(const JPH::Shape* shape, const JPH::SubShapeID& sub_shape_id)
{
    if (!shape) return u32(-1);

    JPH::SubShapeID remainder;
    const JPH::Shape* leaf = shape->GetLeafShape(sub_shape_id, remainder);
    if (leaf && leaf->GetSubType() == JPH::EShapeSubType::Mesh)
    {
        return static_cast<const JPH::MeshShape*>(leaf)->GetTriangleUserData(remainder);
    }
    return u32(-1);
}

static inline u16 GetMaterialIndexForSubShape(const JPH::Shape* shape, const JPH::SubShapeID& sub_shape_id)
{
    if (!shape) return GAMEMTL_NONE_IDX;
    u32 user_data = GetTriangleUserDataForSubShape(shape, sub_shape_id);
    if (user_data != u32(-1)) return JPH::XRayTriangleMaterial(shape, sub_shape_id);
    JPH::SubShapeID remainder;
    const auto* leaf = shape->GetLeafShape(sub_shape_id, remainder);
    const auto metadata = leaf ? leaf->GetUserData() : 0;
    return metadata & (u64(1) << 63) ? static_cast<u16>(metadata) : GAMEMTL_NONE_IDX;
}

PhysicsShapeHandle JoltPhysicsCore::BuildCDBModel(const Fvector* verts, u32 v_cnt, const void* tris_raw, u32 t_cnt, const u32* tri_indices, u32 tri_indices_cnt)
{
    if (!verts || v_cnt < 3 || !tris_raw || t_cnt == 0)
        return nullptr;

    if (!JPH::Factory::sInstance) {
        JPH::Trace = JoltTraceImpl;
#ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = JoltAssertFailedImpl;
#endif
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
    }

    const CDB_TRI_Mock* tris = static_cast<const CDB_TRI_Mock*>(tris_raw);

    u32 actual_t_cnt = tri_indices ? tri_indices_cnt : t_cnt;
    if (actual_t_cnt == 0)
        return nullptr;

    JPH::IndexedTriangleList jolt_triangles;
    jolt_triangles.reserve(actual_t_cnt);

    for (u32 i = 0; i < actual_t_cnt; ++i) {
        u32 original_index = tri_indices ? tri_indices[i] : i;

        if (original_index >= t_cnt)
            continue;

        const auto& tri = tris[original_index];
        u32 idx0 = tri.verts[0];
        u32 idx1 = tri.verts[1];
        u32 idx2 = tri.verts[2];

        if (idx0 >= v_cnt || idx1 >= v_cnt || idx2 >= v_cnt)
            continue;

        if (idx0 == idx1 || idx1 == idx2 || idx0 == idx2)
            continue;

        const Fvector& v0 = verts[idx0];
        const Fvector& v1 = verts[idx1];
        const Fvector& v2 = verts[idx2];

        if (!_valid(v0) || !_valid(v1) || !_valid(v2))
            continue;

        float e1x = v1.x - v0.x;
        float e1y = v1.y - v0.y;
        float e1z = v1.z - v0.z;

        float e2x = v2.x - v0.x;
        float e2y = v2.y - v0.y;
        float e2z = v2.z - v0.z;

        float cx = e1y * e2z - e1z * e2y;
        float cy = e1z * e2x - e1x * e2z;
        float cz = e1x * e2y - e1y * e2x;

        float cross_sq_mag = cx * cx + cy * cy + cz * cz;
        if (cross_sq_mag < 1e-12f)
            continue;

        u16 mtl = tri.material & 0x3FFF;

        jolt_triangles.push_back(JPH::IndexedTriangle(
            idx0,
            idx1,
            idx2,
            0,
            original_index
        ));
    }

    if (jolt_triangles.empty())
        return nullptr;

    JPH::VertexList jolt_vertices;
    jolt_vertices.reserve(v_cnt);
    for (u32 i = 0; i < v_cnt; ++i) {
        jolt_vertices.push_back(JPH::Float3(verts[i].x, verts[i].y, verts[i].z));
    }

    JPH::MeshShapeSettings settings(std::move(jolt_vertices), std::move(jolt_triangles));
    JPH::Ref<JPH::XRayMeshMaterial> metadata = new JPH::XRayMeshMaterial;
    metadata->materials.reserve(t_cnt);
    for (u32 index = 0; index < t_cnt; ++index) metadata->materials.push_back(tris[index].material & 0x3fff);
    settings.mMaterials.emplace_back(metadata.GetPtr());
    settings.mUserData = JPH::XRayMeshMaterial::Tag;
    settings.mPerTriangleUserData = true;

    JPH::ShapeSettings::ShapeResult result = settings.Create();
    if (result.HasError()) {
        Msg("! [JoltPhysicsCore] BuildCDBModel error: %s", result.GetError().c_str());
        return nullptr;
    }

    JPH::Shape* shape = result.Get().GetPtr();
    shape->AddRef();
    return static_cast<PhysicsShapeHandle>(shape);
}

void JoltPhysicsCore::DestroyCDBModel(PhysicsShapeHandle handle)
{
    if (handle) {
        JPH::Shape* shape = static_cast<JPH::Shape*>(handle);
        shape->Release();
    }
}

long JoltPhysicsCore::GetShapeMemoryUsage(PhysicsShapeHandle handle)
{
    if(handle) {
        JPH::Shape* shape = static_cast<JPH::Shape*>(handle);
        return static_cast<long>(shape->GetStats().mSizeBytes);
    } else {
        return 0;
    }
}

void JoltPhysicsCore::RaycastCDBModel(PhysicsShapeHandle handle,
                                      const Fvector& start, const Fvector& dir, float range,
                                      CDBRayMode mode, bool cull_backfaces,
                                      std::vector<CDBRaycastHit>& out_hits)
{
    if (!handle) return;

    JPH::MeshShape* mesh_shape = static_cast<JPH::MeshShape*>(handle);

    JPH::Vec3 j_start(start.x, start.y, start.z);
    JPH::Vec3 j_dir(dir.x * range, dir.y * range, dir.z * range);
    JPH::RayCast ray(j_start, j_dir);

    JPH::RayCastSettings settings;
    settings.mBackFaceModeTriangles = cull_backfaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mTreatConvexAsSolid = false;

    JPH::SubShapeIDCreator id_creator;

    if (mode == CDBRayMode::Nearest)
    {
        JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;
        mesh_shape->CastRay(ray, settings, id_creator, collector);
        if (collector.HadHit()) {
            out_hits.push_back({
                collector.mHit.mFraction * range,
                UnpackTriangleIndex(mesh_shape->GetTriangleUserData(collector.mHit.mSubShapeID2))
            });
        }
    }
    else if (mode == CDBRayMode::First)
    {
        JPH::AnyHitCollisionCollector<JPH::CastRayCollector> collector;
        mesh_shape->CastRay(ray, settings, id_creator, collector);
        if (collector.HadHit()) {
            out_hits.push_back({
                collector.mHit.mFraction * range,
                UnpackTriangleIndex(mesh_shape->GetTriangleUserData(collector.mHit.mSubShapeID2))
            });
        }
    }
    else
    {
        JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
        mesh_shape->CastRay(ray, settings, id_creator, collector);
        for (const JPH::RayCastResult& hit : collector.mHits) {
            out_hits.push_back({
                hit.mFraction * range,
                UnpackTriangleIndex(mesh_shape->GetTriangleUserData(hit.mSubShapeID2))
            });
        }
        std::sort(out_hits.begin(), out_hits.end(), [](const CDBRaycastHit& a, const CDBRaycastHit& b) {
            return a.tri_index < b.tri_index;
        });
        out_hits.erase(std::unique(out_hits.begin(), out_hits.end(), [](const CDBRaycastHit& a, const CDBRaycastHit& b) {
            return a.tri_index == b.tri_index;
        }), out_hits.end());
    }
}

void JoltPhysicsCore::RaycastCDBModel(PhysicsShapeHandle handle, const Fvector& start, const Fvector& dir, float range, std::vector<CDBRaycastHit>& out_hits)
{
    if (!handle) return;

    JPH::MeshShape* mesh_shape = static_cast<JPH::MeshShape*>(handle);

    JPH::Vec3 j_start(start.x, start.y, start.z);
    JPH::Vec3 j_dir(dir.x * range, dir.y * range, dir.z * range);
    JPH::RayCast ray(j_start, j_dir);

    JPH::RayCastSettings settings;
    settings.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces;
    settings.mTreatConvexAsSolid = false;

    JPH::SubShapeIDCreator id_creator;
    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;

    mesh_shape->CastRay(ray, settings, id_creator, collector);

    for (const JPH::RayCastResult& hit : collector.mHits) {
        CDBRaycastHit cdb_hit;
        cdb_hit.range = hit.mFraction * range;
        cdb_hit.tri_index = UnpackTriangleIndex(mesh_shape->GetTriangleUserData(hit.mSubShapeID2));
        out_hits.push_back(cdb_hit);
    }
}

PhysicsShapeHandle JoltPhysicsCore::CreateCompoundShape(PhysicsShapeHandle* shapes, const Fmatrix* transforms, size_t count)
{
    if (count == 0 || !shapes || !transforms) return nullptr;

    JPH::StaticCompoundShapeSettings compound_settings;

    for (size_t i = 0; i < count; ++i)
    {
        JPH::Shape* jph_shape = static_cast<JPH::Shape*>(shapes[i]);
        if (!jph_shape) continue;

        const Fvector& pos = transforms[i].c;
        if (!_valid(pos)) continue;
        JPH::Vec3 position(pos.x, pos.y, pos.z);

        const Fmatrix& m = transforms[i];
        if (!_valid(m)) continue;

        JPH::Vec3 axis_x(m.i.x, m.i.y, m.i.z);
        JPH::Vec3 axis_y(m.j.x, m.j.y, m.j.z);
        JPH::Vec3 axis_z(m.k.x, m.k.y, m.k.z);

        if (axis_x.LengthSq() > 1e-6f) axis_x = axis_x.Normalized();
        else axis_x = JPH::Vec3::sAxisX();

        if (axis_y.LengthSq() > 1e-6f) axis_y = axis_y.Normalized();
        else axis_y = axis_x.GetNormalizedPerpendicular();

        axis_z = axis_x.Cross(axis_y);
        if (axis_z.LengthSq() > 1e-6f) axis_z = axis_z.Normalized();
        else axis_z = JPH::Vec3::sAxisZ();

        axis_y = axis_z.Cross(axis_x).Normalized();

        JPH::Mat44 rot_mat(
            JPH::Vec4(axis_x, 0.0f),
            JPH::Vec4(axis_y, 0.0f),
            JPH::Vec4(axis_z, 0.0f),
            JPH::Vec4(0, 0, 0, 1)
        );

        JPH::Quat rotation = rot_mat.GetQuaternion().Normalized();
        // Добавляем форму с ее локальным смещением
        compound_settings.AddShape(position, rotation, jph_shape, static_cast<u32>(i));
    }

    JPH::ShapeSettings::ShapeResult result = compound_settings.Create();
    if (result.HasError())
    {
        return CreateBoxShape({0.5f, 0.5f, 0.5f});
    }

    // Child offsets are relative to X-Ray's prescribed mass center.
    auto* final_shape = new JPH::OffsetCenterOfMassShape(result.Get().GetPtr(), -result.Get()->GetCenterOfMass());
    final_shape->AddRef();

    return static_cast<PhysicsShapeHandle>(final_shape);
}

BodyHandle JoltPhysicsCore::CreateBodyFromShape(PhysicsShapeHandle shape_handle, const Fvector& pos, float mass)
{
    if (!m_physics_system || !shape_handle) return INVALID_BODY_HANDLE;
    R_ASSERT2(is_valid_pos(pos), "CreateBodyFromShape: Invalid body position!");

    JPH::Shape* shape = static_cast<JPH::Shape*>(shape_handle);

    JPH::BodyCreationSettings body_settings(
        shape,
        JPH::RVec3(pos.x, pos.y, pos.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Layers::MOVING
    );

    body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    body_settings.mMassPropertiesOverride.mMass = mass;
    body_settings.mFriction = 0.7f;
    body_settings.mRestitution = 0.1f;
    body_settings.mLinearDamping = 0.05f;
    body_settings.mAngularDamping = 0.05f;

    JPH::Body* body = m_physics_system->GetBodyInterface().CreateBody(body_settings);
    if (!body) {
        return INVALID_BODY_HANDLE;
    }

    m_physics_system->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);

    return static_cast<BodyHandle>(body->GetID().GetIndexAndSequenceNumber());
}

BodyHandle JoltPhysicsCore::CreateBox(const Fvector& half_extents, const Fvector& position, float mass) {
    if (!m_physics_system) return INVALID_BODY_HANDLE;
    R_ASSERT2(is_valid_pos(position), "CreateBox: Invalid body position!");

    JPH::BoxShapeSettings shape_settings(JPH::Vec3(half_extents.x, half_extents.y, half_extents.z));
    JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
    if (shape_result.HasError()) return INVALID_BODY_HANDLE;

    JPH::BodyCreationSettings body_settings(
        shape_result.Get(),
        JPH::Vec3(position.x, position.y, position.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Layers::MOVING
    );

    body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    body_settings.mMassPropertiesOverride.mMass = mass;
    body_settings.mFriction = 0.7f;
    body_settings.mRestitution = 0.1f;
    body_settings.mLinearDamping = 0.05f;
    body_settings.mAngularDamping = 0.05f;

    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
    JPH::Body* body = body_interface.CreateBody(body_settings);

    if (!body) return INVALID_BODY_HANDLE;

    body_interface.AddBody(body->GetID(), JPH::EActivation::Activate);

    return body->GetID().GetIndexAndSequenceNumber();
}

BodyHandle JoltPhysicsCore::CreateSphere(float radius, const Fvector& position, float mass) {
    if (!m_physics_system) return INVALID_BODY_HANDLE;
    R_ASSERT2(is_valid_pos(position), "CreateSphere: Invalid body position!");

    JPH::SphereShapeSettings shape_settings(radius);
    JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();

    JPH::BodyCreationSettings body_settings(
        shape_result.Get(),
        JPH::Vec3(position.x, position.y, position.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Layers::MOVING
    );

    body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    body_settings.mMassPropertiesOverride.mMass = mass;
    body_settings.mFriction = 0.7f;
    body_settings.mRestitution = 0.1f;
    body_settings.mLinearDamping = 0.05f;
    body_settings.mAngularDamping = 0.05f;

    JPH::Body* body = m_physics_system->GetBodyInterface().CreateBody(body_settings);
    m_physics_system->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);

    return body->GetID().GetIndexAndSequenceNumber();
}

BodyHandle JoltPhysicsCore::CreateCylinder(float radius, float half_height, const Fvector& position, float mass) {
    if (!m_physics_system) return INVALID_BODY_HANDLE;
    R_ASSERT2(is_valid_pos(position), "CreateCylinder: Invalid body position!");

    JPH::CylinderShapeSettings shape_settings(half_height, radius);
    JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();

    JPH::BodyCreationSettings body_settings(
        shape_result.Get(),
        JPH::Vec3(position.x, position.y, position.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Dynamic,
        Layers::MOVING
    );

    body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    body_settings.mMassPropertiesOverride.mMass = mass;
    body_settings.mFriction = 0.7f;
    body_settings.mRestitution = 0.1f;
    body_settings.mLinearDamping = 0.05f;
    body_settings.mAngularDamping = 0.05f;

    JPH::Body* body = m_physics_system->GetBodyInterface().CreateBody(body_settings);
    m_physics_system->GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);

    return body->GetID().GetIndexAndSequenceNumber();
}

void JoltPhysicsCore::GetBoxExtents(BodyHandle body_handle, Fvector& out_extents) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    const JPH::Shape* shape = m_physics_system->GetBodyInterface().GetShape(id).GetPtr();
    if (shape->GetSubType() == JPH::EShapeSubType::Box) {
        const JPH::BoxShape* box = static_cast<const JPH::BoxShape*>(shape);
        JPH::Vec3 extents = box->GetHalfExtent();
        out_extents.set(extents.GetX(), extents.GetY(), extents.GetZ());
    }
}

void JoltPhysicsCore::SetBoxExtents(BodyHandle body_handle, const Fvector& extents) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    JPH::BoxShapeSettings shape_settings(JPH::Vec3(extents.x, extents.y, extents.z));
    JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();

    if (shape_result.IsValid()) {
        m_physics_system->GetBodyInterface().SetShape(id, shape_result.Get(), false, JPH::EActivation::Activate);
    }
}

void JoltPhysicsCore::DestroyBody(BodyHandle body_handle) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    m_fluid_exclusions.clear();

    const JPH::BodyID retiring(body_handle);
    SetBodyContactFeedback(body_handle, false);
    SetBodyCollisionOwner(body_handle, nullptr);
    std::vector<JointHandle> attached;
    for (const auto& [handle, constraint] : m_constraints) {
        const auto* pair = static_cast<const JPH::TwoBodyConstraint*>(constraint.GetPtr());
        if (pair->GetBody1()->GetID() == retiring || pair->GetBody2()->GetID() == retiring)
            attached.push_back(handle);
    }
    for (const auto handle : attached) DestroyJoint(handle);
    m_ignore_static_bodies.erase(retiring);
    m_connected_bodies.erase(retiring);

    // Clean up any pending activation events for this body to prevent stale callbacks
    int max_threads = std::min((int)g_jolt_thread_counter.load(), JOLT_MAX_THREADS);
    for (int i = 0; i < max_threads; ++i) {
        auto& vec = g_deferred_activations[i];
        vec.erase(std::remove_if(vec.begin(), vec.end(), [body_handle](const SDeferredBodyActivation& ev) {
            return ev.body_handle == body_handle;
        }), vec.end());
    }

    JPH::BodyID id(body_handle);
    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();

    if (body_interface.IsAdded(id)) body_interface.RemoveBody(id);
    body_interface.DestroyBody(id);
}

void JoltPhysicsCore::SetBodyInWorld(BodyHandle handle, bool enabled) {
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return;
    auto& bodies = m_physics_system->GetBodyInterface();
    const JPH::BodyID id(handle);
    if (enabled && !bodies.IsAdded(id)) bodies.AddBody(id, JPH::EActivation::Activate);
    if (!enabled && bodies.IsAdded(id)) bodies.RemoveBody(id);
}

static void StoreBodyTransform(const JPH::Mat44& transform, Fmatrix& out_matrix) {
    JPH::Vec3 pos = transform.GetTranslation();

    if (_isnan(pos.GetX()) || _isnan(pos.GetY()) || _isnan(pos.GetZ())) {
        out_matrix.i.set(1.0f, 0.0f, 0.0f);
        out_matrix.j.set(0.0f, 1.0f, 0.0f);
        out_matrix.k.set(0.0f, 0.0f, 1.0f);
        out_matrix.c.set(0.0f, 0.0f, 0.0f);
        out_matrix._14_ = 0.0f; out_matrix._24_ = 0.0f; out_matrix._34_ = 0.0f; out_matrix._44_ = 1.0f;
        return;
    }

    JPH::Vec3 axis_x = transform.GetAxisX();
    JPH::Vec3 axis_y = transform.GetAxisY();
    JPH::Vec3 axis_z = transform.GetAxisZ();

    out_matrix.i.set(axis_x.GetX(), axis_x.GetY(), axis_x.GetZ());
    out_matrix.j.set(axis_y.GetX(), axis_y.GetY(), axis_y.GetZ());
    out_matrix.k.set(axis_z.GetX(), axis_z.GetY(), axis_z.GetZ());
    out_matrix.c.set(pos.GetX(), pos.GetY(), pos.GetZ());
    out_matrix._14_ = 0.0f; out_matrix._24_ = 0.0f; out_matrix._34_ = 0.0f; out_matrix._44_ = 1.0f;
}

void JoltPhysicsCore::GetBodyTransform(BodyHandle body_handle, Fmatrix& out_matrix) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) {
        out_matrix = Fidentity;
        return;
    }
    StoreBodyTransform(m_physics_system->GetBodyInterface().GetCenterOfMassTransform(JPH::BodyID(body_handle)), out_matrix);
}

bool JoltPhysicsCore::ReadBodyState(BodyHandle handle, NativeBodyState& state) const {
    state = NativeBodyState{};
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return false;
    JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(handle));
    if (!lock.Succeeded()) return false;
    const auto& body = lock.GetBody();
    StoreBodyTransform(body.GetCenterOfMassTransform(), state.transform);
    state.active = body.IsActive();
    if (!body.IsStatic()) {
        const auto linear = body.GetLinearVelocity(), angular = body.GetAngularVelocity();
        state.linear_velocity.set(linear.GetX(), linear.GetY(), linear.GetZ());
        state.angular_velocity.set(angular.GetX(), angular.GetY(), angular.GetZ());
    }
    const auto* motion = body.GetMotionPropertiesUnchecked();
    if (motion && motion->GetInverseMass() > 0) state.mass = 1.f / motion->GetInverseMass();
    if (body.IsDynamic()) {
        const auto force = body.GetAccumulatedForce(), torque = body.GetAccumulatedTorque();
        state.force.set(force.GetX(), force.GetY(), force.GetZ());
        state.torque.set(torque.GetX(), torque.GetY(), torque.GetZ());
    }
    return true;
}

void JoltPhysicsCore::SetBodyTransform(BodyHandle body_handle, const Fmatrix& matrix) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    R_ASSERT2(is_valid_pos(matrix.c), "SetBodyTransform: Invalid matrix position!");
    if (!_valid(matrix)) return;

    JPH::BodyID id(body_handle);
    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();

    JPH::Vec3 com_world(matrix.c.x, matrix.c.y, matrix.c.z);

    JPH::Vec3 axis_x(matrix.i.x, matrix.i.y, matrix.i.z);
    JPH::Vec3 axis_y(matrix.j.x, matrix.j.y, matrix.j.z);
    JPH::Vec3 axis_z(matrix.k.x, matrix.k.y, matrix.k.z);

    if (axis_x.LengthSq() > 1e-6f) axis_x = axis_x.Normalized();
    else axis_x = JPH::Vec3::sAxisX();

    if (axis_y.LengthSq() > 1e-6f) axis_y = axis_y.Normalized();
    else axis_y = axis_x.GetNormalizedPerpendicular();

    axis_z = axis_x.Cross(axis_y);
    if (axis_z.LengthSq() > 1e-6f) axis_z = axis_z.Normalized();
    else axis_z = JPH::Vec3::sAxisZ();

    axis_y = axis_z.Cross(axis_x).Normalized();

    JPH::Mat44 transform(
        JPH::Vec4(axis_x, 0.0f),
        JPH::Vec4(axis_y, 0.0f),
        JPH::Vec4(axis_z, 0.0f),
        JPH::Vec4(0, 0, 0, 1)
    );
    JPH::Quat rotation = transform.GetQuaternion().Normalized();

    JPH::Vec3 com_local = body_interface.GetShape(id)->GetCenterOfMass();
    JPH::Vec3 shape_origin = com_world - rotation * com_local;

    body_interface.SetPositionAndRotationWhenChanged(id, shape_origin, rotation,
        body_interface.IsAdded(id) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
}

void JoltPhysicsCore::GetBodyAABB(BodyHandle body_handle, Fvector& center, Fvector& half_extents) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) {
        center.set(0.f, 0.f, 0.f);
        half_extents.set(0.1f, 0.1f, 0.1f);
        return;
    }

    JPH::BodyID id(body_handle);
    JPH::AABox bounds = m_physics_system->GetBodyInterface().GetTransformedShape(id).GetWorldSpaceBounds();

    JPH::Vec3 j_center = bounds.GetCenter();
    JPH::Vec3 j_extents = bounds.GetExtent();

    if (_isnan(j_center.GetX()) || _isnan(j_extents.GetX())) {
        center.set(0.f, 0.f, 0.f);
        half_extents.set(0.1f, 0.1f, 0.1f);
        return;
    }

    center.set(j_center.GetX(), j_center.GetY(), j_center.GetZ());
    half_extents.set(j_extents.GetX(), j_extents.GetY(), j_extents.GetZ());
}

bool JoltPhysicsCore::BoxQueryCDB(PhysicsShapeHandle handle,
                                  const Fvector& box_center,
                                  const Fvector& box_z_axis,
                                  const Fvector& box_y_axis,
                                  const Fvector& box_sizes,
                                  std::vector<u32>& out_tri_indices)
{
    if (!handle) return false;

    JPH::MeshShape* mesh_shape = static_cast<JPH::MeshShape*>(handle);

    JPH::Vec3 j_z(box_z_axis.x, box_z_axis.y, box_z_axis.z);
    JPH::Vec3 j_y(box_y_axis.x, box_y_axis.y, box_y_axis.z);

    j_z = j_z.Normalized();
    j_y = j_y.Normalized();
    JPH::Vec3 j_x = j_y.Cross(j_z).Normalized();

    JPH::BoxShape box(JPH::Vec3(box_sizes.x * 0.5f, box_sizes.y * 0.5f, box_sizes.z * 0.5f));

    JPH::Mat44 rot(
        JPH::Vec4(j_x.GetX(), j_x.GetY(), j_x.GetZ(), 0.0f),
        JPH::Vec4(j_y.GetX(), j_y.GetY(), j_y.GetZ(), 0.0f),
        JPH::Vec4(j_z.GetX(), j_z.GetY(), j_z.GetZ(), 0.0f),
        JPH::Vec4(box_center.x, box_center.y, box_center.z, 1.0f)
    );

    class JoltOBBCollector : public JPH::CollideShapeCollector {
    public:
        std::vector<u32>& indices;
        JPH::MeshShape* m_mesh_shape;
        bool has_hit = false;

        JoltOBBCollector(std::vector<u32>& out_ind, JPH::MeshShape* shape)
            : indices(out_ind), m_mesh_shape(shape) {}

        virtual void AddHit(const JPH::CollideShapeResult &inResult) override {
            has_hit = true;
            indices.push_back(UnpackTriangleIndex(m_mesh_shape->GetTriangleUserData(inResult.mSubShapeID2)));
        }
    };

    JoltOBBCollector collector(out_tri_indices, mesh_shape);
    JPH::CollideShapeSettings settings;
    settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::CollisionDispatch::sCollideShapeVsShape(
        &box, mesh_shape,
        JPH::Vec3::sReplicate(1.0f), JPH::Vec3::sReplicate(1.0f),
        rot, JPH::Mat44::sIdentity(),
        JPH::SubShapeIDCreator(), JPH::SubShapeIDCreator(),
        settings, collector, JPH::ShapeFilter()
    );

    return collector.has_hit;
}

void JoltPhysicsCore::BoxQueryCDB(PhysicsShapeHandle handle,
                                  const Fvector& center, const Fvector& extents,
                                  CDBRayMode mode, bool cull_backfaces,
                                  std::vector<u32>& out_tri_indices)
{
    if (!handle) return;

    JPH::MeshShape* mesh_shape = static_cast<JPH::MeshShape*>(handle);
    JPH::BoxShape box(JPH::Vec3(extents.x, extents.y, extents.z));

    JPH::CollideShapeSettings settings;
    settings.mBackFaceMode = cull_backfaces ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::Mat44 boxTransform = JPH::Mat44::sTranslation(JPH::Vec3(center.x, center.y, center.z));
    JPH::Mat44 meshTransform = JPH::Mat44::sIdentity();

    JPH::SubShapeIDCreator id_creator1, id_creator2;

    if (mode == CDBRayMode::First)
    {
        JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;
        JPH::CollisionDispatch::sCollideShapeVsShape(
            &box, mesh_shape,
            JPH::Vec3::sReplicate(1.0f), JPH::Vec3::sReplicate(1.0f),
            boxTransform, meshTransform,
            id_creator1, id_creator2,
            settings, collector, JPH::ShapeFilter()
        );
        if (collector.HadHit()) {
            out_tri_indices.push_back(UnpackTriangleIndex(mesh_shape->GetTriangleUserData(collector.mHit.mSubShapeID2)));
        }
    }
    else
    {
        JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
        JPH::CollisionDispatch::sCollideShapeVsShape(
            &box, mesh_shape,
            JPH::Vec3::sReplicate(1.0f), JPH::Vec3::sReplicate(1.0f),
            boxTransform, meshTransform,
            id_creator1, id_creator2,
            settings, collector, JPH::ShapeFilter()
        );
        for (const JPH::CollideShapeResult& hit : collector.mHits) {
            out_tri_indices.push_back(UnpackTriangleIndex(mesh_shape->GetTriangleUserData(hit.mSubShapeID2)));
        }
        std::sort(out_tri_indices.begin(), out_tri_indices.end());
        out_tri_indices.erase(std::unique(out_tri_indices.begin(), out_tri_indices.end()), out_tri_indices.end());
    }
}

void JoltPhysicsCore::GetCDBModelBounds(PhysicsShapeHandle handle, Fvector& out_center, Fvector& out_extents) const
{
    if (!handle)
    {
        out_center.set(0.f, 0.f, 0.f);
        out_extents.set(0.f, 0.f, 0.f);
        return;
    }

    const JPH::Shape* shape = static_cast<const JPH::Shape*>(handle);
    JPH::AABox bounds = shape->GetLocalBounds();

    out_center.set(bounds.GetCenter().GetX(), bounds.GetCenter().GetY(), bounds.GetCenter().GetZ());
    out_extents.set(bounds.GetExtent().GetX(), bounds.GetExtent().GetY(), bounds.GetExtent().GetZ());
}

void JoltPhysicsCore::SetBodyFixedRotation(BodyHandle body_handle) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    JPH::BodyLockWrite lock(m_physics_system->GetBodyLockInterface(), id);
    if (lock.Succeeded()) {
        JPH::Body& body = lock.GetBody();
        if (body.IsDynamic()) {
            body.GetMotionProperties()->SetInverseInertia(JPH::Vec3::sZero(), JPH::Quat::sIdentity());
        }
    }
}

void JoltPhysicsCore::SetBodyMotionType(BodyHandle body_handle, int motion_type) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    JPH::EMotionType j_motion = JPH::EMotionType::Dynamic;
    JPH::ObjectLayer j_layer = Layers::MOVING;

    bool ignore_static = m_ignore_static_bodies.count(id) != 0;

    if (motion_type == 0) {
        j_motion = JPH::EMotionType::Static;
        j_layer = Layers::NON_MOVING;
    } else if (motion_type == 1) {
        j_motion = JPH::EMotionType::Kinematic;
        j_layer = ignore_static ? Layers::MOVING_NO_STATIC : Layers::MOVING;
    } else {
        j_motion = JPH::EMotionType::Dynamic;
        j_layer = ignore_static ? Layers::MOVING_NO_STATIC : Layers::MOVING;
    }

    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
    body_interface.SetMotionType(id, j_motion,
        body_interface.IsAdded(id) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    body_interface.SetObjectLayer(id, j_layer);
}

BodyHandle JoltPhysicsCore::CreateStaticBody(PhysicsShapeHandle shape_handle, const Fvector& position)
{
    if (!m_physics_system || !shape_handle) return INVALID_BODY_HANDLE;
    R_ASSERT2(is_valid_pos(position), "CreateStaticBody: Invalid static body position!");

    JPH::Shape* shape = static_cast<JPH::Shape*>(shape_handle);

    JPH::BodyCreationSettings body_settings(
        shape,
        JPH::Vec3(position.x, position.y, position.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Layers::NON_MOVING
    );

    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();
    JPH::Body* body = body_interface.CreateBody(body_settings);

    if (!body) return INVALID_BODY_HANDLE;

    body_interface.AddBody(body->GetID(), JPH::EActivation::DontActivate);

    return body->GetID().GetIndexAndSequenceNumber();
}

// ============================================================================
// JOINT IMPLEMENTATIONS (NEW UNIVERSAL)
// ============================================================================

JointHandle JoltPhysicsCore::CreateJoint(int type, BodyHandle body1, BodyHandle body2, const Fvector& anchor, const Fvector& axis0, const Fvector& axis1, const Fvector& axis2, const Fvector& limits_lo, const Fvector& limits_hi)
{
    if (!m_physics_system) return INVALID_JOINT_HANDLE;

    JPH::BodyID ids[2];
    int count = 0;
    int idx1 = -1, idx2 = -1;
    if (body1 != INVALID_BODY_HANDLE) {
        idx1 = count;
        ids[count++] = JPH::BodyID(body1);
    }
    if (body2 != INVALID_BODY_HANDLE) {
        idx2 = count;
        ids[count++] = JPH::BodyID(body2);
    }

    JPH::BodyLockMultiRead lock(m_physics_system->GetBodyLockInterface(), ids, count);

    const JPH::Body* b1 = (body1 == INVALID_BODY_HANDLE) ? &JPH::Body::sFixedToWorld : (idx1 >= 0 ? lock.GetBody(idx1) : nullptr);
    const JPH::Body* b2 = (body2 == INVALID_BODY_HANDLE) ? &JPH::Body::sFixedToWorld : (idx2 >= 0 ? lock.GetBody(idx2) : nullptr);

    if (!b1 || !b2) return INVALID_JOINT_HANDLE;

    JPH::Vec3 j_anchor(anchor.x, anchor.y, anchor.z);
    JPH::Constraint* constraint = nullptr;

    switch (type) {
        case 0: { // ball
            JPH::PointConstraintSettings settings;
            settings.mSpace = JPH::EConstraintSpace::WorldSpace;
            settings.mPoint1 = settings.mPoint2 = j_anchor;
            constraint = settings.Create(*const_cast<JPH::Body*>(b1), *const_cast<JPH::Body*>(b2));
            break;
        }
        case 1: { // hinge
            JPH::HingeConstraintSettings settings;
            settings.mSpace = JPH::EConstraintSpace::WorldSpace;
            settings.mPoint1 = settings.mPoint2 = j_anchor;

            JPH::Vec3 j_axis = JPH::Vec3(axis0.x, axis0.y, axis0.z).Normalized();
            settings.mHingeAxis1 = settings.mHingeAxis2 = j_axis;

            JPH::Vec3 ref_normal1(axis1.x, axis1.y, axis1.z);
            if (ref_normal1.LengthSq() < 0.001f) {
                ref_normal1 = j_axis.GetNormalizedPerpendicular();
            } else {
                ref_normal1 = ref_normal1.Normalized();
            }

            settings.mNormalAxis1 = ref_normal1;
            settings.mNormalAxis2 = ref_normal1;

            float src_min = limits_lo.x;
            float src_max = limits_hi.x;
            if (src_min > src_max) std::swap(src_min, src_max);

            settings.mLimitsMin = std::clamp(src_min, -JPH::JPH_PI, JPH::JPH_PI);
            settings.mLimitsMax = std::clamp(src_max, -JPH::JPH_PI, JPH::JPH_PI);

            settings.mLimitsSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
            settings.mLimitsSpringSettings.mFrequency = 5.0f; // 5 Hz soft limit spring (allows pushing past limit on force)
            settings.mLimitsSpringSettings.mDamping = 0.7f;   // slightly underdamped for natural spring feel

            constraint = settings.Create(*const_cast<JPH::Body*>(b1), *const_cast<JPH::Body*>(b2));
            break;
        }
        case 4: { // slider
            JPH::SliderConstraintSettings settings;
            settings.mSpace = JPH::EConstraintSpace::WorldSpace;
            settings.mPoint1 = settings.mPoint2 = j_anchor;

            JPH::Vec3 j_axis(axis0.x, axis0.y, axis0.z);
            settings.mSliderAxis1 = settings.mSliderAxis2 = j_axis.Normalized();
            settings.mNormalAxis1 = settings.mNormalAxis2 = settings.mSliderAxis1.GetNormalizedPerpendicular();

            settings.mLimitsMin = limits_lo.x;
            settings.mLimitsMax = limits_hi.x;

            constraint = settings.Create(*const_cast<JPH::Body*>(b1), *const_cast<JPH::Body*>(b2));
            break;
        }
        case 2: // hinge2
        case 3: // full_control
        {
            JPH::SixDOFConstraintSettings settings;
            settings.mSpace = JPH::EConstraintSpace::WorldSpace;
            settings.mPosition1 = settings.mPosition2 = j_anchor;

            JPH::Vec3 j_axis0 = JPH::Vec3(axis0.x, axis0.y, axis0.z).Normalized();
            JPH::Vec3 j_axis1(axis1.x, axis1.y, axis1.z);
            if (j_axis1.LengthSq() > 0.001f) {
                j_axis1 = j_axis1.Normalized();
                JPH::Vec3 right = j_axis0.Cross(j_axis1);
                if (right.LengthSq() < 0.001f) {
                    j_axis1 = j_axis0.GetNormalizedPerpendicular();
                } else {
                    j_axis1 = right.Cross(j_axis0).Normalized();
                }
            } else {
                j_axis1 = j_axis0.GetNormalizedPerpendicular();
            }

            settings.mAxisX1 = settings.mAxisX2 = j_axis0;
            settings.mAxisY1 = settings.mAxisY2 = j_axis1;

            settings.mSwingType = JPH::ESwingType::Pyramid;

            settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationX);
            settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationY);
            settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationZ);

            // Лямбда для безопасного назначения лимитов SixDOF осей
            auto apply_limit = [&](JPH::SixDOFConstraintSettings::EAxis axis, float lo, float hi) {
                if (lo <= -M_PI && hi >= M_PI) {
                    settings.MakeFreeAxis(axis);
                } else if (lo == hi || lo > hi) {
                    settings.MakeFixedAxis(axis);
                } else {
                    settings.mLimitMin[(int)axis] = lo;
                    settings.mLimitMax[(int)axis] = hi;
                }
            };

            apply_limit(JPH::SixDOFConstraintSettings::EAxis::RotationX, limits_lo.x, limits_hi.x);
            apply_limit(JPH::SixDOFConstraintSettings::EAxis::RotationY, limits_lo.y, limits_hi.y);

            if (type == 3) {
                apply_limit(JPH::SixDOFConstraintSettings::EAxis::RotationZ, limits_lo.z, limits_hi.z);
            } else {
                settings.MakeFixedAxis(JPH::SixDOFConstraintSettings::EAxis::RotationZ);
            }

            constraint = settings.Create(*const_cast<JPH::Body*>(b1), *const_cast<JPH::Body*>(b2));
            break;
        }
    }

    if (constraint) {
        m_physics_system->AddConstraint(constraint);
        JointHandle handle = m_next_joint_handle++;
        m_constraints[handle] = constraint;
        if (type == 2) m_wheel_joints.insert(handle);

        if (b1 && b2) {
            m_connected_bodies[b1->GetID()].push_back(b2->GetID());
            m_connected_bodies[b2->GetID()].push_back(b1->GetID());
        }

        return handle;
    }

    return INVALID_JOINT_HANDLE;
}

void JoltPhysicsCore::DestroyJoint(JointHandle joint)
{
    if (!m_physics_system) return;

    auto it = m_constraints.find(joint);
    if (it != m_constraints.end()) {
        JPH::Constraint* c = it->second.GetPtr();
        JPH::TwoBodyConstraint* two_body_c = static_cast<JPH::TwoBodyConstraint*>(c);
        const JPH::Body* b1 = two_body_c->GetBody1();
        const JPH::Body* b2 = two_body_c->GetBody2();

        if (b1 && b2) {
            auto& vec1 = m_connected_bodies[b1->GetID()];
            vec1.erase(std::remove(vec1.begin(), vec1.end(), b2->GetID()), vec1.end());

            auto& vec2 = m_connected_bodies[b2->GetID()];
            vec2.erase(std::remove(vec2.begin(), vec2.end(), b1->GetID()), vec2.end());
        }

        m_physics_system->RemoveConstraint(it->second);
        m_constraints.erase(it);
        m_joint_feedback.erase(joint);
        m_joint_springs.erase(joint);
        m_joint_motors.erase(joint);
        m_joint_limits.erase(joint);
        m_wheel_joints.erase(joint);
    }
}

void JoltPhysicsCore::SetJointLimits(JointHandle joint, int axis_num, float lo, float hi)
{
    auto it = m_constraints.find(joint);
    if (it == m_constraints.end()) return;

    JPH::Constraint* c = it->second.GetPtr();
    if (lo > hi) std::swap(lo, hi);
    auto& limits = m_joint_limits[joint];
    auto previous = std::find_if(limits.begin(), limits.end(),
        [axis_num](const JointLimit& value) { return value.axis == axis_num; });
    if (previous != limits.end()) {
        if (previous->low == lo && previous->high == hi) return;
        *previous = {axis_num, lo, hi};
    } else limits.push_back({axis_num, lo, hi});

    if (c->GetSubType() == JPH::EConstraintSubType::Hinge) {
        auto* hinge = static_cast<JPH::HingeConstraint*>(c);
        hinge->SetLimits(std::clamp(lo, -JPH::JPH_PI, JPH::JPH_PI),
            std::clamp(hi, -JPH::JPH_PI, JPH::JPH_PI));
    } else if (c->GetSubType() == JPH::EConstraintSubType::Slider) {
        static_cast<JPH::SliderConstraint*>(c)->SetLimits(lo, hi);
    } else if (c->GetSubType() == JPH::EConstraintSubType::SixDOF) {
        R_ASSERT(axis_num >= 0 && axis_num < 3);
        auto* six = static_cast<JPH::SixDOFConstraint*>(c);
        JPH::Vec3 minimum = six->GetRotationLimitsMin(), maximum = six->GetRotationLimitsMax();
        minimum.SetComponent(axis_num, std::clamp(lo, -JPH::JPH_PI, JPH::JPH_PI));
        maximum.SetComponent(axis_num, std::clamp(hi, -JPH::JPH_PI, JPH::JPH_PI));
        six->SetRotationLimits(minimum, maximum);
    }

    if (m_physics_system) {
        JPH::TwoBodyConstraint* two_body_c = static_cast<JPH::TwoBodyConstraint*>(c);
        const JPH::Body* b1 = two_body_c->GetBody1();
        const JPH::Body* b2 = two_body_c->GetBody2();
        JPH::BodyInterface& bi = m_physics_system->GetBodyInterface();
        if (b1 && b1->IsDynamic()) { ProfileWake(WakeSource::Limit, b1->GetID()); bi.ActivateBody(b1->GetID()); }
        if (b2 && b2->IsDynamic()) { ProfileWake(WakeSource::Limit, b2->GetID()); bi.ActivateBody(b2->GetID()); }
    }
}

void JoltPhysicsCore::SetJointMotor(JointHandle joint, int axis_num, float force, float velocity)
{
    auto it = m_constraints.find(joint);
    if (it == m_constraints.end()) return;

    JPH::Constraint* c = it->second.GetPtr();
    // A zero-speed powered motor is a brake, not an inactive motor.
    bool active = force > 0.0f;
    const bool brake = active && velocity == 0;
    bool wasDriving = false;
    // Identical resistance updates must preserve warm starts and sleeping.
    auto& motors = m_joint_motors[joint];
    auto previous = std::find_if(motors.begin(), motors.end(),
        [axis_num](const JointMotor& value) { return value.axis == axis_num; });
    if (previous != motors.end()) {
        wasDriving = previous->force > 0 && previous->velocity != 0;
        if ((!active && previous->force <= 0) ||
            (previous->force == force && previous->velocity == velocity)) return;
        *previous = {axis_num, force, velocity};
    } else motors.push_back({axis_num, force, velocity});

    if (m_profile_enabled) {
        if (!_valid(force) || !_valid(velocity)) ++m_profile_motor_invalid;
        else if (velocity == 0) ++m_profile_motor_brakes;
        else ++m_profile_motor_drives;
    }

    if (c->GetSubType() == JPH::EConstraintSubType::Hinge) {
        auto* hinge = static_cast<JPH::HingeConstraint*>(c);
        hinge->SetMaxFrictionTorque(brake ? force : 0);
        hinge->SetMotorState(active && !brake ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
        if (active && !brake) {
            hinge->SetTargetAngularVelocity(velocity);
            hinge->GetMotorSettings().SetTorqueLimit(force);
        }
    } else if (c->GetSubType() == JPH::EConstraintSubType::Slider) {
        auto* slider = static_cast<JPH::SliderConstraint*>(c);
        slider->SetMaxFrictionForce(brake ? force : 0);
        slider->SetMotorState(active && !brake ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
        if (active && !brake) {
            slider->SetTargetVelocity(velocity);
            slider->GetMotorSettings().SetForceLimit(force);
        }
    } else if (c->GetSubType() == JPH::EConstraintSubType::SixDOF) {
        auto* six = static_cast<JPH::SixDOFConstraint*>(c);
        auto axis = (axis_num == 0) ? JPH::SixDOFConstraintSettings::EAxis::RotationX :
                    (axis_num == 1) ? JPH::SixDOFConstraintSettings::EAxis::RotationY :
                                      JPH::SixDOFConstraintSettings::EAxis::RotationZ;
        six->SetMaxFriction(axis, brake ? force : 0);
        six->SetMotorState(axis, active && !brake ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
        if (active && !brake) {
            JPH::Vec3 target_vel = six->GetTargetAngularVelocityCS();
            if (axis_num == 0) target_vel.SetX(velocity);
            else if (axis_num == 1) target_vel.SetY(velocity);
            else if (axis_num == 2) target_vel.SetZ(velocity);
            six->SetTargetAngularVelocityCS(target_vel);
            six->GetMotorSettings(axis).SetTorqueLimit(force);
        }
    }

    // Passive resistance cannot accelerate a resting body. Changing its cap
    // preserves native friction warm starts and does not reset sleep timers.
    // Starting/changing a drive, or stopping a previously powered drive,
    // still wakes connected bodies so the new command takes effect.
    if (m_physics_system && (wasDriving || (active && !brake))) {
        JPH::TwoBodyConstraint* two_body_c = static_cast<JPH::TwoBodyConstraint*>(c);
        const JPH::Body* b1 = two_body_c->GetBody1();
        const JPH::Body* b2 = two_body_c->GetBody2();
        JPH::BodyInterface& bi = m_physics_system->GetBodyInterface();
        if (b1 && b1->IsDynamic()) { ProfileWake(WakeSource::Motor, b1->GetID()); bi.ActivateBody(b1->GetID()); }
        if (b2 && b2->IsDynamic()) { ProfileWake(WakeSource::Motor, b2->GetID()); bi.ActivateBody(b2->GetID()); }
    }
}

void JoltPhysicsCore::SetJointSpringDamping(JointHandle joint, int axis_num, float erp, float cfm)
{
    auto it = m_constraints.find(joint);
    if (it == m_constraints.end()) return;
    JPH::Constraint* c = it->second.GetPtr();

    auto& axes = m_joint_springs[joint];
    auto settings = std::find_if(axes.begin(), axes.end(),
        [axis_num](const JointSpring& value) { return value.axis == axis_num; });
    if (settings == axes.end()) axes.push_back({axis_num, erp, cfm});
    else *settings = {axis_num, erp, cfm};
    JPH::SpringSettings spring;
    spring.mMode = JPH::ESpringMode::StiffnessAndDamping;
    // ERP = dt*k/(dt*k+c), CFM = 1/(dt*k+c).
    // A zero stiffness selects a hard limit in Jolt.
    spring.mStiffness = cfm > 0 ? std::clamp(erp, 0.f, 1.f) / (cfm * m_step_time) : 0.f;
    spring.mDamping = cfm > 0 ? (1.f - std::clamp(erp, 0.f, 1.f)) / cfm : 0.f;

    switch (c->GetSubType())
    {
    case JPH::EConstraintSubType::Hinge:
        if (axis_num == 0 || axis_num == -1)
            static_cast<JPH::HingeConstraint*>(c)->SetLimitsSpringSettings(spring);
        break;
    case JPH::EConstraintSubType::Slider:
        if (axis_num == 0 || axis_num == -1)
            static_cast<JPH::SliderConstraint*>(c)->SetLimitsSpringSettings(spring);
        break;
    case JPH::EConstraintSubType::SixDOF:
        if (axis_num == -1) {
            auto* six = static_cast<JPH::SixDOFConstraint*>(c);
            const bool wheel = m_wheel_joints.contains(joint);
            // Axis X follows the hinge2 steering/suspension direction. Equal
            // limits select a spring at zero displacement; a fixed axis bypasses it.
            six->SetTranslationLimits(wheel ? JPH::Vec3(0, FLT_MAX, FLT_MAX) : JPH::Vec3::sZero(),
                wheel ? JPH::Vec3(0, -FLT_MAX, -FLT_MAX) : JPH::Vec3::sZero());
            const int count = wheel ? 1 : 3;
            for (int index = 0; index < count; ++index)
                six->SetLimitsSpringSettings(static_cast<JPH::SixDOFConstraintSettings::EAxis>(index), spring);
        }
        if (axis_num >= 0 && axis_num < 3) {
            auto ax = axis_num == 0 ? JPH::SixDOFConstraintSettings::EAxis::RotationX
                    : axis_num == 1 ? JPH::SixDOFConstraintSettings::EAxis::RotationY
                                    : JPH::SixDOFConstraintSettings::EAxis::RotationZ;
            static_cast<JPH::SixDOFConstraint*>(c)->GetMotorSettings(ax).mSpringSettings = spring;
        }
        break;
    default: break;
    }
}

void JoltPhysicsCore::SetJointFudgeFactor(JointHandle joint, float factor) {}
void JoltPhysicsCore::SetJointFeedback(JointHandle joint, SPhysicsJointFeedback* feedback) {
    if (feedback && m_constraints.contains(joint)) m_joint_feedback[joint] = feedback;
    else m_joint_feedback.erase(joint);
}


float JoltPhysicsCore::GetJointAxisAngle(JointHandle joint, int axis_num) const {
    auto it = m_constraints.find(joint);
    if (it == m_constraints.end()) return 0.0f;

    JPH::Constraint* c = it->second.GetPtr();
    if (c->GetSubType() == JPH::EConstraintSubType::Hinge) {
        return static_cast<JPH::HingeConstraint*>(c)->GetCurrentAngle();
    } else if (c->GetSubType() == JPH::EConstraintSubType::Slider) {
        return static_cast<JPH::SliderConstraint*>(c)->GetCurrentPosition();
    } else if (c->GetSubType() == JPH::EConstraintSubType::SixDOF) {
        R_ASSERT(axis_num >= 0 && axis_num < 3);
        return static_cast<JPH::SixDOFConstraint*>(c)->GetRotationInConstraintSpace().GetEulerAngles()[axis_num];
    }
    return 0.0f;
}

void JoltPhysicsCore::GetBodyLinearVelocity(BodyHandle body_handle, Fvector& out_vel) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();

    JPH::Vec3 vel = body_interface.GetLinearVelocity(id);
    out_vel.set(vel.GetX(), vel.GetY(), vel.GetZ());
}

void JoltPhysicsCore::SetBodyLinearVelocity(BodyHandle body_handle, const Fvector& vel) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();

    if (m_profile_enabled && (vel.x != 0 || vel.y != 0 || vel.z != 0)) ProfileWake(WakeSource::LinearVelocity, id);
    body_interface.SetLinearVelocity(id, JPH::Vec3(vel.x, vel.y, vel.z));
}

void JoltPhysicsCore::GetBodyAngularVelocity(BodyHandle body_handle, Fvector& out_vel) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();

    JPH::Vec3 vel = body_interface.GetAngularVelocity(id);
    out_vel.set(vel.GetX(), vel.GetY(), vel.GetZ());
}

void JoltPhysicsCore::SetBodyAngularVelocity(BodyHandle body_handle, const Fvector& vel) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();

    if (m_profile_enabled && (vel.x != 0 || vel.y != 0 || vel.z != 0)) ProfileWake(WakeSource::AngularVelocity, id);
    body_interface.SetAngularVelocity(id, JPH::Vec3(vel.x, vel.y, vel.z));
}

void JoltPhysicsCore::SetBodyGravityFactor(BodyHandle body_handle, float factor) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    m_physics_system->GetBodyInterface().SetGravityFactor(id, factor);
}

void JoltPhysicsCore::SetBodyDamping(BodyHandle body, float linear, float angular)
{
    if (!m_physics_system || body == INVALID_BODY_HANDLE) return;
    JPH::BodyLockWrite lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(body));
    if (lock.Succeeded() && lock.GetBody().IsDynamic())
    {
        auto* motion = lock.GetBody().GetMotionProperties();
        motion->SetLinearDamping(linear);
        motion->SetAngularDamping(angular);
    }
}

void JoltPhysicsCore::ApplyLinearImpulse(BodyHandle body_handle, const Fvector& impulse) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    JPH::BodyID id(body_handle);
    JPH::BodyInterface& bi = m_physics_system->GetBodyInterface();
    bi.ActivateBody(id);
    bi.AddImpulse(id, JPH::Vec3(impulse.x, impulse.y, impulse.z));
}

void JoltPhysicsCore::ApplyPointImpulse(BodyHandle body_handle, const Fvector& impulse, const Fvector& point) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    JPH::BodyID id(body_handle);
    m_physics_system->GetBodyInterface().AddImpulse(id,
        JPH::Vec3(impulse.x, impulse.y, impulse.z),
        JPH::Vec3(point.x, point.y, point.z));
}

void JoltPhysicsCore::ApplyForce(BodyHandle body_handle, const Fvector& force) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    JPH::BodyID id(body_handle);
    if (m_profile_enabled) ProfileWake(WakeSource::Force, id);
    m_physics_system->GetBodyInterface().AddForce(id, JPH::Vec3(force.x, force.y, force.z));
}

void JoltPhysicsCore::ApplyTorque(BodyHandle body_handle, const Fvector& torque) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    JPH::BodyID id(body_handle);
    if (m_profile_enabled) ProfileWake(WakeSource::Torque, id);
    m_physics_system->GetBodyInterface().AddTorque(id, JPH::Vec3(torque.x, torque.y, torque.z));
}

bool JoltPhysicsCore::IsBodyActive(BodyHandle body_handle) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return false;
    JPH::BodyID id(body_handle);
    return m_physics_system->GetBodyInterface().IsActive(id);
}

void JoltPhysicsCore::ActivateBody(BodyHandle body_handle) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    JPH::BodyID id(body_handle);
    auto& bodies = m_physics_system->GetBodyInterface();
    if (bodies.IsAdded(id)) { ProfileWake(WakeSource::Explicit, id); bodies.ActivateBody(id); }
}

void JoltPhysicsCore::DeactivateBody(BodyHandle body_handle) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    JPH::BodyID id(body_handle);
    m_physics_system->GetBodyInterface().DeactivateBody(id);
}

float JoltPhysicsCore::GetBodyMass(BodyHandle body_handle) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return 0.0f;

    JPH::BodyID id(body_handle);

    JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), id);
    if (!lock.Succeeded()) return 0.0f;

    const JPH::Body& body = lock.GetBody();
    const auto* motion = body.GetMotionPropertiesUnchecked();
    return motion && motion->GetInverseMass() > 0 ? 1.0f / motion->GetInverseMass() : 0.0f;
}

void JoltPhysicsCore::GetBodyPointVelocity(BodyHandle body_handle, const Fvector& point, Fvector& velocity) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) {
        velocity.set(0.f, 0.f, 0.f);
        return;
    }

    JPH::BodyID id(body_handle);
    if (!m_physics_system->GetBodyInterface().IsAdded(id)) {
        velocity.set(0.f, 0.f, 0.f);
        return;
    }

    JPH::Vec3 jolt_pos(point.x, point.y, point.z);
    JPH::Vec3 jolt_vel = m_physics_system->GetBodyInterface().GetPointVelocity(id, jolt_pos);

    velocity.set(jolt_vel.GetX(), jolt_vel.GetY(), jolt_vel.GetZ());
}

void JoltPhysicsCore::SetBodyIgnoreStatic(BodyHandle body_handle) {
    SetBodyCollideWithStatics(body_handle, false);
}

void JoltPhysicsCore::SetBodyCollideWithStatics(BodyHandle body_handle, bool collide) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    JPH::BodyID id(body_handle);

    if (collide)
        m_ignore_static_bodies.erase(id);
    else
        m_ignore_static_bodies[id] = true;

    JPH::EMotionType motion = m_physics_system->GetBodyLockInterface()
        .TryGetBody(id)->GetMotionType();
    JPH::ObjectLayer layer = collide ? Layers::MOVING : Layers::MOVING_NO_STATIC;

    if (motion != JPH::EMotionType::Static)
        m_physics_system->GetBodyInterface().SetObjectLayer(id, layer);
}

void JoltPhysicsCore::SetBodyObjectLayer(BodyHandle body_handle, u32 layer) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    JPH::BodyID id(body_handle);
    if (m_physics_system->GetBodyInterface().IsAdded(id)) {
        m_physics_system->GetBodyInterface().SetObjectLayer(id, static_cast<JPH::ObjectLayer>(layer));
    }
}



void JoltPhysicsCore::GetBodyPosition(BodyHandle body_handle, Fvector& position) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) {
        position.set(0.f, 0.f, 0.f);
        return;
    }

    JPH::BodyID id(body_handle);
    JPH::Vec3 jolt_pos = m_physics_system->GetBodyInterface().GetCenterOfMassPosition(id);

    position.set(jolt_pos.GetX(), jolt_pos.GetY(), jolt_pos.GetZ());
}

void JoltPhysicsCore::SetBodyPosition(BodyHandle body_handle, const Fvector& position) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;
    R_ASSERT2(is_valid_pos(position), "SetBodyPosition: Invalid position!");

    JPH::BodyID id(body_handle);
    JPH::BodyInterface& body_interface = m_physics_system->GetBodyInterface();

    JPH::Vec3 com_world(position.x, position.y, position.z);
    JPH::Quat rotation = body_interface.GetRotation(id);
    JPH::Vec3 com_local = body_interface.GetShape(id)->GetCenterOfMass();

    JPH::Vec3 shape_origin = com_world - rotation * com_local;
    body_interface.SetPosition(id, shape_origin,
        body_interface.IsAdded(id) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
}

void JoltPhysicsCore::GetBodyForce(BodyHandle body_handle, Fvector& force) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) {
        force.set(0.f, 0.f, 0.f);
        return;
    }
    JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(body_handle));
    force.set(0, 0, 0);
    if (lock.Succeeded() && lock.GetBody().IsDynamic()) {
        const auto value = lock.GetBody().GetAccumulatedForce();
        force.set(value.GetX(), value.GetY(), value.GetZ());
    }
}

void JoltPhysicsCore::SetBodyForce(BodyHandle body_handle, const Fvector& force) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);

    JPH::Vec3 jolt_force(force.x, force.y, force.z);
    JPH::BodyLockWrite lock(m_physics_system->GetBodyLockInterface(), id);
    if (lock.Succeeded() && lock.GetBody().IsDynamic()) {
        const auto previous = lock.GetBody().GetAccumulatedForce();
        lock.GetBody().AddForce(jolt_force - previous);
    }
}

float JoltPhysicsCore::GetBodyGravityFactor(BodyHandle body_handle) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return 1.0f;
    JPH::BodyID id(body_handle);
    return m_physics_system->GetBodyInterface().GetGravityFactor(id);
}

void* JoltPhysicsCore::GetBodyUserData(BodyHandle body_handle) const {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return nullptr;

    JPH::BodyID id(body_handle);
    JPH::uint64 user_data = m_physics_system->GetBodyInterface().GetUserData(id);

    return reinterpret_cast<void*>(user_data);
}

void JoltPhysicsCore::SetBodyUserData(BodyHandle body_handle, void* data) {
    if (!m_physics_system || body_handle == INVALID_BODY_HANDLE) return;

    JPH::BodyID id(body_handle);
    JPH::BodyLockWrite lock(m_physics_system->GetBodyLockInterface(), id);
    if (lock.Succeeded()) {
        lock.GetBody().SetUserData(reinterpret_cast<JPH::uint64>(data));
    }
}

PhysicsShapeHandle JoltPhysicsCore::CreateBoxShape(const Fvector& half_extents) {
    float hx = std::max(half_extents.x, 0.001f);
    float hy = std::max(half_extents.y, 0.001f);
    float hz = std::max(half_extents.z, 0.001f);
    JPH::RefConst<JPH::Shape> shape = new JPH::BoxShape(JPH::Vec3(hx, hy, hz));
    shape->AddRef();
    return reinterpret_cast<PhysicsShapeHandle>(const_cast<JPH::Shape*>(shape.GetPtr()));
}

PhysicsShapeHandle JoltPhysicsCore::CreateSphereShape(float radius) {
    float r = std::max(radius, 0.001f);
    JPH::RefConst<JPH::Shape> shape = new JPH::SphereShape(r);
    shape->AddRef();
    return reinterpret_cast<PhysicsShapeHandle>(const_cast<JPH::Shape*>(shape.GetPtr()));
}

PhysicsShapeHandle JoltPhysicsCore::CreateCylinderShape(float radius, float half_height) {
    float hh = std::max(half_height, 0.001f);
    float r = std::max(radius, 0.001f);
    JPH::RefConst<JPH::Shape> cylinder_shape = new JPH::CylinderShape(hh, r);

    JPH::Shape* shape = const_cast<JPH::Shape*>(cylinder_shape.GetPtr());
    shape->AddRef();
    return reinterpret_cast<PhysicsShapeHandle>(shape);
}

PhysicsShapeHandle JoltPhysicsCore::CreateCapsuleShape(float radius, float half_height) {
    float hh = std::max(half_height, 0.001f);
    float r = std::max(radius, 0.001f);
    JPH::RefConst<JPH::Shape> capsule = new JPH::CapsuleShape(hh, r);
    capsule->AddRef();
    return reinterpret_cast<PhysicsShapeHandle>(const_cast<JPH::Shape*>(capsule.GetPtr()));
}

class JoltIgnoreActorBodyFilter : public JPH::BodyFilter {
public:
    JPH::uint64 m_actor_user_data;
    IPhysicsCore::QueryFilterFun m_filter;
    bool m_camera;

    JoltIgnoreActorBodyFilter(JPH::uint64 user_data,
        IPhysicsCore::QueryFilterFun filter = nullptr, bool camera = false)
        : m_actor_user_data(user_data), m_filter(filter), m_camera(camera) {}

    bool ShouldCollideLocked(const JPH::Body& body) const override {
        // NarrowPhaseQuery already holds the read lock here. Inspecting body
        // members in ShouldCollide used to acquire and release it a second time.
        if (body.IsSensor()) return false;
        if (m_filter && !m_filter(reinterpret_cast<void*>(body.GetUserData()),
            body.GetObjectLayer(), reinterpret_cast<void*>(m_actor_user_data), m_camera)) return false;
        if (m_actor_user_data != 0 && body.GetUserData() == m_actor_user_data) {
            return false;
        }
        // Alive characters collide through their CharacterVirtual controller.
        if (body.GetObjectLayer() == Layers::RAGDOLL && body.IsKinematic()) {
            return false;
        }
        return true;
    }
};

struct SMaterialPhysicsCache {
    float friction = 0.7f;
    float restitution = 0.0f;
    float bounce_start_velocity = 0;
    bool valid = false;
};
static inline SMaterialPhysicsCache GetMaterialPhysicsProperties(u16 mtl_idx)
{
    SMaterialPhysicsCache entry;
    if (mtl_idx < GMLib.CountMaterial()) {
        const auto* mtl = GMLib.GetMaterialByIdx(mtl_idx);
        entry.friction = mtl->fPHFriction;
          entry.restitution = mtl->Flags.test(SGameMtl::flBounceable) ? mtl->fPHBouncing : 0.0f;
          entry.bounce_start_velocity = mtl->fPHBounceStartVelocity;
          entry.valid = true;
    }
    return entry;
}

void JoltPhysicsCore::MyContactListener::OnContactAdded(
    const JPH::Body& inBody1,
    const JPH::Body& inBody2,
    const JPH::ContactManifold& inManifold,
    JPH::ContactSettings& ioSettings)
{
    if (m_core->m_rejected_contacts.contains(Key(inBody1.GetID().GetIndexAndSequenceNumber(),
        inBody2.GetID().GetIndexAndSequenceNumber(), inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue()))) {
        ioSettings.mIsSensor = true;
        return;
    }
    if (m_core->m_collision_filter && !m_core->m_collision_filter(
        reinterpret_cast<void*>(inBody1.GetUserData()), inBody1.GetObjectLayer(),
        reinterpret_cast<void*>(inBody2.GetUserData()), inBody2.GetObjectLayer())) {
        ioSettings.mIsSensor = true;
        return;
    }
    m_core->m_contact_points.fetch_add(static_cast<u32>(inManifold.mRelativeContactPointsOn1.size()),
        std::memory_order_relaxed);
    m_core->QueueDeferredContact(inBody1, inBody2, inManifold.mSubShapeID1, inManifold.mSubShapeID2,
        inManifold.GetWorldSpaceContactPointOn2(0), -inManifold.mWorldSpaceNormal, inManifold.mPenetrationDepth);
    auto applyFrictionPolicy = [&] {
        float nativeFriction = -1;
        // Match the canonical order used by prepared game callbacks.
        const auto firstID = inBody1.GetID(), secondID = inBody2.GetID();
        for (const auto id : {std::min(firstID, secondID), std::max(firstID, secondID)})
            if (id.GetIndex() < m_core->m_contact_bodies.size()) {
                const auto& snapshot = m_core->m_contact_bodies[id.GetIndex()];
                if (snapshot.id == id && snapshot.policy.friction >= 0) nativeFriction = snapshot.policy.friction;
            }
        if (nativeFriction >= 0) ioSettings.mCombinedFriction = nativeFriction;
        const auto policy = m_core->m_contact_friction.find(Key(inBody1.GetID().GetIndexAndSequenceNumber(),
            inBody2.GetID().GetIndexAndSequenceNumber(), inManifold.mSubShapeID1.GetValue(), inManifold.mSubShapeID2.GetValue()));
        if (policy != m_core->m_contact_friction.end()) {
            if (policy->second.friction >= 0) ioSettings.mCombinedFriction = policy->second.friction;
            ioSettings.mCombinedFriction *= policy->second.scale;
            const bool ordered = inBody1.GetID() < inBody2.GetID();
            if (ordered ? policy->second.static_lower : policy->second.static_higher)
                ioSettings.mInvMassScale1 = ioSettings.mInvInertiaScale1 = 0;
            if (ordered ? policy->second.static_higher : policy->second.static_lower)
                ioSettings.mInvMassScale2 = ioSettings.mInvInertiaScale2 = 0;
        }
    };
    const auto firstMaterial = GetMaterialPhysicsProperties(GetMaterialIndexForSubShape(inBody1.GetShape(), inManifold.mSubShapeID1));
    const auto secondMaterial = GetMaterialPhysicsProperties(GetMaterialIndexForSubShape(inBody2.GetShape(), inManifold.mSubShapeID2));
    if (firstMaterial.valid && secondMaterial.valid) {
        ioSettings.mCombinedFriction = firstMaterial.friction * secondMaterial.friction;
        ioSettings.mCombinedRestitution = std::min(firstMaterial.restitution, secondMaterial.restitution);
        const auto point = inManifold.GetWorldSpaceContactPointOn1(0);
        const auto relative = inBody2.GetPointVelocity(point) - inBody1.GetPointVelocity(point);
        if (std::abs(relative.Dot(inManifold.mWorldSpaceNormal)) <
            std::max(firstMaterial.bounce_start_velocity, secondMaterial.bounce_start_velocity))
            ioSettings.mCombinedRestitution = 0;
        applyFrictionPolicy();
        return;
    }
    const JPH::Body* static_body = inBody1.IsStatic() ? &inBody1 : (inBody2.IsStatic() ? &inBody2 : nullptr);
    const JPH::Body* dynamic_body = inBody1.IsDynamic() ? &inBody1 : (inBody2.IsDynamic() ? &inBody2 : nullptr);

    if (static_body && dynamic_body)
    {
        JPH::SubShapeID sub_shape = inBody1.IsStatic() ? inManifold.mSubShapeID1 : inManifold.mSubShapeID2;
        const JPH::Shape* shape = static_body->GetShape();

        if (shape)
        {
            u16 mtl_idx = GetMaterialIndexForSubShape(shape, sub_shape);
            if (mtl_idx != GAMEMTL_NONE_IDX)
            {
                const auto& mtl = GetMaterialPhysicsProperties(mtl_idx);
                ioSettings.mCombinedFriction = std::sqrt(dynamic_body->GetFriction() * mtl.friction);
                ioSettings.mCombinedRestitution = std::max(dynamic_body->GetRestitution(), mtl.restitution);
            }
        }
    }
    else
    {
        ioSettings.mCombinedFriction = std::sqrt(inBody1.GetFriction() * inBody2.GetFriction());
        ioSettings.mCombinedRestitution = std::max(inBody1.GetRestitution(), inBody2.GetRestitution());
    }
    applyFrictionPolicy();
}

void JoltPhysicsCore::MyContactListener::OnContactPersisted(
    const JPH::Body& inBody1,
    const JPH::Body& inBody2,
    const JPH::ContactManifold& inManifold,
    JPH::ContactSettings& ioSettings)
{
    OnContactAdded(inBody1, inBody2, inManifold, ioSettings);
}

void JoltPhysicsCore::MyBodyActivationListener::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
    if (inBodyUserData == 0) return;
    if (m_core->m_physics_system) {
        JPH::ObjectLayer layer = m_core->m_physics_system->GetBodyInterfaceNoLock().GetObjectLayer(inBodyID);
        if (layer != Layers::MOVING) return;
    }

    int t_idx = GetJoltThreadIndex();
    Enqueue(g_deferred_activations[t_idx], {
        inBodyID.GetIndexAndSequenceNumber(),
        reinterpret_cast<void*>(inBodyUserData),
        true
    });
}

void JoltPhysicsCore::MyBodyActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
    if (inBodyUserData == 0) return;
    if (m_core->m_physics_system) {
        JPH::ObjectLayer layer = m_core->m_physics_system->GetBodyInterfaceNoLock().GetObjectLayer(inBodyID);
        if (layer != Layers::MOVING) return;
    }

    int t_idx = GetJoltThreadIndex();
    Enqueue(g_deferred_activations[t_idx], {
        inBodyID.GetIndexAndSequenceNumber(),
        reinterpret_cast<void*>(inBodyUserData),
        false
    });
}

void JoltPhysicsCore::SetBodyActivationCallback(BodyActivationCallbackFun callback)
{
    m_body_activation_callback = callback;
}

void JoltPhysicsCore::MyCharacterContactListener::OnContactAdded(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::CharacterContact& inContact,
    JPH::CharacterContactSettings& ioSettings)
{
    if (!inContact.mBodyB.IsInvalid() && m_core->m_physics_system)
    {
        JPH::BodyLockWrite lock(m_core->m_physics_system->GetBodyLockInterface(), inContact.mBodyB);
        if (lock.Succeeded())
        {
            JPH::Body& body = lock.GetBody();

            if (body.IsStatic() && body.GetShape())
            {
                u32 tri_user_data = GetTriangleUserDataForSubShape(body.GetShape(), inContact.mSubShapeIDB);
                u16 mtl_idx = GetMaterialIndexForSubShape(body.GetShape(), inContact.mSubShapeIDB);
                if (mtl_idx != GAMEMTL_NONE_IDX && mtl_idx < GMLib.CountMaterial())
                {
                    SGameMtl* mtl = GMLib.GetMaterialByIdx(mtl_idx);
                    if (mtl && IsPassableMaterial(mtl))
                    {
                        ioSettings.mCanPushCharacter = false;
                        ioSettings.mCanReceiveImpulses = false;
                    }
                }
            }

        }
    }

    ProcessContact(inCharacter, inContact);
}

void JoltPhysicsCore::MyCharacterContactListener::OnContactPersisted(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::CharacterContact& inContact,
    JPH::CharacterContactSettings& ioSettings)
{
    if (!inContact.mBodyB.IsInvalid() && m_core->m_physics_system)
    {
        JPH::BodyLockRead lock(m_core->m_physics_system->GetBodyLockInterface(), inContact.mBodyB);
        if (lock.Succeeded())
        {
            const JPH::Body& body = lock.GetBody();
            if (body.IsStatic() && body.GetShape())
            {
                u32 tri_user_data = GetTriangleUserDataForSubShape(body.GetShape(), inContact.mSubShapeIDB);
                u16 mtl_idx = GetMaterialIndexForSubShape(body.GetShape(), inContact.mSubShapeIDB);
                if (mtl_idx != GAMEMTL_NONE_IDX && mtl_idx < GMLib.CountMaterial())
                {
                    SGameMtl* mtl = GMLib.GetMaterialByIdx(mtl_idx);
                    if (mtl && IsPassableMaterial(mtl))
                    {
                        ioSettings.mCanPushCharacter = false;
                        ioSettings.mCanReceiveImpulses = false;
                    }
                }
            }
        }
    }
    ProcessContact(inCharacter, inContact);
}

void JoltPhysicsCore::MyCharacterContactListener::OnCharacterContactAdded(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::CharacterContact& inContact,
    JPH::CharacterContactSettings& ioSettings)
{
    // Enable mutual pushing between characters so they don't lock each other rigidly
    ioSettings.mCanPushCharacter = true;
    ioSettings.mCanReceiveImpulses = true;
}

void JoltPhysicsCore::MyCharacterContactListener::OnCharacterContactPersisted(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::CharacterContact& inContact,
    JPH::CharacterContactSettings& ioSettings)
{
    ioSettings.mCanPushCharacter = true;
    ioSettings.mCanReceiveImpulses = true;
}

void JoltPhysicsCore::MyCharacterContactListener::OnCharacterContactSolve(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::CharacterVirtual* inOtherCharacter,
    const JPH::SubShapeID& inSubShapeID2,
    JPH::RVec3Arg inContactPosition,
    JPH::Vec3Arg inContactNormal,
    JPH::Vec3Arg inContactVelocity,
    const JPH::PhysicsMaterial* inContactMaterial,
    JPH::Vec3Arg inCharacterVelocity,
    JPH::Vec3& ioNewCharacterVelocity)
{
    // Natural collision sliding and penetration resolution are handled by Jolt's solver
}

void JoltPhysicsCore::MyCharacterContactListener::ProcessContact(const JPH::CharacterVirtual* inCharacter, const JPH::CharacterContact& inContact) {
    if (!inCharacter) return;

    u32 tri_user_data = u32(-1);
    void* other_body_user_data = nullptr;
    BodyHandle other_body_handle = INVALID_BODY_HANDLE;

    if (!inContact.mBodyB.IsInvalid() && m_core->m_physics_system) {
        JPH::BodyLockRead lock(m_core->m_physics_system->GetBodyLockInterface(), inContact.mBodyB);
        if (lock.Succeeded()) {
            const JPH::Body& body = lock.GetBody();
            if (body.IsDynamic() && body.GetObjectLayer() == Layers::MOVING) {
                other_body_handle = inContact.mBodyB.GetIndexAndSequenceNumber();
                other_body_user_data = reinterpret_cast<void*>(body.GetUserData());
            }
            const JPH::Shape* shape = body.GetShape();
            if (shape) {
                tri_user_data = UnpackTriangleIndex(GetTriangleUserDataForSubShape(shape, inContact.mSubShapeIDB));
            }
        }
    }

    Fvector contact_pos = { (float)inContact.mPosition.GetX(), (float)inContact.mPosition.GetY(), (float)inContact.mPosition.GetZ() };
    Fvector contact_norm = { inContact.mContactNormal.GetX(), inContact.mContactNormal.GetY(), inContact.mContactNormal.GetZ() };
    Fvector contact_vel = { inContact.mLinearVelocity.GetX(), inContact.mLinearVelocity.GetY(), inContact.mLinearVelocity.GetZ() };

    for (const auto& [handle, char_ptr] : m_core->m_characters) {
        if (char_ptr.GetPtr() == inCharacter) {
            auto cb_it = m_core->m_character_callbacks.find(handle);
            if (cb_it != m_core->m_character_callbacks.end() && cb_it->second.callback) {
                int t_idx = GetJoltThreadIndex();
                Enqueue(g_deferred_contacts[t_idx], {
                    cb_it->second.callback, cb_it->second.user_data,
                    contact_pos, contact_norm, contact_vel,
                    tri_user_data, other_body_handle, other_body_user_data, inContact.mIsSensorB
                });
            }
            break;
        }
    }
}

JPH::ValidateResult JoltPhysicsCore::MyContactListener::OnContactValidate(
    const JPH::Body& inBody1,
    const JPH::Body& inBody2,
    JPH::RVec3Arg inBaseOffset,
    const JPH::CollideShapeResult& inCollisionResult)
{
    if (m_core->m_rejected_contacts.contains(Key(inBody1.GetID().GetIndexAndSequenceNumber(),
        inBody2.GetID().GetIndexAndSequenceNumber(), inCollisionResult.mSubShapeID1.GetValue(),
        inCollisionResult.mSubShapeID2.GetValue()))) return JPH::ValidateResult::RejectContact;
    if (m_core->m_collision_filter && !m_core->m_collision_filter(
        reinterpret_cast<void*>(inBody1.GetUserData()), inBody1.GetObjectLayer(),
        reinterpret_cast<void*>(inBody2.GetUserData()), inBody2.GetObjectLayer()))
        return JPH::ValidateResult::RejectAllContactsForThisBodyPair;
    auto it = m_core->m_connected_bodies.find(inBody1.GetID());
    if (it != m_core->m_connected_bodies.end()) {
        const auto& connected = it->second;
        if (std::find(connected.begin(), connected.end(), inBody2.GetID()) != connected.end()) {
            return JPH::ValidateResult::RejectAllContactsForThisBodyPair;
        }
    }

    const JPH::Body* static_body = inBody1.IsStatic() ? &inBody1 : (inBody2.IsStatic() ? &inBody2 : nullptr);
    if (static_body)
    {
        JPH::SubShapeID sub_shape = inBody1.IsStatic() ? inCollisionResult.mSubShapeID1 : inCollisionResult.mSubShapeID2;
        const JPH::Shape* shape = static_body->GetShape();
        if (shape)
        {
            u32 tri_user_data = GetTriangleUserDataForSubShape(shape, sub_shape);
            u32 tri_idx = UnpackTriangleIndex(tri_user_data);
            u16 mtl_idx = GetMaterialIndexForSubShape(shape, sub_shape);
            if (mtl_idx != GAMEMTL_NONE_IDX)
            {
                SGameMtl* mtl = (mtl_idx < GMLib.CountMaterial()) ? GMLib.GetMaterialByIdx(mtl_idx) : nullptr;
                if (mtl && IsPassableMaterial(mtl))
                {
                    m_core->QueueDeferredContact(inBody1, inBody2, inCollisionResult.mSubShapeID1,
                        inCollisionResult.mSubShapeID2, inBaseOffset + inCollisionResult.mContactPointOn2,
                        -inCollisionResult.mPenetrationAxis.NormalizedOr(JPH::Vec3::sAxisY()), inCollisionResult.mPenetrationDepth);
                    return JPH::ValidateResult::RejectContact;
                }
            }
        }
    }

    return JPH::ValidateResult::AcceptContact;
}

CharacterVirtualHandle JoltPhysicsCore::CreateCharacterVirtual(PhysicsShapeHandle shape, const Fvector& initial_pos, float mass) {
    R_ASSERT2(is_valid_pos(initial_pos), "CreateCharacterVirtual: Invalid initial character position!");
    JPH::Shape* jolt_shape = reinterpret_cast<JPH::Shape*>(shape);

    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mShape = jolt_shape;
    settings->mMass = mass;
    settings->mMaxSlopeAngle = JPH::DegreesToRadians(60.0f);
    settings->mEnhancedInternalEdgeRemoval = true;
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.1f);

    JPH::RVec3 pos(initial_pos.x, initial_pos.y, initial_pos.z);

    JPH::CharacterVirtual* character = new JPH::CharacterVirtual(settings, pos, JPH::Quat::sIdentity(), 0, m_physics_system);
    character->SetListener(&m_character_contact_listener);
    character->SetCharacterVsCharacterCollision(&m_char_vs_char_collision);
    m_char_vs_char_collision.Add(character);

    CharacterVirtualHandle handle = m_next_character_handle++;
    m_characters[handle] = character;
    m_stick_to_floor[handle] = true;
    return handle;
}

void JoltPhysicsCore::DestroyCharacterVirtual(CharacterVirtualHandle handle) {
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        if (it->second) {
            m_char_vs_char_collision.Remove(it->second.GetPtr());
        }
        m_characters.erase(it);
        m_stick_to_floor.erase(handle);
        m_character_callbacks.erase(handle);
        m_character_gravity_factors.erase(handle);
        m_inactive_characters.erase(handle);
    }
}

void JoltPhysicsCore::SetCharacterVirtualVelocity(CharacterVirtualHandle handle, const Fvector& velocity) {
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        it->second->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
    }
}

void JoltPhysicsCore::GetCharacterVirtualVelocity(CharacterVirtualHandle handle, Fvector& velocity) const {
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        JPH::Vec3 vel = it->second->GetLinearVelocity();
        velocity.set(vel.GetX(), vel.GetY(), vel.GetZ());
    }
}

void JoltPhysicsCore::GetCharacterVirtualPosition(CharacterVirtualHandle handle, Fvector& position) const {
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        JPH::CharacterVirtual* character = it->second.GetPtr();
        JPH::RVec3 pos = character->GetPosition();
        pos -= character->GetUp() * character->GetCharacterPadding();
        position.set(pos.GetX(), pos.GetY(), pos.GetZ());
    }
}

void JoltPhysicsCore::SetCharacterVirtualPosition(CharacterVirtualHandle handle, const Fvector& position) {
    R_ASSERT2(is_valid_pos(position), "SetCharacterVirtualPosition: Invalid position!");
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        JPH::CharacterVirtual* character = it->second.GetPtr();
        JPH::RVec3 pos(position.x, position.y, position.z);
        pos += character->GetUp() * character->GetCharacterPadding();
        character->SetPosition(pos);
        if (m_physics_system && m_temp_allocator) {
            JoltIgnoreActorBodyFilter filter(character->GetUserData(), m_query_filter);
            character->RefreshContacts(m_physics_system->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                m_physics_system->GetDefaultLayerFilter(Layers::MOVING), filter, {}, *m_temp_allocator);
        }
    }
}

void JoltPhysicsCore::SetCharacterVirtualShape(CharacterVirtualHandle handle, PhysicsShapeHandle shape) {
    auto it = m_characters.find(handle);
    if (it != m_characters.end() && m_physics_system && m_temp_allocator && shape) {
        JPH::Shape* jolt_shape = reinterpret_cast<JPH::Shape*>(shape);
        JPH::CharacterVirtual* character = it->second.GetPtr();
        if (!character) return;

        JoltIgnoreActorBodyFilter body_filter(character->GetUserData(), m_query_filter);

        if (character->SetShape(jolt_shape, 1.5f,
                             m_physics_system->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                             m_physics_system->GetDefaultLayerFilter(Layers::MOVING),
                             body_filter, {}, *m_temp_allocator))
        {
            character->SetInnerBodyShape(jolt_shape);
        }
    }
}

void JoltPhysicsCore::ActivateCharacterVirtual(CharacterVirtualHandle handle) {
    const auto found = m_characters.find(handle);
    if (found != m_characters.end() && m_inactive_characters.erase(handle))
        m_char_vs_char_collision.Add(found->second.GetPtr());
}

void JoltPhysicsCore::DeactivateCharacterVirtual(CharacterVirtualHandle handle) {
    const auto found = m_characters.find(handle);
    if (found != m_characters.end() && m_inactive_characters.insert(handle).second)
        m_char_vs_char_collision.Remove(found->second.GetPtr());
}

bool JoltPhysicsCore::IsCharacterVirtualOnGround(CharacterVirtualHandle handle) const {
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        return it->second->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround && !it->second->IsSlopeTooSteep(it->second->GetGroundNormal());
    }
    return false;
}

void JoltPhysicsCore::GetCharacterVirtualGroundState(CharacterVirtualHandle handle, SJoltCharacterGroundState& out_state) const {
    out_state.on_ground = false;
    out_state.ground_normal.set(0.f, 1.f, 0.f);
    out_state.ground_velocity.set(0.f, 0.f, 0.f);
    out_state.ground_triangle_user_data = u32(-1);

    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        JPH::CharacterVirtual* character = it->second.GetPtr();

        out_state.on_ground = (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround ||
                               character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnSteepGround);

        JPH::Vec3 normal = character->GetGroundNormal();
        out_state.ground_normal.set(normal.GetX(), normal.GetY(), normal.GetZ());

        JPH::Vec3 ground_vel = character->GetGroundVelocity();
        out_state.ground_velocity.set(ground_vel.GetX(), ground_vel.GetY(), ground_vel.GetZ());

        JPH::BodyID ground_body = character->GetGroundBodyID();
        JPH::SubShapeID ground_sub_shape = character->GetGroundSubShapeID();
        if (!ground_body.IsInvalid() && m_physics_system) {
            JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), ground_body);
            if (lock.Succeeded()) {
                const JPH::Body& body = lock.GetBody();
                const JPH::Shape* shape = body.GetShape();
                if (shape) {
                    out_state.ground_triangle_user_data = UnpackTriangleIndex(GetTriangleUserDataForSubShape(shape, ground_sub_shape));
                }
            }
        }
    }
}

void JoltPhysicsCore::SetCharacterVirtualUserData(CharacterVirtualHandle handle, void* data) {
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        it->second->SetUserData(reinterpret_cast<JPH::uint64>(data));
    }
}

void JoltPhysicsCore::SetCharacterVirtualContactCallback(CharacterVirtualHandle handle, CharacterContactCallbackFun callback, void* char_user_data) {
    if (callback || char_user_data) {
        m_character_callbacks[handle] = { callback, char_user_data };
    } else {
        m_character_callbacks.erase(handle);
    }
}


class JoltPassableContactCollector : public JPH::CollideShapeCollector {
public:
    JPH::PhysicsSystem* m_system;
    IPhysicsCore::CharacterContactCallbackFun m_callback;
    void* m_user_data;
    Fvector m_char_vel;

    JoltPassableContactCollector(JPH::PhysicsSystem* sys, IPhysicsCore::CharacterContactCallbackFun cb, void* ud, const Fvector& vel)
        : m_system(sys), m_callback(cb), m_user_data(ud), m_char_vel(vel) {}

    virtual void AddHit(const JPH::CollideShapeResult& inResult) override {
        if (inResult.mBodyID2.IsInvalid() || !m_callback || !m_system) return;

        JPH::BodyLockRead lock(m_system->GetBodyLockInterface(), inResult.mBodyID2);
        if (!lock.Succeeded()) return;

        const JPH::Body& body = lock.GetBody();
        const JPH::Shape* shape = body.GetShape();
        if (!shape) return;

        u16 mtl_idx = GetMaterialIndexForSubShape(shape, inResult.mSubShapeID2);
        if (mtl_idx == GAMEMTL_NONE_IDX) return;

        SGameMtl* mtl = GMLib.GetMaterialByIdx(mtl_idx);

        if (IsPassableMaterial(mtl) || (mtl && mtl->Flags.test(SGameMtl::flPassable))) {
            JPH::RVec3 hit_pos = inResult.mContactPointOn2;
            JPH::Vec3 hit_norm = -inResult.mPenetrationAxis.Normalized();

            Fvector contact_pos = { (float)hit_pos.GetX(), (float)hit_pos.GetY(), (float)hit_pos.GetZ() };
            Fvector contact_norm = { hit_norm.GetX(), hit_norm.GetY(), hit_norm.GetZ() };
            u32 tri_idx = UnpackTriangleIndex(GetTriangleUserDataForSubShape(shape, inResult.mSubShapeID2));

            int t_idx = GetJoltThreadIndex();
            Enqueue(g_deferred_contacts[t_idx], {
                m_callback, m_user_data,
                contact_pos, contact_norm, m_char_vel,
                tri_idx, INVALID_BODY_HANDLE, nullptr, true
            });
        }
    }
};


void JoltPhysicsCore::UpdateCharacterVirtual(CharacterVirtualHandle handle, float delta_time, const Fvector& gravity) {
    if (m_inactive_characters.contains(handle)) return;
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        JPH::CharacterVirtual* character = it->second.GetPtr();

        float gravity_factor = 1.0f;
        if (m_character_gravity_factors.find(handle) != m_character_gravity_factors.end()) {
            gravity_factor = m_character_gravity_factors[handle];
        }

        JPH::Vec3 current_vel = character->GetLinearVelocity();
        JPH::Vec3 jolt_gravity(gravity.x, gravity.y, gravity.z);
        JPH::Vec3 applied_gravity = jolt_gravity * gravity_factor;

        auto ground_state = character->GetGroundState();

        if (ground_state == JPH::CharacterVirtual::EGroundState::OnGround ||
            ground_state == JPH::CharacterVirtual::EGroundState::OnSteepGround)
        {
            if (current_vel.GetY() < 0.0f) {
                current_vel.SetY(0.0f);
            }
        } else {
            current_vel += applied_gravity * delta_time;
            if (current_vel.GetY() < -100.0f) {
                current_vel.SetY(-100.0f);
            }
        }

        character->SetLinearVelocity(current_vel);

        JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
        bool stick_to_floor = true;
        if (m_stick_to_floor.find(handle) != m_stick_to_floor.end()) {
            stick_to_floor = m_stick_to_floor[handle];
        }

        if (!stick_to_floor) {
            update_settings.mStickToFloorStepDown = JPH::Vec3::sZero();
        } else {
            update_settings.mStickToFloorStepDown = -character->GetUp() * 0.6f;
        }
        update_settings.mWalkStairsStepUp = character->GetUp() * 0.50f;
        update_settings.mWalkStairsMinStepForward = 0.02f;
        update_settings.mWalkStairsStepForwardTest = 0.35f;
        update_settings.mWalkStairsCosAngleForwardContact = JPH::Cos(JPH::DegreesToRadians(80.0f));
        update_settings.mWalkStairsStepDownExtra = -character->GetUp() * 0.20f;

        JoltIgnoreActorBodyFilter body_filter(character->GetUserData(), m_query_filter);

        const auto profileStart = m_profile_enabled ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
        character->ExtendedUpdate(
            delta_time,
            jolt_gravity * gravity_factor,
            update_settings,
            m_physics_system->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
            m_physics_system->GetDefaultLayerFilter(Layers::MOVING),
            body_filter, {}, *m_temp_allocator
        );
        const auto profileContacts = m_profile_enabled ? std::chrono::steady_clock::now() : profileStart;

        auto cb_it = m_character_callbacks.find(handle);
        if (cb_it != m_character_callbacks.end() && cb_it->second.callback) {
            JPH::CollideShapeSettings collide_settings;
            collide_settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

            Fvector vel;
            GetCharacterVirtualVelocity(handle, vel);

            JoltPassableContactCollector passable_collector(
                m_physics_system,
                cb_it->second.callback,
                cb_it->second.user_data,
                vel
            );

            m_physics_system->GetNarrowPhaseQuery().CollideShape(
                character->GetShape(),
                JPH::Vec3::sReplicate(1.0f),
                character->GetCenterOfMassTransform(),
                collide_settings,
                JPH::RVec3::sZero(),
                passable_collector,
                m_physics_system->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                m_physics_system->GetDefaultLayerFilter(Layers::MOVING),
                body_filter,
                JPH::ShapeFilter()
            );

            FlushDeferredContacts();
        }
        if (m_profile_enabled) {
            ++m_profile_character_calls;
            m_profile_character_update_ms += std::chrono::duration<double, std::milli>(profileContacts - profileStart).count();
            m_profile_character_contacts_ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - profileContacts).count();
        }
    }
}

void JoltPhysicsCore::SetCharacterVirtualStickToFloor(CharacterVirtualHandle handle, bool stick_to_floor) {
    if (m_stick_to_floor.find(handle) != m_stick_to_floor.end()) {
        m_stick_to_floor[handle] = stick_to_floor;
    }
}

PhysicsShapeHandle JoltPhysicsCore::CreateRotatedTranslatedShape(PhysicsShapeHandle base_shape, const Fvector& position, const Fquaternion& rotation) {
    if (!base_shape) return nullptr;
    JPH::Shape* inner = reinterpret_cast<JPH::Shape*>(base_shape);
    JPH::Quat q(-rotation.x, -rotation.y, -rotation.z, rotation.w);
    if (q.LengthSq() > 1e-6f) q = q.Normalized();
    else q = JPH::Quat::sIdentity();

    JPH::Vec3 pos(position.x, position.y, position.z);
    if (!_valid(position)) pos = JPH::Vec3::sZero();

    JPH::RotatedTranslatedShapeSettings settings(
        pos,
        q,
        inner
    );
    JPH::ShapeSettings::ShapeResult result = settings.Create();
    if (result.IsValid()) {
        JPH::Shape* shape = result.Get().GetPtr();
        shape->AddRef();
        return reinterpret_cast<PhysicsShapeHandle>(shape);
    }
    return base_shape;
}

RagdollHandle JoltPhysicsCore::CreateRagdoll(const SRagdollSettings& settings) {
    if (!m_physics_system || settings.parts.empty()) return INVALID_RAGDOLL_HANDLE;

    JPH::Ref<JPH::RagdollSettings> jph_settings = new JPH::RagdollSettings();
    jph_settings->mSkeleton = new JPH::Skeleton();

    // 1. Build Skeleton & Parts
    jph_settings->mParts.resize(settings.parts.size());
    for (size_t i = 0; i < settings.parts.size(); ++i) {
        const auto& part = settings.parts[i];

        string64 joint_name;
        xr_sprintf(joint_name, "joint_%zu", i);
        if (part.parent_index != -1 && part.parent_index >= 0) {
            string64 parent_name;
            xr_sprintf(parent_name, "joint_%d", part.parent_index);
            jph_settings->mSkeleton->AddJoint(joint_name, parent_name);
        } else {
            jph_settings->mSkeleton->AddJoint(joint_name);
        }

        auto& jph_part = jph_settings->mParts[i];
        jph_part.SetShape(static_cast<JPH::Shape*>(part.shape));
        jph_part.mPosition = JPH::Vec3(part.position.x, part.position.y, part.position.z);
        JPH::Quat part_rot_raw(part.rotation.x, part.rotation.y, part.rotation.z, part.rotation.w);
        if (part_rot_raw.LengthSq() > 1e-6f) part_rot_raw = part_rot_raw.Normalized();
        else part_rot_raw = JPH::Quat::sIdentity();
        // X-Ray quaternions have inverted xyz (left-handed), convert to Jolt right-handed
        JPH::Quat part_rot(-part_rot_raw.GetX(), -part_rot_raw.GetY(), -part_rot_raw.GetZ(), part_rot_raw.GetW());
        if (part_rot.LengthSq() > 1e-6f) part_rot = part_rot.Normalized();
        else part_rot = JPH::Quat::sIdentity();
        jph_part.mRotation = part_rot;
        jph_part.mFriction = 0.8f;
        jph_part.mRestitution = 0.02f;

        float mass = std::max(part.mass, 0.05f);
        jph_part.mMassPropertiesOverride.mMass = mass;
        jph_part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;

        // Mass-proportional body damping (heavy bones damp oscillations heavily, light limbs stay nimble)
        float mass_factor = std::clamp(mass / 30.0f, 0.0f, 1.0f);
        jph_part.mLinearDamping = 0.05f + mass_factor * 0.20f;
        jph_part.mAngularDamping = 0.15f + mass_factor * 0.45f;

        // Root is kinematic, others are dynamic
        if (part.parent_index == -1) {
            jph_part.mMotionType = JPH::EMotionType::Kinematic;
        } else {
            jph_part.mMotionType = JPH::EMotionType::Dynamic;
        }

        jph_part.mMotionQuality = JPH::EMotionQuality::LinearCast;

        jph_part.mObjectLayer = Layers::RAGDOLL;
    }

    jph_settings->mSkeleton->CalculateParentJointIndices();

    // 2. Build Constraints
    for (const auto& c_desc : settings.constraints) {
        if (c_desc.child_index < 0 || c_desc.child_index >= settings.parts.size()) continue;

        const auto& part_child = settings.parts[c_desc.child_index];
        const auto& part_parent = settings.parts[part_child.parent_index];

        JPH::Ref<JPH::SwingTwistConstraintSettings> constraint = new JPH::SwingTwistConstraintSettings();
        constraint->mSpace = JPH::EConstraintSpace::LocalToBodyCOM;

        JPH::Quat rot_parent_raw(part_parent.rotation.x, part_parent.rotation.y, part_parent.rotation.z, part_parent.rotation.w);
        if (rot_parent_raw.LengthSq() > 1e-6f) rot_parent_raw = rot_parent_raw.Normalized();
        else rot_parent_raw = JPH::Quat::sIdentity();
        JPH::Quat rot_parent(-rot_parent_raw.GetX(), -rot_parent_raw.GetY(), -rot_parent_raw.GetZ(), rot_parent_raw.GetW());
        if (rot_parent.LengthSq() > 1e-6f) rot_parent = rot_parent.Normalized();
        else rot_parent = JPH::Quat::sIdentity();

        JPH::Quat rot_child_raw(part_child.rotation.x, part_child.rotation.y, part_child.rotation.z, part_child.rotation.w);
        if (rot_child_raw.LengthSq() > 1e-6f) rot_child_raw = rot_child_raw.Normalized();
        else rot_child_raw = JPH::Quat::sIdentity();
        JPH::Quat rot_child(-rot_child_raw.GetX(), -rot_child_raw.GetY(), -rot_child_raw.GetZ(), rot_child_raw.GetW());
        if (rot_child.LengthSq() > 1e-6f) rot_child = rot_child.Normalized();
        else rot_child = JPH::Quat::sIdentity();

        JPH::Vec3 pos_parent(part_parent.position.x, part_parent.position.y, part_parent.position.z);
        JPH::Vec3 pos_child(part_child.position.x, part_child.position.y, part_child.position.z);

        JPH::Vec3 parent_com_local = jph_settings->mParts[part_child.parent_index].GetShape()->GetCenterOfMass();
        JPH::Vec3 child_com_local = jph_settings->mParts[c_desc.child_index].GetShape()->GetCenterOfMass();

        // The joint anchor is located at the child bone origin in model space (pos_child).
        // Specify anchor position relative to the center-of-mass for both bodies.
        constraint->mPosition1 = (rot_parent.Conjugated() * (pos_child - pos_parent)) - parent_com_local;
        constraint->mPosition2 = -child_com_local;

        JPH::Vec3 twist_world = rot_child * JPH::Vec3(c_desc.twist_axis.x, c_desc.twist_axis.y, c_desc.twist_axis.z);
        JPH::Vec3 plane_world = rot_child * JPH::Vec3(c_desc.plane_axis.x, c_desc.plane_axis.y, c_desc.plane_axis.z);

        constraint->mTwistAxis1 = (rot_parent.Conjugated() * twist_world).Normalized();
        constraint->mPlaneAxis1 = (rot_parent.Conjugated() * plane_world).Normalized();
        constraint->mTwistAxis2 = JPH::Vec3(c_desc.twist_axis.x, c_desc.twist_axis.y, c_desc.twist_axis.z).Normalized();
        constraint->mPlaneAxis2 = JPH::Vec3(c_desc.plane_axis.x, c_desc.plane_axis.y, c_desc.plane_axis.z).Normalized();

        constraint->mNormalHalfConeAngle = std::clamp(c_desc.swing_limit_z, 0.0f, float(M_PI * 0.5f));
        constraint->mPlaneHalfConeAngle = std::clamp(c_desc.swing_limit_y, 0.0f, float(M_PI * 0.5f));
        constraint->mTwistMinAngle = std::clamp(c_desc.twist_limit_min, -float(M_PI), float(M_PI));
        constraint->mTwistMaxAngle = std::clamp(c_desc.twist_limit_max, -float(M_PI), float(M_PI));
        if (constraint->mTwistMaxAngle < constraint->mTwistMinAngle) {
            std::swap(constraint->mTwistMinAngle, constraint->mTwistMaxAngle);
        }

        // Mass-proportional joint friction
        float joint_friction = std::max(c_desc.max_friction_torque, part_child.mass * 0.4f);
        constraint->mMaxFrictionTorque = std::clamp(joint_friction, 1.0f, 80.0f);

        // Overdamped frequency mode (damping ratio 1.2) to prevent pendulum swing oscillations
        float max_torque = std::clamp(part_child.mass * 15.0f, 50.0f, 500.0f);
        constraint->mSwingMotorSettings = JPH::MotorSettings(JPH::ESpringMode::FrequencyAndDamping, 6.0f, 1.2f, max_torque, max_torque);
        constraint->mTwistMotorSettings = JPH::MotorSettings(JPH::ESpringMode::FrequencyAndDamping, 6.0f, 1.2f, max_torque, max_torque);

        // Attach to part
        jph_settings->mParts[c_desc.child_index].mToParent = constraint;
    }

    std::vector<JPH::Mat44> bind_joint_matrices(settings.parts.size());
    for (size_t i = 0; i < settings.parts.size(); ++i) {
        bind_joint_matrices[i] = JPH::Mat44::sRotationTranslation(jph_settings->mParts[i].mRotation, jph_settings->mParts[i].mPosition);
    }
    jph_settings->DisableParentChildCollisions(bind_joint_matrices.data(), 0.0f);

    RagdollHandle handle = m_next_ragdoll_handle++;
    for (int i = 0; i < (int)settings.parts.size(); ++i) {
        jph_settings->mParts[i].mCollisionGroup.SetGroupID(handle);
        jph_settings->mParts[i].mUserData = 0;
    }

    jph_settings->CalculateConstraintPriorities();
    jph_settings->Stabilize();
    jph_settings->CalculateBodyIndexToConstraintIndex();
    jph_settings->CalculateConstraintIndexToBodyIdxPair();

    JPH::Ragdoll* ragdoll = jph_settings->CreateRagdoll(handle, 0, m_physics_system);
    if (!ragdoll) return INVALID_RAGDOLL_HANDLE;

    // Ensure all spawned ragdoll bodies have 0 user data initially
    for (u32 i = 0; i < ragdoll->GetBodyCount(); ++i) {
        JPH::BodyID id = ragdoll->GetBodyID(i);
        if (!id.IsInvalid()) {
            m_physics_system->GetBodyInterface().SetUserData(id, 0);
        }
    }
    m_ragdolls[handle] = ragdoll;
    m_ragdoll_settings[handle] = jph_settings;

    return handle;
}

void JoltPhysicsCore::DestroyRagdoll(RagdollHandle handle) {
    if (m_ragdolls.find(handle) != m_ragdolls.end()) {
        RemoveRagdollFromWorld(handle);
        m_ragdolls.erase(handle);
        m_ragdoll_settings.erase(handle);
    }
}

void JoltPhysicsCore::AddRagdollToWorld(RagdollHandle handle, bool activate) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        it->second->AddToPhysicsSystem(JPH::EActivation::Activate);
    }
}

void JoltPhysicsCore::RemoveRagdollFromWorld(RagdollHandle handle) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        it->second->RemoveFromPhysicsSystem();
    }
}

void JoltPhysicsCore::SetRagdollTargetPose(RagdollHandle handle, const Fquaternion* target_rotations, u32 count) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && target_rotations) {
        JPH::SkeletonPose target_pose;
        target_pose.SetSkeleton(m_ragdoll_settings[handle]->mSkeleton);

        u32 j_count = (u32)target_pose.GetJointMatrices().size();
        u32 it_count = (count < j_count) ? count : j_count;
        for (u32 i = 0; i < it_count; ++i) {
            target_pose.GetJointMatrices()[i] = JPH::Mat44::sRotation(JPH::Quat(target_rotations[i].x, target_rotations[i].y, target_rotations[i].z, target_rotations[i].w));
        }

        target_pose.CalculateJointStates();
        it->second->DriveToPoseUsingMotors(target_pose);
    }
}

void JoltPhysicsCore::SetRagdollTargetPose(RagdollHandle handle, const Fmatrix* target_matrices, u32 count) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && target_matrices && m_physics_system) {
        JPH::Ragdoll* ragdoll = it->second;

        std::vector<JPH::Mat44> jph_matrices(count);
        for (u32 i = 0; i < count; ++i) {
            const Fmatrix& m = target_matrices[i];
            if (!_valid(m) || !is_valid_pos(m.c)) {
                jph_matrices[i] = JPH::Mat44::sIdentity();
                continue;
            }

            JPH::Vec3 axis_x(m._11, m._12, m._13);
            JPH::Vec3 axis_y(m._21, m._22, m._23);
            JPH::Vec3 axis_z(m._31, m._32, m._33);

            if (axis_x.LengthSq() > 1e-6f) axis_x = axis_x.Normalized();
            else axis_x = JPH::Vec3::sAxisX();

            if (axis_y.LengthSq() > 1e-6f) axis_y = axis_y.Normalized();
            else axis_y = axis_x.GetNormalizedPerpendicular();

            axis_z = axis_x.Cross(axis_y);
            if (axis_z.LengthSq() > 1e-6f) axis_z = axis_z.Normalized();
            else axis_z = JPH::Vec3::sAxisZ();

            axis_y = axis_z.Cross(axis_x).Normalized();

            JPH::Mat44 mat(
                JPH::Vec4(axis_x, 0.0f),
                JPH::Vec4(axis_y, 0.0f),
                JPH::Vec4(axis_z, 0.0f),
                JPH::Vec4(0.0f, 0.0f, 0.0f, 1.0f)
            );
            JPH::Quat q = mat.GetQuaternion().Normalized();
            JPH::Quat jph_q(-q.GetX(), -q.GetY(), -q.GetZ(), q.GetW());
            if (jph_q.LengthSq() > 1e-6f) jph_q = jph_q.Normalized();
            else jph_q = JPH::Quat::sIdentity();

            JPH::Vec3 jph_pos(m.c.x, m.c.y, m.c.z);
            jph_matrices[i] = JPH::Mat44::sRotationTranslation(jph_q, jph_pos);
        }

        // Check if there are kinematic vs dynamic bodies
        bool has_dynamic = false;
        bool has_kinematic = false;
        for (u32 i = 0; i < ragdoll->GetBodyCount(); ++i) {
            JPH::BodyID id = ragdoll->GetBodyID(i);
            if (!id.IsInvalid()) {
                JPH::EMotionType mt = m_physics_system->GetBodyInterface().GetMotionType(id);
                if (mt == JPH::EMotionType::Kinematic) {
                    has_kinematic = true;
                } else if (mt == JPH::EMotionType::Dynamic) {
                    has_dynamic = true;
                }
            }
        }

        // 1. Instantly update kinematic bodies to the target pose without runaway velocity integration
        if (has_kinematic) {
            for (u32 i = 0; i < ragdoll->GetBodyCount() && i < count; ++i) {
                JPH::BodyID id = ragdoll->GetBodyID(i);
                if (!id.IsInvalid() && m_physics_system->GetBodyInterface().GetMotionType(id) == JPH::EMotionType::Kinematic) {
                    m_physics_system->GetBodyInterface().SetPositionAndRotation(
                        id,
                        jph_matrices[i].GetTranslation(),
                        jph_matrices[i].GetQuaternion(),
                        JPH::EActivation::DontActivate
                    );
                }
            }
        }

        // 2. Drive dynamic bodies using motors only if dynamic bodies actually exist (prevents 0/0 divide in constraints)
        if (has_dynamic) {
            JPH::SkeletonPose target_pose;
            target_pose.SetSkeleton(m_ragdoll_settings[handle]->mSkeleton);
            u32 j_count = (u32)target_pose.GetJointMatrices().size();
            u32 it_count = (count < j_count) ? count : j_count;

            const auto& ragdoll_settings = m_ragdoll_settings[handle];
            for (u32 i = 0; i < it_count; ++i) {
                int parent_idx = ragdoll_settings->mSkeleton->GetJoint(i).mParentJointIndex;
                if (parent_idx == -1 || parent_idx >= (int)count) {
                    target_pose.GetJointMatrices()[i] = jph_matrices[i];
                } else {
                    JPH::Mat44 parent_inv = jph_matrices[parent_idx].InversedRotationTranslation();
                    target_pose.GetJointMatrices()[i] = parent_inv * jph_matrices[i];
                }
            }

            target_pose.CalculateJointStates();
            ragdoll->DriveToPoseUsingMotors(target_pose);
        }
    }
}

static inline void MatrixMul43(Fmatrix& dest, const Fmatrix& A, const Fmatrix& B) {
    dest.m[0][0] = A.m[0][0] * B.m[0][0] + A.m[1][0] * B.m[0][1] + A.m[2][0] * B.m[0][2];
    dest.m[0][1] = A.m[0][1] * B.m[0][0] + A.m[1][1] * B.m[0][1] + A.m[2][1] * B.m[0][2];
    dest.m[0][2] = A.m[0][2] * B.m[0][0] + A.m[1][2] * B.m[0][1] + A.m[2][2] * B.m[0][2];
    dest.m[0][3] = 0.0f;

    dest.m[1][0] = A.m[0][0] * B.m[1][0] + A.m[1][0] * B.m[1][1] + A.m[2][0] * B.m[1][2];
    dest.m[1][1] = A.m[0][1] * B.m[1][0] + A.m[1][1] * B.m[1][1] + A.m[2][1] * B.m[1][2];
    dest.m[1][2] = A.m[0][2] * B.m[1][0] + A.m[1][2] * B.m[1][1] + A.m[2][2] * B.m[1][2];
    dest.m[1][3] = 0.0f;

    dest.m[2][0] = A.m[0][0] * B.m[2][0] + A.m[1][0] * B.m[2][1] + A.m[2][0] * B.m[2][2];
    dest.m[2][1] = A.m[0][1] * B.m[2][0] + A.m[1][1] * B.m[2][1] + A.m[2][1] * B.m[2][2];
    dest.m[2][2] = A.m[0][2] * B.m[2][0] + A.m[1][2] * B.m[2][1] + A.m[2][2] * B.m[2][2];
    dest.m[2][3] = 0.0f;

    dest.m[3][0] = A.m[0][0] * B.m[3][0] + A.m[1][0] * B.m[3][1] + A.m[2][0] * B.m[3][2] + A.m[3][0];
    dest.m[3][1] = A.m[0][1] * B.m[3][0] + A.m[1][1] * B.m[3][1] + A.m[2][1] * B.m[3][2] + A.m[3][1];
    dest.m[3][2] = A.m[0][2] * B.m[3][0] + A.m[1][2] * B.m[3][1] + A.m[2][2] * B.m[3][2] + A.m[3][2];
    dest.m[3][3] = 1.0f;
}

void JoltPhysicsCore::SetRagdollWorldPose(RagdollHandle handle, const Fmatrix& world_transform, const Fmatrix* bone_model_matrices, u32 count) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system && bone_model_matrices) {
        JPH::Ragdoll* ragdoll = it->second;

        JPH::RVec3 root_offset(world_transform.c.x, world_transform.c.y, world_transform.c.z);

        std::vector<JPH::Mat44> jph_matrices(count);
        for (u32 i = 0; i < count; ++i) {
            Fmatrix bone_world;
            MatrixMul43(bone_world, world_transform, bone_model_matrices[i]);

            R_ASSERT2(_valid(bone_world) && is_valid_pos(bone_world.c), "SetRagdollWorldPose: input bone matrix is invalid!");

            JPH::Vec3 axis_x(bone_world._11, bone_world._12, bone_world._13);
            JPH::Vec3 axis_y(bone_world._21, bone_world._22, bone_world._23);
            JPH::Vec3 axis_z(bone_world._31, bone_world._32, bone_world._33);

            if (axis_x.LengthSq() > 1e-6f) axis_x = axis_x.Normalized();
            else axis_x = JPH::Vec3::sAxisX();

            if (axis_y.LengthSq() > 1e-6f) axis_y = axis_y.Normalized();
            else axis_y = axis_x.GetNormalizedPerpendicular();

            axis_z = axis_x.Cross(axis_y);
            if (axis_z.LengthSq() > 1e-6f) axis_z = axis_z.Normalized();
            else axis_z = JPH::Vec3::sAxisZ();

            axis_y = axis_z.Cross(axis_x).Normalized();

            jph_matrices[i] = JPH::Mat44(
                JPH::Vec4(axis_x, 0.0f),
                JPH::Vec4(axis_y, 0.0f),
                JPH::Vec4(axis_z, 0.0f),
                JPH::Vec4(bone_world._41 - root_offset.GetX(), bone_world._42 - root_offset.GetY(), bone_world._43 - root_offset.GetZ(), 1.0f)
            );
        }

        ragdoll->SetPose(root_offset, jph_matrices.data());
        ragdoll->ResetWarmStart();
        ragdoll->SetLinearAndAngularVelocity(JPH::Vec3::sZero(), JPH::Vec3::sZero());

        // Проверка: все ли тела рэгдолла находятся в адекватных координатах прямо после SetPose
        for (u32 i = 0; i < ragdoll->GetBodyCount(); ++i) {
            JPH::BodyID id = ragdoll->GetBodyID(i);
            if (!id.IsInvalid()) {
                JPH::RVec3 p = m_physics_system->GetBodyInterface().GetPosition(id);
                Fvector pos; pos.set(p.GetX(), p.GetY(), p.GetZ());
                R_ASSERT2(is_valid_pos(pos), "SetRagdollWorldPose: body position exploded after SetPose!");
            }
        }
    }
}

void JoltPhysicsCore::SetRagdollRootKinematic(RagdollHandle handle, bool kinematic) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::BodyID root_id = it->second->GetBodyID(0);
        if (!root_id.IsInvalid()) {
            m_physics_system->GetBodyInterface().SetMotionType(root_id, kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic, JPH::EActivation::Activate);
        }
    }
}

void JoltPhysicsCore::SetRagdollRootTransform(RagdollHandle handle, const Fvector& position, const Fquaternion& rotation) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::BodyID root_id = it->second->GetBodyID(0);
        if (!root_id.IsInvalid()) {
            JPH::Quat q(rotation.x, rotation.y, rotation.z, rotation.w);
            if (q.LengthSq() > 1e-6f) q = q.Normalized();
            else q = JPH::Quat::sIdentity();

            m_physics_system->GetBodyInterface().SetPositionAndRotation(root_id, JPH::Vec3(position.x, position.y, position.z), q, JPH::EActivation::DontActivate);
        }
    }
}

void JoltPhysicsCore::SetRagdollMotorState(RagdollHandle handle, bool enabled) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end()) {
        JPH::EMotorState state = enabled ? JPH::EMotorState::Position : JPH::EMotorState::Off;
        for (u32 i = 0; i < it->second->GetConstraintCount(); ++i) {
            JPH::TwoBodyConstraint* c = it->second->GetConstraint(i);
            if (c && c->GetSubType() == JPH::EConstraintSubType::SwingTwist) {
                JPH::SwingTwistConstraint* st = static_cast<JPH::SwingTwistConstraint*>(c);
                st->SetSwingMotorState(state);
                st->SetTwistMotorState(state);
            }
        }
    }
}

void JoltPhysicsCore::SetRagdollConstraintMotorState(RagdollHandle handle, u32 constraint_index, bool enabled) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end()) {
        if (constraint_index < it->second->GetConstraintCount()) {
            JPH::TwoBodyConstraint* c = it->second->GetConstraint(constraint_index);
            if (c && c->GetSubType() == JPH::EConstraintSubType::SwingTwist) {
                JPH::SwingTwistConstraint* st = static_cast<JPH::SwingTwistConstraint*>(c);
                JPH::EMotorState state = enabled ? JPH::EMotorState::Position : JPH::EMotorState::Off;
                st->SetSwingMotorState(state);
                st->SetTwistMotorState(state);
            }
        }
    }
}

void JoltPhysicsCore::SetRagdollMotorStiffness(RagdollHandle handle, float stiffness) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end()) {
        for (u32 i = 0; i < it->second->GetConstraintCount(); ++i) {
            JPH::TwoBodyConstraint* c = it->second->GetConstraint(i);
            if (c && c->GetSubType() == JPH::EConstraintSubType::SwingTwist) {
                JPH::SwingTwistConstraint* st = static_cast<JPH::SwingTwistConstraint*>(c);
                st->GetSwingMotorSettings().mSpringSettings.mStiffness = stiffness;
                st->GetTwistMotorSettings().mSpringSettings.mStiffness = stiffness;
                st->GetSwingMotorSettings().SetTorqueLimits(-250.0f, 250.0f);
                st->GetTwistMotorSettings().SetTorqueLimits(-250.0f, 250.0f);
            }
        }
    }
}

void JoltPhysicsCore::SetRagdollMotorDamping(RagdollHandle handle, float damping) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end()) {
        for (u32 i = 0; i < it->second->GetConstraintCount(); ++i) {
            JPH::TwoBodyConstraint* c = it->second->GetConstraint(i);
            if (c && c->GetSubType() == JPH::EConstraintSubType::SwingTwist) {
                JPH::SwingTwistConstraint* st = static_cast<JPH::SwingTwistConstraint*>(c);
                st->GetSwingMotorSettings().mSpringSettings.mDamping = damping;
                st->GetTwistMotorSettings().mSpringSettings.mDamping = damping;
                st->GetSwingMotorSettings().SetTorqueLimits(-250.0f, 250.0f);
                st->GetTwistMotorSettings().SetTorqueLimits(-250.0f, 250.0f);
            }
        }
    }
}

void JoltPhysicsCore::SetRagdollConstraintMotor(RagdollHandle handle, u32 constraint_index, float stiffness, float damping) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end()) {
        if (constraint_index < it->second->GetConstraintCount()) {
            JPH::TwoBodyConstraint* c = it->second->GetConstraint(constraint_index);
            if (c && c->GetSubType() == JPH::EConstraintSubType::SwingTwist) {
                JPH::SwingTwistConstraint* st = static_cast<JPH::SwingTwistConstraint*>(c);
                st->GetSwingMotorSettings().mSpringSettings.mStiffness = stiffness;
                st->GetSwingMotorSettings().mSpringSettings.mDamping = damping;
                st->GetTwistMotorSettings().mSpringSettings.mStiffness = stiffness;
                st->GetTwistMotorSettings().mSpringSettings.mDamping = damping;
                st->GetSwingMotorSettings().SetTorqueLimits(-250.0f, 250.0f);
                st->GetTwistMotorSettings().SetTorqueLimits(-250.0f, 250.0f);
            }
        }
    }
}

void JoltPhysicsCore::SetRagdollPartMotor(RagdollHandle handle, u32 part_index, float stiffness, float damping) {
    auto it_settings = m_ragdoll_settings.find(handle);
    auto it_ragdoll = m_ragdolls.find(handle);
    if (it_settings != m_ragdoll_settings.end() && it_ragdoll != m_ragdolls.end()) {
        const auto& b2c = it_settings->second->GetBodyIndexToConstraintIndex();
        if (part_index < b2c.size()) {
            int constraint_idx = it_settings->second->GetConstraintIndexForBodyIndex((int)part_index);
            if (constraint_idx >= 0 && (size_t)constraint_idx < it_ragdoll->second->GetConstraintCount()) {
                SetRagdollConstraintMotor(handle, (u32)constraint_idx, stiffness, damping);
            }
        }
    }
}

void JoltPhysicsCore::SetRagdollPartMotionType(RagdollHandle handle, u32 part_index, bool kinematic) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::BodyID body_id = it->second->GetBodyID(part_index);
        if (!body_id.IsInvalid()) {
            m_physics_system->GetBodyInterface().SetMotionType(
                body_id,
                kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic,
                JPH::EActivation::Activate
            );
        }
    }
}

void JoltPhysicsCore::SetRagdollAllPartsKinematic(RagdollHandle handle, bool kinematic) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::EMotionType motion_type = kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
        for (u32 i = 0; i < it->second->GetBodyCount(); ++i) {
            JPH::BodyID body_id = it->second->GetBodyID(i);
            if (!body_id.IsInvalid()) {
                m_physics_system->GetBodyInterface().SetMotionType(body_id, motion_type, JPH::EActivation::Activate);
                if (!kinematic) {
                    m_physics_system->GetBodyInterface().SetLinearAndAngularVelocity(body_id, JPH::Vec3::sZero(), JPH::Vec3::sZero());
                }
            }
        }
        if (!kinematic) {
            it->second->ResetWarmStart();
        }
    }
}

void JoltPhysicsCore::SetRagdollUserData(RagdollHandle handle, void* user_data) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::uint64 ud = reinterpret_cast<JPH::uint64>(user_data);
        for (u32 i = 0; i < it->second->GetBodyCount(); ++i) {
            JPH::BodyID id = it->second->GetBodyID(i);
            if (!id.IsInvalid()) {
                m_physics_system->GetBodyInterface().SetUserData(id, ud);
            }
        }
    }
}

void JoltPhysicsCore::SetRagdollGroupID(RagdollHandle handle, u32 group_id) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end()) {
        it->second->SetGroupID(static_cast<JPH::CollisionGroup::GroupID>(group_id));
    }
}

void JoltPhysicsCore::SetRagdollPartTransform(RagdollHandle handle, u32 part_index, const Fvector& position, const Fquaternion& rotation) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::BodyID body_id = it->second->GetBodyID(part_index);
        if (!body_id.IsInvalid()) {
            JPH::Quat q(rotation.x, rotation.y, rotation.z, rotation.w);
            if (q.LengthSq() > 1e-6f) q = q.Normalized();
            else q = JPH::Quat::sIdentity();

            m_physics_system->GetBodyInterface().SetPositionAndRotation(
                body_id,
                JPH::Vec3(position.x, position.y, position.z),
                q,
                JPH::EActivation::DontActivate
            );
        }
    }
}

void JoltPhysicsCore::ApplyRagdollImpulse(RagdollHandle handle, u32 part_index, const Fvector& impulse, const Fvector& point) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::BodyID body_id = it->second->GetBodyID(part_index);
        if (!body_id.IsInvalid() &&
            m_physics_system->GetBodyInterface().GetMotionType(body_id) == JPH::EMotionType::Dynamic) {
            m_physics_system->GetBodyInterface().ActivateBody(body_id);
            m_physics_system->GetBodyInterface().AddImpulse(
                body_id,
                JPH::Vec3(impulse.x, impulse.y, impulse.z),
                JPH::Vec3(point.x, point.y, point.z)
            );
        }
    }
}

void JoltPhysicsCore::ApplyRagdollLinearImpulse(RagdollHandle handle, u32 part_index, const Fvector& impulse) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::BodyID body_id = it->second->GetBodyID(part_index);
        if (!body_id.IsInvalid() &&
            m_physics_system->GetBodyInterface().GetMotionType(body_id) == JPH::EMotionType::Dynamic) {
            m_physics_system->GetBodyInterface().ActivateBody(body_id);
            m_physics_system->GetBodyInterface().AddImpulse(
                body_id,
                JPH::Vec3(impulse.x, impulse.y, impulse.z)
            );
        }
    }
}

void JoltPhysicsCore::GetRagdollPartTransform(RagdollHandle handle, u32 part_index, Fvector& out_position, Fquaternion& out_rotation) const {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::BodyID body_id = it->second->GetBodyID(part_index);
        if (!body_id.IsInvalid()) {
            JPH::Vec3 pos = m_physics_system->GetBodyInterface().GetPosition(body_id);
            JPH::Quat rot = m_physics_system->GetBodyInterface().GetRotation(body_id);
            out_position.set(pos.GetX(), pos.GetY(), pos.GetZ());
            out_rotation.set(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());
        }
    }
}

void JoltPhysicsCore::GetRagdollAllTransforms(RagdollHandle handle, Fmatrix* out_matrices, u32 count) const {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        u32 max_parts = std::min(count, (u32)it->second->GetBodyCount());
        for (u32 i = 0; i < max_parts; ++i) {
            JPH::BodyID body_id = it->second->GetBodyID(i);
            if (!body_id.IsInvalid()) {
                JPH::Mat44 jph_mat = m_physics_system->GetBodyInterface().GetWorldTransform(body_id);
                Fmatrix& mat = out_matrices[i];

                JPH::Vec3 axis_x = jph_mat.GetAxisX();
                JPH::Vec3 axis_y = jph_mat.GetAxisY();
                JPH::Vec3 axis_z = jph_mat.GetAxisZ();
                JPH::Vec3 pos = jph_mat.GetTranslation();

                mat.i.set(axis_x.GetX(), axis_x.GetY(), axis_x.GetZ());
                mat.j.set(axis_y.GetX(), axis_y.GetY(), axis_y.GetZ());
                mat.k.set(axis_z.GetX(), axis_z.GetY(), axis_z.GetZ());
                mat.c.set(pos.GetX(), pos.GetY(), pos.GetZ());

                mat._14_ = 0.0f; mat._24_ = 0.0f; mat._34_ = 0.0f; mat._44_ = 1.0f;
            }
        }
    }
}

void JoltPhysicsCore::GetRagdollPartVelocity(RagdollHandle handle, u32 part_index, Fvector& out_linear_vel, Fvector& out_angular_vel) const {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        JPH::BodyID body_id = it->second->GetBodyID(part_index);
        if (!body_id.IsInvalid()) {
            JPH::Vec3 l_vel = m_physics_system->GetBodyInterface().GetLinearVelocity(body_id);
            JPH::Vec3 a_vel = m_physics_system->GetBodyInterface().GetAngularVelocity(body_id);
            out_linear_vel.set(l_vel.GetX(), l_vel.GetY(), l_vel.GetZ());
            out_angular_vel.set(a_vel.GetX(), a_vel.GetY(), a_vel.GetZ());
            return;
        }
    }
    out_linear_vel.set(0.f, 0.f, 0.f);
    out_angular_vel.set(0.f, 0.f, 0.f);
}

float JoltPhysicsCore::GetRagdollTotalEnergy(RagdollHandle handle) const {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end() && m_physics_system) {
        float total_energy = 0.0f;
        for (u32 i = 0; i < it->second->GetBodyCount(); ++i) {
            JPH::BodyID body_id = it->second->GetBodyID(i);
            if (!body_id.IsInvalid() && m_physics_system->GetBodyInterface().IsActive(body_id)) {
                JPH::Vec3 v = m_physics_system->GetBodyInterface().GetLinearVelocity(body_id);
                JPH::Vec3 w = m_physics_system->GetBodyInterface().GetAngularVelocity(body_id);
                total_energy += v.LengthSq() + w.LengthSq();
            }
        }
        return total_energy;
    }
    return 0.0f;
}

u32 JoltPhysicsCore::GetRagdollPartCount(RagdollHandle handle) const {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end()) {
        return (u32)it->second->GetBodyCount();
    }
    return 0;
}

void JoltPhysicsCore::SetRagdollCollisionGroup(RagdollHandle handle, u32 group_id) {
    auto it = m_ragdolls.find(handle);
    if (it != m_ragdolls.end()) {
        it->second->SetGroupID(group_id);
    }
}

static JoltPhysicsCore g_physics_core;

bool GetPhysicsBodyState(BodyHandle body, NativeBodyState& state) {
    return g_physics_core.ReadBodyState(body, state);
}

extern "C" PHYSICS_CORE_API IPhysicsCore* GetPhysicsCore() {
    g_physics_core.Initialize();
    return &g_physics_core;
}

void JoltPhysicsCore::GetCharacterVirtualAABB(CharacterVirtualHandle handle, Fvector& center, Fvector& half_extents) const {
    auto it = m_characters.find(handle);
    if (it != m_characters.end()) {
        const JPH::Shape* shape = it->second->GetShape();
        if (shape) {
            JPH::AABox bounds = shape->GetLocalBounds();
            JPH::Mat44 transform = it->second->GetCenterOfMassTransform().ToMat44();
            JPH::AABox world_bounds = bounds.Transformed(transform);
            JPH::Vec3 jph_center = world_bounds.GetCenter();
            JPH::Vec3 jph_extents = world_bounds.GetExtent();
            center.set(jph_center.GetX(), jph_center.GetY(), jph_center.GetZ());
            half_extents.set(jph_extents.GetX(), jph_extents.GetY(), jph_extents.GetZ());
            return;
        }
    }
    center.set(0.f, 0.f, 0.f);
    half_extents.set(0.f, 0.f, 0.f);
}

void JoltPhysicsCore::SetCharacterVirtualGravityFactor(CharacterVirtualHandle handle, float factor) {
    if (m_character_gravity_factors.find(handle) != m_character_gravity_factors.end() || factor != 1.0f) {
        m_character_gravity_factors[handle] = factor;
    }
}

float JoltPhysicsCore::GetCharacterVirtualGravityFactor(CharacterVirtualHandle handle) const {
    const auto found = m_character_gravity_factors.find(handle);
    return found == m_character_gravity_factors.end() ? 1.0f : found->second;
}

class PlacementCollector final : public JPH::CollideShapeCollector {
public:
    void AddHit(const JPH::CollideShapeResult& hit) override {
        if (hit.mPenetrationDepth <= .002f) return;
        const auto* context = GetContext();
        if (context && context->mShape) {
            const auto material = GetMaterialIndexForSubShape(context->mShape.GetPtr(),
                context->MakeSubShapeIDRelativeToShape(hit.mSubShapeID2));
            if (material != GAMEMTL_NONE_IDX && material < GMLib.CountMaterial() &&
                IsPassableMaterial(GMLib.GetMaterialByIdx(material))) return;
        }
        blocked = true;
        ForceEarlyOut();
    }
    bool blocked = false;
};

bool JoltPhysicsCore::CheckShapePlacement(PhysicsShapeHandle shape, const Fvector& pos, const Fquaternion& rot, bool check_characters, void* ignore_user_data, bool camera) const {
    if (!shape || !m_physics_system) return true;

    const JPH::Shape* jolt_shape = reinterpret_cast<const JPH::Shape*>(shape);
    JPH::Quat jph_rot(rot.x, rot.y, rot.z, rot.w);
    JPH::RMat44 com_transform = JPH::RMat44::sRotationTranslation(jph_rot,
        JPH::RVec3(pos.x, pos.y, pos.z) + jph_rot * jolt_shape->GetCenterOfMass());

    JPH::CollideShapeSettings settings;
    settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::uint64 actor_user_data = reinterpret_cast<JPH::uint64>(ignore_user_data);
    PlacementCollector collector;
    JoltIgnoreActorBodyFilter body_filter(actor_user_data, m_query_filter, camera);

    m_physics_system->GetNarrowPhaseQuery().CollideShape(
        jolt_shape,
        JPH::Vec3::sReplicate(1.0f),
        com_transform,
        settings,
        JPH::RVec3(pos.x, pos.y, pos.z),
        collector,
        m_physics_system->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        m_physics_system->GetDefaultLayerFilter(Layers::MOVING),
        body_filter,
        JPH::ShapeFilter()
    );

    if (collector.blocked) {
        return false;
    }

    if (check_characters) {
        JPH::AABox check_bounds = jolt_shape->GetWorldSpaceBounds(com_transform.ToMat44(), JPH::Vec3::sOne());
        for (const auto& [handle, char_ptr] : m_characters) {
            if (!char_ptr || m_inactive_characters.contains(handle)) continue;
            if (actor_user_data != 0 && char_ptr->GetUserData() == actor_user_data) continue;
            if (m_query_filter && !m_query_filter(reinterpret_cast<void*>(char_ptr->GetUserData()),
                7, ignore_user_data, camera)) continue;

            const JPH::Shape* other_shape = char_ptr->GetShape();
            if (!other_shape) continue;

            JPH::Mat44 other_transform = char_ptr->GetCenterOfMassTransform().ToMat44();
            JPH::AABox other_bounds = other_shape->GetWorldSpaceBounds(other_transform, JPH::Vec3::sOne());
            if (check_bounds.Overlaps(other_bounds)) {
                // Perform detailed shape vs shape collision check
                PlacementCollector char_collector;
                JPH::CollisionDispatch::sCollideShapeVsShape(
                    jolt_shape, other_shape,
                    JPH::Vec3::sReplicate(1.0f), JPH::Vec3::sReplicate(1.0f),
                    com_transform.ToMat44(), other_transform,
                    JPH::SubShapeIDCreator(), JPH::SubShapeIDCreator(),
                    settings, char_collector, JPH::ShapeFilter()
                );
                if (char_collector.blocked) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool JoltPhysicsCore::FindFreeShapePlacement(PhysicsShapeHandle shape, const Fvector& start_pos, const Fquaternion& rot, Fvector& out_pos, float search_radius, int samples, bool check_characters, void* ignore_user_data, bool camera) const {
    if (!shape || !m_physics_system) {
        out_pos = start_pos;
        return true;
    }

    // First check start position directly
    if (CheckShapePlacement(shape, start_pos, rot, check_characters, ignore_user_data, camera)) {
        out_pos = start_pos;
        return true;
    }

    // Try nearby positions in every direction, including above a penetrated
    // floor. Every candidate is bounded by the caller's displacement limit.
    const int rings = std::clamp(samples, 8, 32);
    const float radius_step = _max(0.f, search_radius) / float(rings);
    for (int r = 1; r <= rings; ++r) {
        float current_r = r * radius_step;
        for (int y = 1; y >= -1; --y) for (int x = -1; x <= 1; ++x) for (int z = -1; z <= 1; ++z) {
            if (x == 0 && y == 0 && z == 0) continue;
            const float scale = current_r / std::sqrt(float(x*x + y*y + z*z));
            Fvector candidate_pos = Fvector().set(start_pos).add(Fvector().set(x*scale, y*scale, z*scale));
            if (CheckShapePlacement(shape, candidate_pos, rot, check_characters, ignore_user_data, camera)) {
                out_pos = candidate_pos;
                return true;
            }
        }
    }

    out_pos = start_pos;
    return false;
}
