#pragma once
#include "xrPhysicsCore/IPhysicsCore.h"
#include "BodyState.h"
#include "JoltLayers.h"
#include "JoltDebugRenderer.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

#include <unordered_map>
#include <atomic>
#include <unordered_set>
#include <mutex>
#include <array>

class JoltPhysicsCore : public IPhysicsCore {
private:
    JPH::PhysicsSystem* m_physics_system = nullptr;
    JPH::TempAllocatorImpl* m_temp_allocator = nullptr;
    JPH::JobSystemThreadPool* m_job_system = nullptr;
    u32 m_worker_count = 0;
    PhysicsCoreStatistics m_step_statistics;
    bool m_profile_enabled = false;
    u32 m_profile_steps = 0;
    enum class WakeSource { Explicit, Motor, Limit, Force, Torque, LinearVelocity, AngularVelocity, Count };
    std::array<u64, static_cast<size_t>(WakeSource::Count)> m_profile_wake_calls{}, m_profile_wake_sleeping{};
    std::array<double, 8> m_profile_times{};
    double m_profile_character_update_ms = 0, m_profile_character_contacts_ms = 0;
    u64 m_profile_character_calls = 0;
    u64 m_profile_motor_brakes = 0, m_profile_motor_drives = 0, m_profile_motor_invalid = 0;
    void ProfileWake(WakeSource source, JPH::BodyID body);
    struct CollisionOwner { u32 group, references; };
    std::unordered_map<void*, CollisionOwner> m_collision_owners;
    std::unordered_map<BodyHandle, void*> m_body_owners;
    JPH::Ref<JPH::GroupFilterTable> m_owner_filter = new JPH::GroupFilterTable(1);
    u32 m_next_owner_group = 0;
    void (*m_pre_integration_callback)() = nullptr;
    std::atomic<u32> m_contact_points{0};
    std::unordered_map<JointHandle, SPhysicsJointFeedback*> m_joint_feedback;
    float m_step_time = 0.025f;
    struct JointSpring { int axis; float erp, cfm; };
    std::unordered_map<JointHandle, std::vector<JointSpring>> m_joint_springs;
    struct JointMotor { int axis; float force, velocity; };
    struct JointLimit { int axis; float low, high; };
    std::unordered_map<JointHandle, std::vector<JointMotor>> m_joint_motors;
    std::unordered_map<JointHandle, std::vector<JointLimit>> m_joint_limits;
    std::unordered_set<JointHandle> m_wheel_joints;
    struct FeedbackFrame {
        JPH::Mat44 first, second;
        JPH::Vec3 lever1, lever2;
    };
    std::unordered_map<JointHandle, FeedbackFrame> m_feedback_frames;
#ifdef JPH_DEBUG_RENDERER
    CXRayJoltDebugRenderer* m_debug_renderer = nullptr;
#endif
    u32 m_debug_draw_flags = 0;
    float m_debug_draw_distance = 100.0f;
    std::unordered_map<JPH::BodyID, bool> m_ignore_static_bodies;

    BPLayerInterfaceImpl m_broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl m_object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl m_object_vs_object_layer_filter;

    std::unordered_map<JPH::BodyID, std::vector<JPH::BodyID>> m_connected_bodies;

    class MyContactListener : public JPH::ContactListener {
        JoltPhysicsCore* m_core;
    public:
        MyContactListener(JoltPhysicsCore* core) : m_core(core) {}

        virtual JPH::ValidateResult OnContactValidate(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            JPH::RVec3Arg inBaseOffset,
            const JPH::CollideShapeResult& inCollisionResult) override;

        virtual void OnContactAdded(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings) override;

        virtual void OnContactPersisted(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings) override;
    };

    MyContactListener m_contact_listener{this};

    class MyBodyActivationListener : public JPH::BodyActivationListener {
        JoltPhysicsCore* m_core;
    public:
        MyBodyActivationListener(JoltPhysicsCore* core) : m_core(core) {}

        virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;
        virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override;
    };

