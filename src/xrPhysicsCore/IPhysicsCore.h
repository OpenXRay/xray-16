#pragma once

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrCore/_quaternion.h"
#include <vector>

#if defined(XRAY_STATIC_BUILD)
#   define PHYSICS_CORE_API
#elif defined(_MSC_VER)
#   ifdef XRPHYSICSCORE_EXPORTS
#       define PHYSICS_CORE_API __declspec(dllexport)
#   else
#       define PHYSICS_CORE_API __declspec(dllimport)
#   endif
#else
#   define PHYSICS_CORE_API __attribute__((visibility("default")))
#endif

typedef uint32_t BodyHandle;
constexpr BodyHandle INVALID_BODY_HANDLE = 0xFFFFFFFF;

typedef void* PhysicsShapeHandle;

typedef uint32_t JointHandle;
constexpr JointHandle INVALID_JOINT_HANDLE = 0xFFFFFFFF;

typedef uint32_t CharacterVirtualHandle;
constexpr CharacterVirtualHandle INVALID_CHARACTER_VIRTUAL_HANDLE = 0xFFFFFFFF;

typedef uint32_t RagdollHandle;
constexpr RagdollHandle INVALID_RAGDOLL_HANDLE = 0xFFFFFFFF;

struct SRagdollPartDesc {
    PhysicsShapeHandle shape;
    Fvector position;
    Fquaternion rotation;
    float mass;
    u16 bone_id;
    int parent_index; // -1 for root
};

struct SRagdollConstraintDesc {
    int child_index;
    Fvector twist_axis;
    Fvector plane_axis;
    float swing_limit_y;
    float swing_limit_z;
    float twist_limit_min;
    float twist_limit_max;
    float max_friction_torque;
};

struct SRagdollMotorSettings {
    float stiffness;
    float damping;
};

struct SRagdollSettings {
    std::vector<SRagdollPartDesc> parts;
    std::vector<SRagdollConstraintDesc> constraints;
    SRagdollMotorSettings default_motor;
};

struct CDBRaycastHit {
    float range;
    u32 tri_index;
};

enum class CDBRayMode {
    All,
    Nearest,
    First
};

struct SPhysicsJointFeedback;

struct PhysicsCoreStatistics {
    u32 bodies = 0, active_bodies = 0, characters = 0, constraints = 0, contact_points = 0;
    u32 active_constraints = 0;
    u32 workers = 0;
    double contact_preparation_ms = 0, integration_ms = 0, feedback_ms = 0;
};

struct NativePhysicsContact {
    BodyHandle body1 = INVALID_BODY_HANDLE, body2 = INVALID_BODY_HANDLE;
    void* object1 = nullptr;
    void* object2 = nullptr;
    u16 kind1 = 0, kind2 = 0, geometry1 = 0, geometry2 = 0;
    u16 material1 = u16(-1), material2 = u16(-1);
    u32 subshape1 = 0, subshape2 = 0;
    u32 triangle1 = u32(-1), triangle2 = u32(-1);
    Fvector position, normal;
    float relative_velocity = 0, depth = 0;
    // Main-thread gameplay policies can replace or scale the material friction
    // before the native solver builds this contact, including cached contacts.
    mutable float friction = -1, friction_scale = 1;
    mutable bool static_body1 = false, static_body2 = false;
};

struct PhysicsMassProperties {
    float mass = 0;
    Fmatrix inertia = Fidentity;
};

struct PhysicsContactForce {
    u16 geometry = 0;
    Fvector position, force, torque;
};

class IPhysicsCore {
public:
    virtual ~IPhysicsCore() = default;

    virtual void Initialize() = 0;
    virtual void Clear() = 0;
    virtual void Step(float delta_time) = 0;
    virtual void Destroy() = 0;
    virtual PhysicsCoreStatistics GetStatistics() const = 0;
    virtual void SetSimulationParameters(float gravity, u32 iterations) = 0;
    // Runs after contact policies and before integration, outside native locks.
    virtual void SetPreIntegrationCallback(void (*callback)()) = 0;

    virtual void DebugDraw(const Fvector& camera_pos) = 0;
    virtual void SetDebugDrawFlags(u32 flags) = 0;
    virtual void SetDebugDrawDistance(float distance) = 0;

