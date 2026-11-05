# Generate one extended translation unit in the build directory. The fetched
# upstream source stays untouched, including when another worktree shares it.
get_target_property(jolt_sources Jolt SOURCES)
set(contact_manager_source "")
foreach(source IN LISTS jolt_sources)
    if(source MATCHES "/ContactConstraintManager\\.cpp$")
        set(contact_manager_source "${source}")
    endif()
endforeach()
if(NOT EXISTS "${contact_manager_source}")
    message(FATAL_ERROR "Cannot find the pinned Jolt contact manager")
endif()
file(READ "${contact_manager_source}" contact_manager)
set(hook [=[
		table[(int)constraint.mBody1->GetMotionType()][(int)constraint.mBody2->GetMotionType()](constraint, *mWriteCache);
]=])
string(FIND "${contact_manager}" "${hook}" hook_offset)
if(hook_offset EQUAL -1)
    message(FATAL_ERROR "Jolt contact feedback hook does not match the pinned revision")
endif()
set(callback [=[
static void* sXRayContactContext = nullptr;
static XRayContactImpulseCallback sXRayContactImpulseCallback = nullptr;
void SetXRayContactImpulseCallback(void* context, XRayContactImpulseCallback callback)
{
	sXRayContactContext = context;
	sXRayContactImpulseCallback = callback;
}
]=])
string(REPLACE "JPH_NAMESPACE_BEGIN" "#include \"XRayContactFeedback.h\"\nJPH_NAMESPACE_BEGIN\n${callback}" contact_manager "${contact_manager}")
set(emit [=[
		if (sXRayContactImpulseCallback != nullptr)
		{
			const auto* entry = mWriteCache->FromHandle(constraint.mCachedManifoldHandle);
			const auto& manifold = entry->GetValue();
			const auto& pair = entry->GetKey();
			const Body& first = *constraint.mBody1;
			const Body& second = *constraint.mBody2;
			const auto first_transform = first.GetCenterOfMassTransform();
			const auto second_transform = second.GetCenterOfMassTransform();
			const Vec3 normal = constraint.GetWorldSpaceNormal();
			Vec3 tangent1, tangent2;
			constraint.GetTangents(tangent1, tangent2);
			RVec3 friction_center = RVec3::sZero();
			for (uint32 point = 0; point < manifold.mNumContactPoints; ++point)
			{
				const auto& cached = manifold.mContactPoints[point];
				const RVec3 position1 = first_transform * Vec3::sLoadFloat3Unsafe(cached.mPosition1);
				const RVec3 position2 = second_transform * Vec3::sLoadFloat3Unsafe(cached.mPosition2);
				friction_center += 0.5_r * (position1 + position2);
				sXRayContactImpulseCallback(sXRayContactContext, first, pair.GetSubShapeID1(), second, pair.GetSubShapeID2(),
					position1, position2, normal * cached.mNonPenetrationLambda, Vec3::sZero());
			}
			friction_center /= Real(manifold.mNumContactPoints);
			const Vec3 friction = tangent1 * manifold.mFrictionLambda[0] + tangent2 * manifold.mFrictionLambda[1];
			sXRayContactImpulseCallback(sXRayContactContext, first, pair.GetSubShapeID1(), second, pair.GetSubShapeID2(),
				friction_center, friction_center, friction, normal * manifold.mAngularFrictionLambda);
		}
]=])
string(REPLACE "${hook}" "${hook}${emit}" contact_manager "${contact_manager}")
set(extended_source "${CMAKE_CURRENT_BINARY_DIR}/xray-contact-feedback/ContactConstraintManager.cpp")
file(CONFIGURE OUTPUT "${extended_source}" CONTENT "${contact_manager}" @ONLY)
list(REMOVE_ITEM jolt_sources "${contact_manager_source}")
list(APPEND jolt_sources "${extended_source}")
set_property(TARGET Jolt PROPERTY SOURCES "${jolt_sources}")
target_include_directories(Jolt PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