    MyBodyActivationListener m_body_activation_listener{this};
    BodyActivationCallbackFun m_body_activation_callback = nullptr;
    RigidBodyContactCallbackFun m_rb_contact_callback = nullptr;
    BodyContactPolicyFun m_body_contact_policy_callback = nullptr;
    void (*m_deferred_rb_contact_callback)(const NativePhysicsContact&) = nullptr;
    struct ContactBody {
        JPH::BodyID id;
        NativeBodyContactPolicy policy;
        JPH::AABox bounds;
        JPH::AABox fluid_bounds;
        bool fluid_mesh = false;
        bool static_body = false;
    };
    std::vector<ContactBody> m_contact_bodies;
    JPH::BodyIDVector m_query_bodies, m_active_query_bodies;
    JPH::BodyIDVector m_preparation_targets;
    std::vector<u64> m_fluid_preparation_pairs;
    // Caller-thread cache of mesh-local regions proven to contain no fluid.
    struct FluidExclusion {
        JPH::RefConst<JPH::Shape> shape;
        JPH::AABox bounds;
    };
    std::unordered_map<u64, FluidExclusion> m_fluid_exclusions;
    std::vector<u16> m_slowdown_materials, m_current_slowdown_materials;
    std::vector<NativePhysicsContact> m_prepared_contacts;
    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> m_preparation_hits;
    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> m_preparation_sweeps;
    std::vector<NativePhysicsContact> m_deferred_rb_contacts;
    std::mutex m_deferred_rb_mutex;
    bool NeedsContactPreparation(JPH::BodyID first, JPH::BodyID second) const;
    bool NeedsPreparedContact(const NativePhysicsContact& contact) const;
    void QueueDeferredContact(const JPH::Body& first, const JPH::Body& second,
        JPH::SubShapeID shape1, JPH::SubShapeID shape2, JPH::RVec3Arg point,
        JPH::Vec3Arg normal, float depth);
    void FlushDeferredRigidBodyContacts();
    CollisionFilterFun m_collision_filter = nullptr;
    QueryFilterFun m_query_filter = nullptr;
    struct ContactKey {
        u64 bodies, shapes;
        bool operator==(const ContactKey&) const = default;
    };
    struct ContactKeyHash {
        size_t operator()(const ContactKey& key) const { return std::hash<u64>{}(key.bodies) ^ std::hash<u64>{}(key.shapes); }
    };
    std::unordered_set<ContactKey, ContactKeyHash> m_rejected_contacts;
    struct ContactPolicy { float friction, scale; bool static_lower, static_higher; };
    std::unordered_map<ContactKey, ContactPolicy, ContactKeyHash> m_contact_friction;
    std::unordered_set<BodyHandle> m_contact_feedback_bodies;
    std::unordered_map<BodyHandle, std::vector<PhysicsContactForce>> m_contact_forces;
    std::mutex m_contact_force_mutex;
    static void CollectContactImpulse(void* context, const JPH::Body& first, const JPH::SubShapeID& shape1,
        const JPH::Body& second, const JPH::SubShapeID& shape2, JPH::RVec3Arg position1,
        JPH::RVec3Arg position2, JPH::Vec3Arg impulse2, JPH::Vec3Arg angularImpulse2);
    static ContactKey Key(BodyHandle first, BodyHandle second, u32 shape1, u32 shape2);
    void PrepareContacts(float delta_time);

    class MyCharacterContactListener : public JPH::CharacterContactListener {
        JoltPhysicsCore* m_core;
    public:
        MyCharacterContactListener(JoltPhysicsCore* core) : m_core(core) {}

        virtual bool OnContactValidate(
            const JPH::CharacterVirtual* inCharacter,
            const JPH::CharacterContact& inContact) override;
        bool OnCharacterContactValidate(const JPH::CharacterVirtual* character,
            const JPH::CharacterContact& contact) override;

        virtual void OnContactAdded(
            const JPH::CharacterVirtual* inCharacter,
            const JPH::CharacterContact& inContact,
            JPH::CharacterContactSettings& ioSettings) override;

        virtual void OnContactPersisted(
            const JPH::CharacterVirtual* inCharacter,
            const JPH::CharacterContact& inContact,
            JPH::CharacterContactSettings& ioSettings) override;

        virtual void OnCharacterContactAdded(
            const JPH::CharacterVirtual* inCharacter,
            const JPH::CharacterContact& inContact,
            JPH::CharacterContactSettings& ioSettings) override;

        virtual void OnCharacterContactPersisted(
            const JPH::CharacterVirtual* inCharacter,
            const JPH::CharacterContact& inContact,
            JPH::CharacterContactSettings& ioSettings) override;

