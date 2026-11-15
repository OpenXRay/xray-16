#include "stdafx.h"
#include "JoltPhysicsCore.h"
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Geometry/RayAABox.h>

const JPH::Shape* JoltPhysicsCore::CharacterPairCollision::PairShape(
    const JPH::CharacterVirtual* character, const JPH::CharacterVirtual* other) const
{
    if (m_core->m_character_pair_shape) {
        auto object = [this](const JPH::CharacterVirtual* pointer) -> void* {
            for (const auto& [handle, candidate] : m_core->m_characters) {
                if (candidate.GetPtr() != pointer) continue;
                const auto entry = m_core->m_character_callbacks.find(handle);
                return entry == m_core->m_character_callbacks.end() ? nullptr : entry->second.user_data;
            }
            return nullptr;
        };
        if (auto* shape = m_core->m_character_pair_shape(object(character), object(other)))
            return static_cast<const JPH::Shape*>(shape);
    }
    return character->GetShape();
}

// Uses Jolt's CharacterVsCharacterCollisionSimple traversal and collision
// dispatch, with a pair-specific shape and its corresponding center offset.
void JoltPhysicsCore::CharacterPairCollision::CollideCharacter(const JPH::CharacterVirtual* character,
    JPH::RMat44Arg centerTransform, const JPH::CollideShapeSettings& settings,
    JPH::RVec3Arg baseOffset, JPH::CollideShapeCollector& collector) const
{
    const auto originalTransform = centerTransform.PostTranslated(-baseOffset).ToMat44();
    for (const auto* other : mCharacters) {
        if (other == character || collector.ShouldEarlyOut()) continue;
        const auto* first = PairShape(character, other);
        const auto* second = PairShape(other, character);
        const auto transform1 = originalTransform.PreTranslated(first->GetCenterOfMass() - character->GetShape()->GetCenterOfMass());
        const auto transform2 = other->GetCenterOfMassTransform().PostTranslated(-baseOffset).ToMat44()
            .PreTranslated(second->GetCenterOfMass() - other->GetShape()->GetCenterOfMass());
        auto pairSettings = settings;
        pairSettings.mMaxSeparationDistance += other->GetCharacterPadding();
        auto bounds2 = second->GetWorldSpaceBounds(transform2, JPH::Vec3::sOne());
        bounds2.ExpandBy(JPH::Vec3::sReplicate(pairSettings.mMaxSeparationDistance));
        if (!first->GetWorldSpaceBounds(transform1, JPH::Vec3::sOne()).Overlaps(bounds2)) continue;
        collector.SetUserData(reinterpret_cast<JPH::uint64>(other));
        JPH::CollisionDispatch::sCollideShapeVsShape(first, second, JPH::Vec3::sOne(), JPH::Vec3::sOne(),
            transform1, transform2, JPH::SubShapeIDCreator(), JPH::SubShapeIDCreator(), pairSettings, collector);
    }
    collector.SetUserData(0);
}

void JoltPhysicsCore::CharacterPairCollision::CastCharacter(const JPH::CharacterVirtual* character,
    JPH::RMat44Arg centerTransform, JPH::Vec3Arg direction, const JPH::ShapeCastSettings& settings,
    JPH::RVec3Arg baseOffset, JPH::CastShapeCollector& collector) const
{
    const auto originalTransform = centerTransform.PostTranslated(-baseOffset).ToMat44();
    for (const auto* other : mCharacters) {
        if (other == character || collector.ShouldEarlyOut()) continue;
        const auto* first = PairShape(character, other);
        const auto* second = PairShape(other, character);
        const auto transform1 = originalTransform.PreTranslated(first->GetCenterOfMass() - character->GetShape()->GetCenterOfMass());
        const auto transform2 = other->GetCenterOfMassTransform().PostTranslated(-baseOffset).ToMat44()
            .PreTranslated(second->GetCenterOfMass() - other->GetShape()->GetCenterOfMass());
        const JPH::ShapeCast cast(first, JPH::Vec3::sOne(), transform1, direction);
        const auto extents = cast.mShapeWorldBounds.GetExtent() + JPH::Vec3::sReplicate(settings.mExtraConvexRadius);
        auto bounds2 = second->GetWorldSpaceBounds(transform2, JPH::Vec3::sOne());
        bounds2.ExpandBy(extents + JPH::Vec3::sReplicate(other->GetCharacterPadding()));
        if (!JPH::RayAABoxHits(cast.mShapeWorldBounds.GetCenter(), direction, bounds2.mMin, bounds2.mMax)) continue;
        auto pairSettings = settings;
        pairSettings.mExtraConvexRadius += other->GetCharacterPadding();
        collector.SetUserData(reinterpret_cast<JPH::uint64>(other));
        JPH::CollisionDispatch::sCastShapeVsShapeWorldSpace(cast, pairSettings, second, JPH::Vec3::sOne(), {},
            transform2, JPH::SubShapeIDCreator(), JPH::SubShapeIDCreator(), collector);
    }
    collector.SetUserData(0);
}