    virtual PhysicsShapeHandle BuildCDBModel(const Fvector* verts, u32 v_cnt, const void* tris, u32 t_cnt, const u32* tri_indices = nullptr, u32 tri_indices_cnt = 0) = 0;
    virtual void DestroyCDBModel(PhysicsShapeHandle handle) = 0;
    virtual void RaycastCDBModel(PhysicsShapeHandle handle, const Fvector& start, const Fvector& dir, float range, std::vector<CDBRaycastHit>& out_hits) = 0;
    virtual void RaycastCDBModel(PhysicsShapeHandle handle,
                                 const Fvector& start, const Fvector& dir, float range,
                                 CDBRayMode mode, bool cull_backfaces,
                                 std::vector<CDBRaycastHit>& out_hits) = 0;

    virtual bool BoxQueryCDB(PhysicsShapeHandle handle,
                         const Fvector& box_center,
                         const Fvector& box_z_axis,
                         const Fvector& box_y_axis,
                         const Fvector& box_sizes,
                         std::vector<u32>& out_tri_indices) = 0;

    virtual void BoxQueryCDB(PhysicsShapeHandle handle,
                             const Fvector& center, const Fvector& extents,
                             CDBRayMode mode, bool cull_backfaces,
                             std::vector<u32>& out_tri_indices) = 0;

    virtual long GetShapeMemoryUsage(PhysicsShapeHandle handle) = 0;

    virtual BodyHandle CreateBox(const Fvector& half_extents, const Fvector& position, float mass) = 0;
    virtual void DestroyBody(BodyHandle body) = 0;
    virtual void SetBodyInWorld(BodyHandle body, bool enabled) = 0;

    virtual BodyHandle CreateSphere(float radius, const Fvector& position, float mass) = 0;
    virtual BodyHandle CreateCylinder(float radius, float half_height, const Fvector& position, float mass) = 0;

    virtual void GetBoxExtents(BodyHandle body, Fvector& out_extents) const = 0;
    virtual void SetBoxExtents(BodyHandle body, const Fvector& extents) = 0;

    virtual void GetBodyTransform(BodyHandle body, Fmatrix& out_matrix) const = 0;
    virtual void SetBodyTransform(BodyHandle body, const Fmatrix& matrix) = 0;
    virtual void GetBodyAABB(BodyHandle body, Fvector& center, Fvector& half_extents) const = 0;

    virtual void GetCDBModelBounds(PhysicsShapeHandle handle, Fvector& out_center, Fvector& out_extents) const = 0;

    // type: 0=Ball, 1=Hinge, 2=Hinge2, 3=FullControl, 4=Slider (соответствует X-Ray CPhysicsJoint::enumType)
    virtual JointHandle CreateJoint(int type, BodyHandle b1, BodyHandle b2,
                                    const Fvector& anchor,
                                    const Fvector& axis0, const Fvector& axis1, const Fvector& axis2,
                                    const Fvector& limits_lo, const Fvector& limits_hi) = 0;
    virtual void DestroyJoint(JointHandle joint) = 0;

    virtual void SetJointLimits(JointHandle joint, int axis_num, float lo, float hi) = 0;
    virtual void SetJointMotor(JointHandle joint, int axis_num, float force, float velocity) = 0;
    virtual void SetJointSpringDamping(JointHandle joint, int axis_num, float erp, float cfm) = 0;
    virtual void SetJointAxisDir(JointHandle joint, int axis_num, const Fvector& axis) = 0;
    virtual void SetJointFudgeFactor(JointHandle joint, float factor) = 0;
    virtual void SetJointFeedback(JointHandle joint, SPhysicsJointFeedback* feedback) = 0;

    virtual void GetJointAxisDir(JointHandle joint, int axis_num, Fvector& axis) const = 0;
    virtual void GetJointAnchor(JointHandle joint, Fvector& anchor) const = 0;
    virtual float GetJointAxisAngle(JointHandle joint, int axis_num) const = 0;
    virtual float GetJointAxisAngleRate(JointHandle joint, int axis_num) const = 0;

    virtual void GetBodyLinearVelocity(BodyHandle body, Fvector& out_vel) const = 0;
    virtual void SetBodyLinearVelocity(BodyHandle body, const Fvector& vel) = 0;