        virtual void OnCharacterContactSolve(
            const JPH::CharacterVirtual* inCharacter,
            const JPH::CharacterVirtual* inOtherCharacter,
            const JPH::SubShapeID& inSubShapeID2,
            JPH::RVec3Arg inContactPosition,
            JPH::Vec3Arg inContactNormal,
            JPH::Vec3Arg inContactVelocity,
            const JPH::PhysicsMaterial* inContactMaterial,
            JPH::Vec3Arg inCharacterVelocity,
            JPH::Vec3& ioNewCharacterVelocity) override;

        void ProcessContact(
            const JPH::CharacterVirtual* inCharacter,
            const JPH::CharacterContact& inContact);
    };
    MyCharacterContactListener m_character_contact_listener{this};

    std::unordered_map<JointHandle, JPH::Ref<JPH::Constraint>> m_constraints;
    JointHandle m_next_joint_handle = 1;

    std::unordered_map<CharacterVirtualHandle, JPH::Ref<JPH::CharacterVirtual>> m_characters;
    std::unordered_set<CharacterVirtualHandle> m_inactive_characters;
    class CharacterPairCollision final : public JPH::CharacterVsCharacterCollisionSimple {
        JoltPhysicsCore* m_core;
    public:
        explicit CharacterPairCollision(JoltPhysicsCore* core) : m_core(core) {}
        void CollideCharacter(const JPH::CharacterVirtual*, JPH::RMat44Arg, const JPH::CollideShapeSettings&,
            JPH::RVec3Arg, JPH::CollideShapeCollector&) const override;
        void CastCharacter(const JPH::CharacterVirtual*, JPH::RMat44Arg, JPH::Vec3Arg,
            const JPH::ShapeCastSettings&, JPH::RVec3Arg, JPH::CastShapeCollector&) const override;
        const JPH::Shape* PairShape(const JPH::CharacterVirtual*, const JPH::CharacterVirtual*) const;
    };
    CharacterPairCollision m_char_vs_char_collision{this};
    CharacterPairShapeFun m_character_pair_shape = nullptr;
    std::unordered_map<CharacterVirtualHandle, bool> m_stick_to_floor;
    std::unordered_map<CharacterVirtualHandle, float> m_character_gravity_factors;
    struct CharacterCallbackInfo {
        CharacterContactCallbackFun callback;
        void* user_data;
    };
    std::unordered_map<CharacterVirtualHandle, CharacterCallbackInfo> m_character_callbacks;
    CharacterVirtualHandle m_next_character_handle = 1;

    std::unordered_map<RagdollHandle, JPH::Ref<JPH::Ragdoll>> m_ragdolls;
    std::unordered_map<RagdollHandle, JPH::Ref<JPH::RagdollSettings>> m_ragdoll_settings;
    RagdollHandle m_next_ragdoll_handle = 1;

public:
    JoltPhysicsCore() = default;
    virtual ~JoltPhysicsCore() override;
    bool ReadBodyState(BodyHandle body, NativeBodyState& state, bool active_only) const;

    void Initialize() override;
    void SetPreIntegrationCallback(void (*callback)()) override { m_pre_integration_callback = callback; }
    void Clear() override;
    void Step(float delta_time) override;
    void Destroy() override;
    PhysicsCoreStatistics GetStatistics() const override;
    void SetSimulationParameters(float gravity, u32 iterations) override;
    void UpdateJointFeedback(float delta_time);
    void PrepareJointFeedback();

    void DebugDraw(const Fvector& camera_pos) override;
    void SetDebugDrawFlags(u32 flags) override;
    void SetDebugDrawDistance(float distance) override;

    PhysicsShapeHandle BuildCDBModel(const Fvector* verts, u32 v_cnt, const void* tris, u32 t_cnt, const u32* tri_indices = nullptr, u32 tri_indices_cnt = 0) override;
    void DestroyCDBModel(PhysicsShapeHandle handle) override;

    void RaycastCDBModel(PhysicsShapeHandle handle, const Fvector& start, const Fvector& dir, float range, std::vector<CDBRaycastHit>& out_hits) override;
    void RaycastCDBModel(PhysicsShapeHandle handle,
                        const Fvector& start, const Fvector& dir, float range,
                        CDBRayMode mode, bool cull_backfaces,
                        std::vector<CDBRaycastHit>& out_hits) override;

