#include "stdafx.h"
#include "JoltPhysicsCore.h"
#include "xrPhysics/PHJointDestroyInfo.h"
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Body/BodyLock.h>

namespace {
void Store(JPH::Vec3Arg value, Fvector& output) {
    output.set(value.GetX(), value.GetY(), value.GetZ());
}
PhysicsMassProperties ExportMass(const JPH::MassProperties& mass) {
    PhysicsMassProperties result;
    result.mass = mass.mMass;
    Store(mass.mInertia.GetAxisX(), result.inertia.i);
    Store(mass.mInertia.GetAxisY(), result.inertia.j);
    Store(mass.mInertia.GetAxisZ(), result.inertia.k);
    return result;
}
}

PhysicsMassProperties JoltPhysicsCore::GetShapeMassProperties(PhysicsShapeHandle shape, float mass) const {
    if (!shape) return {};
    auto properties = static_cast<const JPH::Shape*>(shape)->GetMassProperties();
    properties.ScaleToMass(mass);
    return ExportMass(properties);
}
PhysicsMassProperties JoltPhysicsCore::GetBodyMassProperties(BodyHandle handle) const {
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return {};
    JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(handle));
    if (!lock.Succeeded() || !lock.GetBody().IsDynamic()) return {};
    const auto* motion = lock.GetBody().GetMotionProperties();
    const auto inverse = motion->GetInverseInertiaDiagonal();
    const auto diagonal = JPH::Vec3(inverse.GetX() > 0 ? 1 / inverse.GetX() : 0,
        inverse.GetY() > 0 ? 1 / inverse.GetY() : 0, inverse.GetZ() > 0 ? 1 / inverse.GetZ() : 0);
    const auto rotation = JPH::Mat44::sRotation(motion->GetInertiaRotation());
    JPH::MassProperties properties;
    properties.mMass = 1 / motion->GetInverseMass();
    properties.mInertia = rotation * JPH::Mat44::sScale(diagonal) * rotation.Transposed3x3();
    return ExportMass(properties);
}
void JoltPhysicsCore::SetBodyMassProperties(BodyHandle handle, const PhysicsMassProperties& mass) {
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return;
    R_ASSERT(mass.mass > 0 && std::isfinite(mass.mass));
    JPH::BodyLockWrite lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(handle));
    if (!lock.Succeeded() || lock.GetBody().IsStatic()) return;
    JPH::MassProperties properties;
    properties.mMass = mass.mass;
    properties.mInertia = JPH::Mat44(JPH::Vec4(JPH::Vec3(mass.inertia.i.x, mass.inertia.i.y, mass.inertia.i.z), 0),
        JPH::Vec4(JPH::Vec3(mass.inertia.j.x, mass.inertia.j.y, mass.inertia.j.z), 0),
        JPH::Vec4(JPH::Vec3(mass.inertia.k.x, mass.inertia.k.y, mass.inertia.k.z), 0), JPH::Vec4(0, 0, 0, 1));
    auto* motion = lock.GetBody().GetMotionProperties();
    motion->SetMassProperties(motion->GetAllowedDOFs(), properties);
}
void JoltPhysicsCore::SetBodyShape(BodyHandle handle, PhysicsShapeHandle shape) {
    if (!m_physics_system || handle == INVALID_BODY_HANDLE || !shape) return;
    auto& bodies = m_physics_system->GetBodyInterface();
    const JPH::BodyID id(handle);
    bodies.SetShape(id, static_cast<const JPH::Shape*>(shape), false,
        bodies.IsAdded(id) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
}

void JoltPhysicsCore::RecenterBody(BodyHandle handle, const Fvector& local_delta) {
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return;
    R_ASSERT(_valid(local_delta));
    const JPH::BodyID id(handle);
    const JPH::Vec3 delta(local_delta.x, local_delta.y, local_delta.z);
    auto& bodies = m_physics_system->GetBodyInterface();
    const auto position = bodies.GetCenterOfMassTransform(id) * delta;
    for (auto& [joint, constraint] : m_constraints) {
        auto* pair = static_cast<JPH::TwoBodyConstraint*>(constraint.GetPtr());
        if (pair->GetBody1()->GetID() == id || pair->GetBody2()->GetID() == id) {
            constraint->NotifyShapeChanged(id, delta);
            constraint->ResetWarmStart();
        }
    }
    SetBodyPosition(handle, Fvector().set(position.GetX(), position.GetY(), position.GetZ()));
}

void JoltPhysicsCore::GetBodyTorque(BodyHandle handle, Fvector& torque) const {
    torque.set(0, 0, 0);
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return;
    JPH::BodyLockRead lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(handle));
    if (lock.Succeeded() && lock.GetBody().IsDynamic()) Store(lock.GetBody().GetAccumulatedTorque(), torque);
}