    virtual void GetBodyAngularVelocity(BodyHandle body, Fvector& out_vel) const = 0;
    virtual void SetBodyAngularVelocity(BodyHandle body, const Fvector& vel) = 0;

    virtual void SetBodyGravityFactor(BodyHandle body, float factor) = 0;
    virtual void SetBodyDamping(BodyHandle body, float linear, float angular) = 0;

    virtual void ApplyLinearImpulse(BodyHandle body, const Fvector& impulse) = 0;
    virtual void ApplyPointImpulse(BodyHandle body, const Fvector& impulse, const Fvector& point) = 0;
    virtual void ApplyForce(BodyHandle body, const Fvector& force) = 0;
    virtual void ApplyTorque(BodyHandle body, const Fvector& torque) = 0;

    virtual bool IsBodyActive(BodyHandle body) const = 0;
    virtual void ActivateBody(BodyHandle body) = 0;
    virtual void DeactivateBody(BodyHandle body) = 0;

    virtual float GetBodyMass(BodyHandle body) const = 0;

    virtual void GetBodyPointVelocity(BodyHandle body, const Fvector& point, Fvector& velocity) const = 0;

    virtual void SetBodyIgnoreStatic(BodyHandle body) = 0;

    virtual void SetBodyCollideWithStatics(BodyHandle body_handle, bool collide) = 0;
    virtual void SetBodyObjectLayer(BodyHandle body_handle, u32 layer) = 0;

    virtual void GetBodyPosition(BodyHandle body, Fvector& position) const = 0;
    virtual void SetBodyPosition(BodyHandle body, const Fvector& position) = 0;

    virtual void GetBodyForce(BodyHandle body, Fvector& force) const = 0;
    virtual void SetBodyForce(BodyHandle body, const Fvector& force) = 0;
    virtual void GetBodyTorque(BodyHandle body, Fvector& torque) const = 0;
    virtual void SetBodyTorque(BodyHandle body, const Fvector& torque) = 0;
    virtual void SetBodyMass(BodyHandle body, float mass) = 0;
    virtual PhysicsMassProperties GetShapeMassProperties(PhysicsShapeHandle shape, float mass) const = 0;
    virtual PhysicsMassProperties GetBodyMassProperties(BodyHandle body) const = 0;
    virtual void SetBodyMassProperties(BodyHandle body, const PhysicsMassProperties& properties) = 0;
    virtual void SetBodyShape(BodyHandle body, PhysicsShapeHandle shape) = 0;
    // Move the mass center within an unchanged body frame, preserving joint anchors.
    virtual void RecenterBody(BodyHandle body, const Fvector& local_delta) = 0;
    virtual void SetBodyContinuousCollision(BodyHandle body, bool enabled) = 0;
    virtual bool CharacterHasDynamicContact(CharacterVirtualHandle character) const = 0;

    virtual float GetBodyGravityFactor(BodyHandle body) const = 0;

    virtual void* GetBodyUserData(BodyHandle body) const = 0;
    virtual void SetBodyUserData(BodyHandle body, void* data) = 0;

    virtual void SetBodyFixedRotation(BodyHandle body_handle) = 0;
    virtual void SetBodyMotionType(BodyHandle body_handle, int motion_type) = 0;
    virtual BodyHandle CreateStaticBody(PhysicsShapeHandle shape_handle, const Fvector& position) = 0;

    virtual PhysicsShapeHandle CreateCompoundShape(PhysicsShapeHandle* shapes, const Fmatrix* transforms, size_t count) = 0;
    virtual BodyHandle CreateBodyFromShape(PhysicsShapeHandle shape, const Fvector& initial_pos, float mass) = 0;

