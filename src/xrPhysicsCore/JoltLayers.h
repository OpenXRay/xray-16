#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer RAGDOLL = 2;
    static constexpr JPH::ObjectLayer MOVING_NO_STATIC = 3;
    static constexpr JPH::ObjectLayer NO_COLLISION = 4;
    static constexpr JPH::ObjectLayer STATIC_GEOMETRY = 5;
    static constexpr JPH::uint NUM_LAYERS = 6;
}

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::BroadPhaseLayer RAGDOLL(2);
    static constexpr JPH::BroadPhaseLayer MOVING_NO_STATIC(3);
    static constexpr JPH::BroadPhaseLayer NO_COLLISION(4);
    static constexpr JPH::uint NUM_LAYERS(5);
}


class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        m_object_to_broad_phase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_object_to_broad_phase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        m_object_to_broad_phase[Layers::RAGDOLL] = BroadPhaseLayers::RAGDOLL;
        m_object_to_broad_phase[Layers::MOVING_NO_STATIC] = BroadPhaseLayers::MOVING_NO_STATIC;
        m_object_to_broad_phase[Layers::NO_COLLISION] = BroadPhaseLayers::NO_COLLISION;
        m_object_to_broad_phase[Layers::STATIC_GEOMETRY] = BroadPhaseLayers::NON_MOVING;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return m_object_to_broad_phase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch ((JPH::BroadPhaseLayer::Type)inLayer) {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:     return "MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::RAGDOLL:    return "RAGDOLL";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING_NO_STATIC:    return "MOVING_NO_STATIC";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NO_COLLISION:        return "NO_COLLISION";
            default:                                                       JPH_ASSERT(false); return "INVALID";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer m_object_to_broad_phase[Layers::NUM_LAYERS];
};


class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case Layers::NON_MOVING:
            case Layers::STATIC_GEOMETRY:
                return inLayer2 == BroadPhaseLayers::MOVING || inLayer2 == BroadPhaseLayers::RAGDOLL;
            case Layers::MOVING:
                return inLayer2 != BroadPhaseLayers::NO_COLLISION;
            case Layers::RAGDOLL:
                return inLayer2 != BroadPhaseLayers::NO_COLLISION;
            case Layers::MOVING_NO_STATIC:
                return inLayer2 != BroadPhaseLayers::NON_MOVING && inLayer2 != BroadPhaseLayers::NO_COLLISION;
            case Layers::NO_COLLISION:
                return false;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
    switch (inObject1) {
        case Layers::NON_MOVING:
        case Layers::STATIC_GEOMETRY:
            return inObject2 == Layers::MOVING || inObject2 == Layers::RAGDOLL;
        case Layers::MOVING:
            return inObject2 != Layers::NO_COLLISION;
        case Layers::RAGDOLL:
            return inObject2 != Layers::NO_COLLISION;
        case Layers::MOVING_NO_STATIC:
            return inObject2 != Layers::NON_MOVING && inObject2 != Layers::STATIC_GEOMETRY && inObject2 != Layers::NO_COLLISION;
        case Layers::NO_COLLISION:
            return false;
        default:
            JPH_ASSERT(false);
            return false;
    }
}
};