void JoltPhysicsCore::SetBodyTorque(BodyHandle handle, const Fvector& torque) {
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return;
    JPH::BodyLockWrite lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(handle));
    if (lock.Succeeded() && lock.GetBody().IsDynamic())
        lock.GetBody().AddTorque(JPH::Vec3(torque.x, torque.y, torque.z) - lock.GetBody().GetAccumulatedTorque());
}

void JoltPhysicsCore::SetBodyMass(BodyHandle handle, float mass) {
    if (!m_physics_system || handle == INVALID_BODY_HANDLE) return;
    R_ASSERT(std::isfinite(mass) && mass > 0);
    JPH::BodyLockWrite lock(m_physics_system->GetBodyLockInterface(), JPH::BodyID(handle));
    if (lock.Succeeded() && !lock.GetBody().IsStatic()) {
        auto properties = lock.GetBody().GetShape()->GetMassProperties();
        properties.ScaleToMass(mass);
        auto* motion = lock.GetBody().GetMotionProperties();
        motion->SetMassProperties(motion->GetAllowedDOFs(), properties);
    }
}
namespace {
JPH::Mat44 WorldFrame(const JPH::Body& body, JPH::Mat44Arg local) {
    return JPH::Mat44::sRotation(body.GetRotation()) * local;
}
JPH::Vec3 Perpendicular(JPH::Vec3Arg axis, JPH::Vec3Arg previous) {
    const auto projected = previous - axis * axis.Dot(previous);
    return projected.LengthSq() > 1e-6f ? projected.Normalized() : axis.GetNormalizedPerpendicular();
}
}

void JoltPhysicsCore::SetJointAxisDir(JointHandle handle, int axis_num, const Fvector& direction) {
    auto found = m_constraints.find(handle);
    if (found == m_constraints.end()) return;
    R_ASSERT(axis_num >= 0 && axis_num < 3 && _valid(direction) &&
        JPH::Vec3(direction.x, direction.y, direction.z).LengthSq() > 1e-6f);
    auto* previous = static_cast<JPH::TwoBodyConstraint*>(found->second.GetPtr());
    auto* first = previous->GetBody1();
    auto* second = previous->GetBody2();
    const auto axis = JPH::Vec3(direction.x, direction.y, direction.z).Normalized();
    const auto local1 = first->GetRotation().Conjugated() * axis;
    const auto local2 = second->GetRotation().Conjugated() * axis;
    auto settings = previous->GetConstraintSettings();
    JPH::Constraint* replacement = nullptr;
    if (previous->GetSubType() == JPH::EConstraintSubType::Hinge) {
        auto* values = static_cast<JPH::HingeConstraintSettings*>(settings.GetPtr());
        values->mHingeAxis1 = local1;
        values->mHingeAxis2 = local2;
        values->mNormalAxis1 = Perpendicular(local1, values->mNormalAxis1);
        values->mNormalAxis2 = Perpendicular(local2, values->mNormalAxis2);
        auto* hinge = static_cast<JPH::HingeConstraint*>(values->Create(*first, *second));
        const auto* old = static_cast<const JPH::HingeConstraint*>(previous);
        hinge->SetMotorState(old->GetMotorState());
        hinge->SetTargetAngularVelocity(old->GetTargetAngularVelocity());
        hinge->SetTargetAngle(old->GetTargetAngle());
        replacement = hinge;
    } else if (previous->GetSubType() == JPH::EConstraintSubType::Slider) {
        auto* values = static_cast<JPH::SliderConstraintSettings*>(settings.GetPtr());
        values->mSliderAxis1 = local1;
        values->mSliderAxis2 = local2;
        values->mNormalAxis1 = Perpendicular(local1, values->mNormalAxis1);
        values->mNormalAxis2 = Perpendicular(local2, values->mNormalAxis2);
        auto* slider = static_cast<JPH::SliderConstraint*>(values->Create(*first, *second));
        const auto* old = static_cast<const JPH::SliderConstraint*>(previous);
        slider->SetMotorState(old->GetMotorState());
        slider->SetTargetVelocity(old->GetTargetVelocity());
        slider->SetTargetPosition(old->GetTargetPosition());
        replacement = slider;
    } else if (previous->GetSubType() == JPH::EConstraintSubType::SixDOF) {
        auto* values = static_cast<JPH::SixDOFConstraintSettings*>(settings.GetPtr());
        auto update = [axis_num](JPH::Vec3Arg new_axis, JPH::Vec3& x, JPH::Vec3& y) {
            if (axis_num == 0) { x = new_axis; y = Perpendicular(x, y); }
            else if (axis_num == 1) { y = new_axis; x = Perpendicular(y, x); }
            else { x = Perpendicular(new_axis, x); y = new_axis.Cross(x).Normalized(); }
        };
        update(local1, values->mAxisX1, values->mAxisY1);
        update(local2, values->mAxisX2, values->mAxisY2);
        auto* six = static_cast<JPH::SixDOFConstraint*>(values->Create(*first, *second));
        const auto* old = static_cast<const JPH::SixDOFConstraint*>(previous);
        for (int i = 0; i < 6; ++i)
            six->SetMotorState(static_cast<JPH::SixDOFConstraint::EAxis>(i),
                old->GetMotorState(static_cast<JPH::SixDOFConstraint::EAxis>(i)));
        six->SetTargetVelocityCS(old->GetTargetVelocityCS());
        six->SetTargetAngularVelocityCS(old->GetTargetAngularVelocityCS());
        six->SetTargetPositionCS(old->GetTargetPositionCS());
        six->SetTargetOrientationCS(old->GetTargetOrientationCS());
        replacement = six;
    }
    if (!replacement) return; // A ball constraint has no rotation axis.
    replacement->SetEnabled(previous->GetEnabled());
    m_physics_system->RemoveConstraint(previous);
    found->second = replacement;
    m_physics_system->AddConstraint(replacement);
    m_joint_motors.erase(handle);
    m_joint_limits.erase(handle);
    ActivateBody(first->GetID().GetIndexAndSequenceNumber());
    ActivateBody(second->GetID().GetIndexAndSequenceNumber());
}