    bool BoxQueryCDB(PhysicsShapeHandle handle,
                        const Fvector& box_center,
                        const Fvector& box_z_axis,
                        const Fvector& box_y_axis,
                        const Fvector& box_sizes,
                        std::vector<u32>& out_tri_indices) override;
    void BoxQueryCDB(PhysicsShapeHandle handle,
                        const Fvector& center, const Fvector& extents,
                        CDBRayMode mode, bool cull_backfaces,
                        std::vector<u32>& out_tri_indices) override;

    long GetShapeMemoryUsage(PhysicsShapeHandle handle) override;
    void GetCDBModelBounds(PhysicsShapeHandle handle, Fvector& out_center, Fvector& out_extents) const override;

    BodyHandle CreateBox(const Fvector& half_extents, const Fvector& position, float mass) override;
    BodyHandle CreateSphere(float radius, const Fvector& position, float mass) override;
    BodyHandle CreateCylinder(float radius, float half_height, const Fvector& position, float mass) override;

    void GetBoxExtents(BodyHandle body, Fvector& out_extents) const override;
    void SetBoxExtents(BodyHandle body, const Fvector& extents) override;

    void DestroyBody(BodyHandle body) override;
    void SetBodyInWorld(BodyHandle body, bool enabled) override;
    void GetBodyTransform(BodyHandle body, Fmatrix& out_matrix) const override;
    void SetBodyTransform(BodyHandle body, const Fmatrix& matrix) override;
    void GetBodyAABB(BodyHandle body, Fvector& center, Fvector& half_extents) const override;

    // --- Сочленения (Joints / Constraints) ---
    JointHandle CreateJoint(int type, BodyHandle b1, BodyHandle b2,
                                    const Fvector& anchor,
                                    const Fvector& axis0, const Fvector& axis1, const Fvector& axis2,
                                    const Fvector& limits_lo, const Fvector& limits_hi) override;
    void DestroyJoint(JointHandle joint) override;

    void SetJointLimits(JointHandle joint, int axis_num, float lo, float hi) override;
    void SetJointMotor(JointHandle joint, int axis_num, float force, float velocity) override;
    void SetJointSpringDamping(JointHandle joint, int axis_num, float erp, float cfm) override;
    void SetJointAxisDir(JointHandle joint, int axis_num, const Fvector& axis) override;
    void SetJointFudgeFactor(JointHandle joint, float factor) override;
    void SetJointFeedback(JointHandle joint, SPhysicsJointFeedback* feedback) override;

    void GetJointAxisDir(JointHandle joint, int axis_num, Fvector& axis) const override;
    void GetJointAnchor(JointHandle joint, Fvector& anchor) const override;
    float GetJointAxisAngle(JointHandle joint, int axis_num) const override;
    float GetJointAxisAngleRate(JointHandle joint, int axis_num) const override;

    // -----------------------------------------

    void GetBodyLinearVelocity(BodyHandle body, Fvector& out_vel) const override;
    void SetBodyLinearVelocity(BodyHandle body, const Fvector& vel) override;

    void GetBodyAngularVelocity(BodyHandle body, Fvector& out_vel) const override;
    void SetBodyAngularVelocity(BodyHandle body, const Fvector& vel) override;

    void SetBodyGravityFactor(BodyHandle body, float factor) override;
    void SetBodyDamping(BodyHandle body, float linear, float angular) override;

    void ApplyLinearImpulse(BodyHandle body, const Fvector& impulse) override;
    void ApplyPointImpulse(BodyHandle body, const Fvector& impulse, const Fvector& point) override;
    void ApplyForce(BodyHandle body, const Fvector& force) override;
    void ApplyTorque(BodyHandle body, const Fvector& torque) override;

    bool IsBodyActive(BodyHandle body) const override;
    void ActivateBody(BodyHandle body) override;
    void DeactivateBody(BodyHandle body) override;

    float GetBodyMass(BodyHandle body) const override;

    void GetBodyPointVelocity(BodyHandle body, const Fvector& point, Fvector& velocity) const override;

    void SetBodyIgnoreStatic(BodyHandle body) override;
    void SetBodyCollideWithStatics(BodyHandle body_handle, bool collide) override;
    void SetBodyObjectLayer(BodyHandle body_handle, u32 layer) override;

    void GetBodyPosition(BodyHandle body, Fvector& position) const override;
    void SetBodyPosition(BodyHandle body, const Fvector& position) override;