    virtual PhysicsShapeHandle CreateBoxShape(const Fvector& half_extents) = 0;
    virtual PhysicsShapeHandle CreateSphereShape(float radius) = 0;
    virtual PhysicsShapeHandle CreateCylinderShape(float radius, float half_height) = 0;
    virtual PhysicsShapeHandle CreateCapsuleShape(float radius, float half_height) = 0;
    virtual PhysicsShapeHandle CreateRotatedTranslatedShape(PhysicsShapeHandle base_shape, const Fvector& position, const Fquaternion& rotation) = 0;
    virtual CharacterVirtualHandle CreateCharacterVirtual(PhysicsShapeHandle shape, const Fvector& initial_pos, float mass) = 0;
    virtual void DestroyCharacterVirtual(CharacterVirtualHandle handle) = 0;
    virtual void SetCharacterVirtualVelocity(CharacterVirtualHandle handle, const Fvector& velocity) = 0;
    virtual void GetCharacterVirtualPosition(CharacterVirtualHandle handle, Fvector& position) const = 0;
    virtual void SetCharacterVirtualPosition(CharacterVirtualHandle handle, const Fvector& position) = 0;
    virtual void SetCharacterVirtualShape(CharacterVirtualHandle handle, PhysicsShapeHandle shape) = 0;
    virtual void ActivateCharacterVirtual(CharacterVirtualHandle handle) = 0;
    virtual void DeactivateCharacterVirtual(CharacterVirtualHandle handle) = 0;
    virtual void GetCharacterVirtualVelocity(CharacterVirtualHandle handle, Fvector& velocity) const = 0;
    virtual void GetCharacterVirtualAABB(CharacterVirtualHandle handle, Fvector& center, Fvector& half_extents) const = 0;
    virtual void SetCharacterVirtualGravityFactor(CharacterVirtualHandle handle, float factor) = 0;
    virtual void SetCharacterVirtualUserData(CharacterVirtualHandle handle, void* data) = 0;

    struct SJoltCharacterGroundState {
        bool on_ground;
        Fvector ground_normal;
        Fvector ground_velocity;
        u32 ground_triangle_user_data;
    };
    virtual void GetCharacterVirtualGroundState(CharacterVirtualHandle handle, SJoltCharacterGroundState& out_state) const = 0;

    typedef void (*CharacterContactCallbackFun)(void* char_user_data,
                                               const Fvector& contact_pos,
                                               const Fvector& contact_normal,
                                               const Fvector& contact_vel,
                                               u32 tri_user_data,
                                               BodyHandle other_body_handle,
                                               void* other_body_user_data,
                                               bool is_sensor);

    // Runs on the gameplay thread before integration, without native body
    // locks. Returning false suppresses this contact's physical response.
    typedef bool (*RigidBodyContactCallbackFun)(const NativePhysicsContact& contact);

    virtual void SetCharacterVirtualContactCallback(CharacterVirtualHandle handle, CharacterContactCallbackFun callback, void* char_user_data) = 0;
    virtual void SetRigidBodyContactCallback(RigidBodyContactCallbackFun callback) = 0;
    virtual void SetShapeMaterial(PhysicsShapeHandle shape, u16 material) = 0;
    // Called during native collision detection. Implementations must only read
    // stable state; gameplay mutation belongs in the deferred contact callback.
    typedef bool (*CollisionFilterFun)(void* object1, u16 kind1, void* object2, u16 kind2);
    virtual void SetCollisionFilter(CollisionFilterFun callback) = 0;
    typedef bool (*QueryFilterFun)(void* object, u16 kind, void* ignored_holder, bool camera);
    virtual void SetQueryFilter(QueryFilterFun callback) = 0;
    virtual float GetCharacterVirtualGravityFactor(CharacterVirtualHandle handle) const = 0;

    virtual bool IsCharacterVirtualOnGround(CharacterVirtualHandle handle) const = 0;
    virtual void UpdateCharacterVirtual(CharacterVirtualHandle handle, float delta_time, const Fvector& gravity) = 0;
    virtual void SetCharacterVirtualStickToFloor(CharacterVirtualHandle handle, bool stick_to_floor) = 0;

    virtual bool CheckShapePlacement(PhysicsShapeHandle shape, const Fvector& pos, const Fquaternion& rot, bool check_characters = true, void* ignore_user_data = nullptr, bool camera = false) const = 0;
    virtual bool FindFreeShapePlacement(PhysicsShapeHandle shape, const Fvector& start_pos, const Fquaternion& rot, Fvector& out_pos, float search_radius = 2.5f, int samples = 16, bool check_characters = true, void* ignore_user_data = nullptr, bool camera = false) const = 0;