PhysicsCoreStatistics JoltPhysicsCore::GetStatistics() const {
    PhysicsCoreStatistics statistics = m_step_statistics;
    if (!m_physics_system) return statistics;
    statistics.bodies = m_physics_system->GetNumBodies();
    statistics.active_bodies = m_physics_system->GetNumActiveBodies(JPH::EBodyType::RigidBody);
    statistics.characters = static_cast<u32>(m_characters.size() - m_inactive_characters.size());
    statistics.constraints = static_cast<u32>(m_constraints.size());
    for (const auto& [handle, constraint] : m_constraints)
        if (constraint->IsActive()) ++statistics.active_constraints;
    statistics.contact_points = m_contact_points.load(std::memory_order_relaxed);
    statistics.workers = m_worker_count;
    return statistics;
}

void JoltPhysicsCore::SetSimulationParameters(float gravity, u32 iterations) {
    if (!m_physics_system) return;
    R_ASSERT(std::isfinite(gravity) && gravity >= 0 && iterations > 0);
    m_physics_system->SetGravity(JPH::Vec3(0, -gravity, 0));
    auto settings = m_physics_system->GetPhysicsSettings();
    settings.mNumVelocitySteps = iterations;
    m_physics_system->SetPhysicsSettings(settings);
}

void JoltPhysicsCore::GetJointAxisDir(JointHandle joint, int axis_num, Fvector& axis) const {
    axis.set(0, 0, 0);
    const auto found = m_constraints.find(joint);
    if (found == m_constraints.end()) return;
    R_ASSERT(axis_num >= 0 && axis_num < 3);
    const auto* constraint = static_cast<const JPH::TwoBodyConstraint*>(found->second.GetPtr());
    const auto frame = WorldFrame(*constraint->GetBody1(), constraint->GetConstraintToBody1Matrix());
    Store(frame.GetColumn3(axis_num), axis);
}

void JoltPhysicsCore::GetJointAnchor(JointHandle joint, Fvector& anchor) const {
    anchor.set(0, 0, 0);
    const auto found = m_constraints.find(joint);
    if (found == m_constraints.end()) return;
    const auto* constraint = static_cast<const JPH::TwoBodyConstraint*>(found->second.GetPtr());
    const auto* body = constraint->GetBody1();
    Store(JPH::Vec3(body->GetCenterOfMassPosition()) +
        body->GetRotation() * constraint->GetConstraintToBody1Matrix().GetTranslation(), anchor);
}

float JoltPhysicsCore::GetJointAxisAngleRate(JointHandle joint, int axis_num) const {
    const auto found = m_constraints.find(joint);
    if (found == m_constraints.end()) return 0;
    Fvector direction;
    GetJointAxisDir(joint, axis_num, direction);
    const JPH::Vec3 axis(direction.x, direction.y, direction.z);
    const auto* constraint = static_cast<const JPH::TwoBodyConstraint*>(found->second.GetPtr());
    const auto* first = constraint->GetBody1();
    const auto* second = constraint->GetBody2();
    if (constraint->GetSubType() == JPH::EConstraintSubType::Slider)
        return (second->GetLinearVelocity() - first->GetLinearVelocity()).Dot(axis);
    return (second->GetAngularVelocity() - first->GetAngularVelocity()).Dot(axis);
}