    void GetBodyForce(BodyHandle body, Fvector& force) const override;
    void SetBodyForce(BodyHandle body, const Fvector& force) override;
    void GetBodyTorque(BodyHandle body, Fvector& torque) const override;
    void SetBodyTorque(BodyHandle body, const Fvector& torque) override;
    void SetBodyMass(BodyHandle body, float mass) override;

    float GetBodyGravityFactor(BodyHandle body) const override;

    void* GetBodyUserData(BodyHandle body) const override;
    void SetBodyUserData(BodyHandle body, void* data) override;

    void SetBodyFixedRotation(BodyHandle body_handle) override;
    void SetBodyMotionType(BodyHandle body_handle, int motion_type) override;
    BodyHandle CreateStaticBody(PhysicsShapeHandle shape_handle, const Fvector& position) override;
    virtual PhysicsShapeHandle CreateCompoundShape(PhysicsShapeHandle* shapes, const Fmatrix* transforms, size_t count) override;
    virtual BodyHandle CreateBodyFromShape(PhysicsShapeHandle shape, const Fvector& initial_pos, float mass) override;

    PhysicsShapeHandle CreateBoxShape(const Fvector& half_extents) override;
    PhysicsShapeHandle CreateSphereShape(float radius) override;
    PhysicsShapeHandle CreateCylinderShape(float radius, float half_height) override;
    PhysicsShapeHandle CreateCapsuleShape(float radius, float half_height) override;
    PhysicsShapeHandle CreateRotatedTranslatedShape(PhysicsShapeHandle base_shape, const Fvector& position, const Fquaternion& rotation) override;
    CharacterVirtualHandle CreateCharacterVirtual(PhysicsShapeHandle shape, const Fvector& initial_pos, float mass) override;
    void DestroyCharacterVirtual(CharacterVirtualHandle handle) override;
    virtual void SetCharacterVirtualVelocity(CharacterVirtualHandle handle, const Fvector& velocity) override;
    virtual void GetCharacterVirtualVelocity(CharacterVirtualHandle handle, Fvector& velocity) const override;
    virtual void GetCharacterVirtualAABB(CharacterVirtualHandle handle, Fvector& center, Fvector& half_extents) const override;
    virtual void SetCharacterVirtualGravityFactor(CharacterVirtualHandle handle, float factor) override;
    virtual void SetCharacterVirtualUserData(CharacterVirtualHandle handle, void* data) override;
    virtual void GetCharacterVirtualGroundState(CharacterVirtualHandle handle, SJoltCharacterGroundState& out_state) const override;
    virtual void GetCharacterVirtualPosition(CharacterVirtualHandle handle, Fvector& position) const override;
    void SetCharacterVirtualPosition(CharacterVirtualHandle handle, const Fvector& position) override;
    void SetCharacterVirtualShape(CharacterVirtualHandle handle, PhysicsShapeHandle shape) override;
    void ActivateCharacterVirtual(CharacterVirtualHandle handle) override;
    void DeactivateCharacterVirtual(CharacterVirtualHandle handle) override;
    bool IsCharacterVirtualOnGround(CharacterVirtualHandle handle) const override;
    void UpdateCharacterVirtual(CharacterVirtualHandle handle, float delta_time, const Fvector& gravity) override;
    void SetCharacterVirtualStickToFloor(CharacterVirtualHandle handle, bool stick_to_floor) override;
    void SetCharacterVirtualContactCallback(CharacterVirtualHandle handle, CharacterContactCallbackFun callback, void* char_user_data) override;
    void SetRigidBodyContactCallback(RigidBodyContactCallbackFun callback) override;
    void SetShapeMaterial(PhysicsShapeHandle shape, u16 material) override;
    PhysicsMassProperties GetShapeMassProperties(PhysicsShapeHandle shape, float mass) const override;
    PhysicsMassProperties GetBodyMassProperties(BodyHandle body) const override;
    void SetBodyMassProperties(BodyHandle body, const PhysicsMassProperties& properties) override;
    void SetBodyShape(BodyHandle body, PhysicsShapeHandle shape) override;
    void RecenterBody(BodyHandle body, const Fvector& local_delta) override;
    void SetBodyContinuousCollision(BodyHandle body, bool enabled) override;
    bool CharacterHasDynamicContact(CharacterVirtualHandle character) const override;
    void SetCollisionFilter(CollisionFilterFun callback) override { m_collision_filter = callback; }
    void SetBodyContactPolicyCallback(BodyContactPolicyFun callback) override { m_body_contact_policy_callback = callback; }
    void SetDeferredRigidBodyContactCallback(void (*callback)(const NativePhysicsContact&)) override { m_deferred_rb_contact_callback = callback; }
    void SetQueryFilter(QueryFilterFun callback) override { m_query_filter = callback; }
    float GetCharacterVirtualGravityFactor(CharacterVirtualHandle handle) const override;
    bool CheckShapePlacement(PhysicsShapeHandle shape, const Fvector& pos, const Fquaternion& rot, bool check_characters = true, void* ignore_user_data = nullptr, bool camera = false) const override;
    bool FindFreeShapePlacement(PhysicsShapeHandle shape, const Fvector& start_pos, const Fquaternion& rot, Fvector& out_pos, float search_radius = 2.5f, int samples = 16, bool check_characters = true, void* ignore_user_data = nullptr, bool camera = false) const override;