    // --- Active Ragdoll API ---
    virtual RagdollHandle CreateRagdoll(const SRagdollSettings& settings) = 0;
    virtual void DestroyRagdoll(RagdollHandle handle) = 0;

    virtual void AddRagdollToWorld(RagdollHandle handle, bool activate) = 0;
    virtual void RemoveRagdollFromWorld(RagdollHandle handle) = 0;

    virtual void SetRagdollTargetPose(RagdollHandle handle, const Fquaternion* target_rotations, u32 count) = 0;
    virtual void SetRagdollTargetPose(RagdollHandle handle, const Fmatrix* target_matrices, u32 count) = 0;
    virtual void SetRagdollWorldPose(RagdollHandle handle, const Fmatrix& world_transform, const Fmatrix* bone_model_matrices, u32 count) = 0;

    virtual void SetRagdollRootKinematic(RagdollHandle handle, bool kinematic) = 0;
    virtual void SetRagdollRootTransform(RagdollHandle handle, const Fvector& position, const Fquaternion& rotation) = 0;

    virtual void SetRagdollMotorState(RagdollHandle handle, bool enabled) = 0;
    virtual void SetRagdollConstraintMotorState(RagdollHandle handle, u32 constraint_index, bool enabled) = 0;
    virtual void SetRagdollMotorStiffness(RagdollHandle handle, float stiffness) = 0;
    virtual void SetRagdollMotorDamping(RagdollHandle handle, float damping) = 0;
    virtual void SetRagdollConstraintMotor(RagdollHandle handle, u32 constraint_index, float stiffness, float damping) = 0;
    virtual void SetRagdollPartMotor(RagdollHandle handle, u32 part_index, float stiffness, float damping) = 0;

    virtual void SetRagdollPartMotionType(RagdollHandle handle, u32 part_index, bool kinematic) = 0;
    virtual void SetRagdollAllPartsKinematic(RagdollHandle handle, bool kinematic) = 0;
    virtual void SetRagdollUserData(RagdollHandle handle, void* user_data) = 0;
    virtual void SetRagdollGroupID(RagdollHandle handle, u32 group_id) = 0;

    virtual void SetRagdollPartTransform(RagdollHandle handle, u32 part_index, const Fvector& position, const Fquaternion& rotation) = 0;
    virtual void ApplyRagdollImpulse(RagdollHandle handle, u32 part_index, const Fvector& impulse, const Fvector& point) = 0;
    virtual void ApplyRagdollLinearImpulse(RagdollHandle handle, u32 part_index, const Fvector& impulse) = 0;

    virtual void GetRagdollPartTransform(RagdollHandle handle, u32 part_index, Fvector& out_position, Fquaternion& out_rotation) const = 0;
    virtual void GetRagdollAllTransforms(RagdollHandle handle, Fmatrix* out_matrices, u32 count) const = 0;
    virtual void GetRagdollPartVelocity(RagdollHandle handle, u32 part_index, Fvector& out_linear_vel, Fvector& out_angular_vel) const = 0;
    virtual float GetRagdollTotalEnergy(RagdollHandle handle) const = 0;
    virtual u32 GetRagdollPartCount(RagdollHandle handle) const = 0;

    virtual void SetRagdollCollisionGroup(RagdollHandle handle, u32 group_id) = 0;

    typedef void (*BodyActivationCallbackFun)(BodyHandle body, void* user_data, bool activated);
    virtual void SetBodyActivationCallback(BodyActivationCallbackFun callback) = 0;
    virtual void SetBodyContactFeedback(BodyHandle body, bool enabled) = 0;
    // Valid until the next step or body retirement. No native locks are held.
    virtual const std::vector<PhysicsContactForce>& GetBodyContactForces(BodyHandle body) const = 0;
    virtual void SetBodyCollisionOwner(BodyHandle body, void* owner) = 0;
    // A pair may use a wider actor/NPC restriction shape while retaining the
    // normal character shape for world geometry. Returned shapes remain owned
    // by the caller throughout the character update.
    using CharacterPairShapeFun = PhysicsShapeHandle(*)(void* character, void* other);
    virtual void SetCharacterPairShapeCallback(CharacterPairShapeFun callback) = 0;
};

extern "C" PHYSICS_CORE_API IPhysicsCore* GetPhysicsCore();