void JoltPhysicsCore::PrepareJointFeedback() {
    m_feedback_frames.clear();
    for (const auto& [handle, output] : m_joint_feedback) {
        const auto found = m_constraints.find(handle);
        if (found == m_constraints.end()) continue;
        const auto* constraint = static_cast<const JPH::TwoBodyConstraint*>(found->second.GetPtr());
        const auto local1 = constraint->GetConstraintToBody1Matrix();
        const auto local2 = constraint->GetConstraintToBody2Matrix();
        m_feedback_frames.emplace(handle, FeedbackFrame{
            WorldFrame(*constraint->GetBody1(), local1), WorldFrame(*constraint->GetBody2(), local2),
            constraint->GetBody1()->GetRotation() * local1.GetTranslation(),
            constraint->GetBody2()->GetRotation() * local2.GetTranslation()});
    }
}

void JoltPhysicsCore::UpdateJointFeedback(float delta_time) {
    if (delta_time <= 0) return;
    for (const auto& [handle, output] : m_joint_feedback) {
        const auto found = m_constraints.find(handle);
        const auto frame = m_feedback_frames.find(handle);
        if (found == m_constraints.end() || frame == m_feedback_frames.end()) continue;
        const auto& basis = frame->second;
        JPH::Vec3 linear = JPH::Vec3::sZero(), angular = JPH::Vec3::sZero();
        switch (found->second->GetSubType()) {
        case JPH::EConstraintSubType::Point:
            linear = static_cast<const JPH::PointConstraint*>(found->second.GetPtr())->GetTotalLambdaPosition();
            break;
        case JPH::EConstraintSubType::Hinge: {
            const auto* hinge = static_cast<const JPH::HingeConstraint*>(found->second.GetPtr());
            linear = hinge->GetTotalLambdaPosition();
            const auto rotation = hinge->GetTotalLambdaRotation();
            const auto axis1 = basis.first.GetAxisX();
            auto axis2 = basis.second.GetAxisX();
            const float dot = axis1.Dot(axis2);
            if (dot <= 1e-3f) {
                auto perpendicular = axis2 - dot * axis1;
                if (perpendicular.LengthSq() < 1e-6f) perpendicular = axis1.GetNormalizedPerpendicular();
                axis2 = (0.99f * perpendicular.Normalized() + 0.01f * axis1).Normalized();
            }
            const auto perpendicular = axis2.GetNormalizedPerpendicular();
            angular = perpendicular.Cross(axis1) * rotation[0] +
                axis2.Cross(perpendicular).Cross(axis1) * rotation[1] +
                axis1 * (hinge->GetTotalLambdaRotationLimits() + hinge->GetTotalLambdaMotor());
            break;
        }
        case JPH::EConstraintSubType::Slider: {
            const auto* slider = static_cast<const JPH::SliderConstraint*>(found->second.GetPtr());
            const auto position = slider->GetTotalLambdaPosition();
            linear = basis.first.GetAxisY() * position[0] + basis.first.GetAxisZ() * position[1] +
                basis.first.GetAxisX() * (slider->GetTotalLambdaPositionLimits() + slider->GetTotalLambdaMotor());
            angular = slider->GetTotalLambdaRotation();
            break;
        }
        case JPH::EConstraintSubType::SixDOF: {
            const auto* six = static_cast<const JPH::SixDOFConstraint*>(found->second.GetPtr());
            bool translationLocked = true;
            for (int axis = 0; axis < 3; ++axis) {
                const auto nativeAxis = static_cast<JPH::SixDOFConstraint::EAxis>(axis);
                translationLocked &= six->GetLimitsMin(nativeAxis) >= six->GetLimitsMax(nativeAxis) &&
                    six->GetLimitsSpringSettings(nativeAxis).mFrequency <= 0;
            }
            linear = translationLocked ? six->GetTotalLambdaPosition() :
                basis.first.Multiply3x3(six->GetTotalLambdaPosition());
            linear += basis.first.Multiply3x3(six->GetTotalLambdaMotorTranslation());
            bool locked = true;
            for (int axis = 3; axis < 6; ++axis)
                locked &= six->GetLimitsMin(static_cast<JPH::SixDOFConstraint::EAxis>(axis)) >=
                    six->GetLimitsMax(static_cast<JPH::SixDOFConstraint::EAxis>(axis));
            angular = locked ? six->GetTotalLambdaRotation() :
                basis.first.Multiply3x3(six->GetTotalLambdaRotation());
            angular += basis.second.Multiply3x3(six->GetTotalLambdaMotorRotation());
            break;
        }
        default: R_ASSERT2(false, "Unsupported native joint feedback type");
        }
        const auto force = linear / delta_time;
        const auto torque = angular / delta_time;
        Store(-force, output->f1);
        Store(force, output->f2);
        Store(-torque - basis.lever1.Cross(force), output->t1);
        Store(torque + basis.lever2.Cross(force), output->t2);
    }
}