    RagdollHandle CreateRagdoll(const SRagdollSettings& settings) override;
    void DestroyRagdoll(RagdollHandle handle) override;
    void AddRagdollToWorld(RagdollHandle handle, bool activate) override;
    void RemoveRagdollFromWorld(RagdollHandle handle) override;
    void SetRagdollTargetPose(RagdollHandle handle, const Fquaternion* target_rotations, u32 count) override;
    void SetRagdollTargetPose(RagdollHandle handle, const Fmatrix* target_matrices, u32 count) override;
    void SetRagdollWorldPose(RagdollHandle handle, const Fmatrix& world_transform, const Fmatrix* bone_model_matrices, u32 count) override;
    void SetRagdollRootKinematic(RagdollHandle handle, bool kinematic) override;
    void SetRagdollRootTransform(RagdollHandle handle, const Fvector& position, const Fquaternion& rotation) override;
    void SetRagdollMotorState(RagdollHandle handle, bool enabled) override;
    void SetRagdollConstraintMotorState(RagdollHandle handle, u32 constraint_index, bool enabled) override;
    void SetRagdollMotorStiffness(RagdollHandle handle, float stiffness) override;
    void SetRagdollMotorDamping(RagdollHandle handle, float damping) override;
    void SetRagdollConstraintMotor(RagdollHandle handle, u32 constraint_index, float stiffness, float damping) override;
    void SetRagdollPartMotor(RagdollHandle handle, u32 part_index, float stiffness, float damping) override;
    void SetRagdollPartMotionType(RagdollHandle handle, u32 part_index, bool kinematic) override;
    void SetRagdollAllPartsKinematic(RagdollHandle handle, bool kinematic) override;
    void SetRagdollUserData(RagdollHandle handle, void* user_data) override;
    void SetRagdollGroupID(RagdollHandle handle, u32 group_id) override;
    void SetRagdollPartTransform(RagdollHandle handle, u32 part_index, const Fvector& position, const Fquaternion& rotation) override;
    void ApplyRagdollImpulse(RagdollHandle handle, u32 part_index, const Fvector& impulse, const Fvector& point) override;
    void ApplyRagdollLinearImpulse(RagdollHandle handle, u32 part_index, const Fvector& impulse) override;
    void GetRagdollPartTransform(RagdollHandle handle, u32 part_index, Fvector& out_position, Fquaternion& out_rotation) const override;
    void GetRagdollAllTransforms(RagdollHandle handle, Fmatrix* out_matrices, u32 count) const override;
    void GetRagdollPartVelocity(RagdollHandle handle, u32 part_index, Fvector& out_linear_vel, Fvector& out_angular_vel) const override;
    float GetRagdollTotalEnergy(RagdollHandle handle) const override;
    u32 GetRagdollPartCount(RagdollHandle handle) const override;
    void SetRagdollCollisionGroup(RagdollHandle handle, u32 group_id) override;
    void SetBodyActivationCallback(BodyActivationCallbackFun callback) override;
    void SetBodyContactFeedback(BodyHandle body, bool enabled) override;
    const std::vector<PhysicsContactForce>& GetBodyContactForces(BodyHandle body) const override;
    void SetBodyCollisionOwner(BodyHandle body, void* owner) override;
    void SetCharacterPairShapeCallback(CharacterPairShapeFun callback) override { m_character_pair_shape = callback; }
};
