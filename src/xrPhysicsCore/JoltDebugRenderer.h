#pragma once
#include <Jolt/Jolt.h>
#include "xrCore/xrCore.h"

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>

class CXRayJoltDebugRenderer final : public JPH::DebugRendererSimple {
public:
    CXRayJoltDebugRenderer();
    virtual ~CXRayJoltDebugRenderer() override;

    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;
    virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;

    u32 m_draw_line_count = 0;
    u32 m_draw_tri_count = 0;
    u32 m_draw_geom_count = 0;
};

#endif // JPH_DEBUG_RENDERER
