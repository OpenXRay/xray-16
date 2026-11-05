#include "stdafx.h"
#include "JoltDebugRenderer.h"
#include "Include/xrAPI/xrAPI.h"
#include "Include/xrRender/DebugRender.h"

#ifdef JPH_DEBUG_RENDERER

CXRayJoltDebugRenderer::CXRayJoltDebugRenderer()
{
    // NOTE: Do NOT call Initialize() here!
    // DebugRendererSimple's constructor already calls Initialize(),
    // which sets up predefined shapes (mBox, mSphere, etc.).
    // Calling it twice would corrupt internal geometry caches.
}

CXRayJoltDebugRenderer::~CXRayJoltDebugRenderer()
{
}

void CXRayJoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    m_draw_line_count++;

    if (GEnv.DRender) {
        Fvector p1, p2;
        p1.set(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
        p2.set(inTo.GetX(), inTo.GetY(), inTo.GetZ());

        u32 color = color_rgba(inColor.r, inColor.g, inColor.b, inColor.a);

        Fvector vertices[2] = { p1, p2 };
        u16 indices[2] = { 0, 1 };

        GEnv.DRender->add_lines(vertices, 2, indices, 1, color);
    }
}

void CXRayJoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow)
{
    m_draw_tri_count++;

    // Convert solid triangles to wireframe lines for reliable X-Ray rendering
    DrawLine(inV1, inV2, inColor);
    DrawLine(inV2, inV3, inColor);
    DrawLine(inV3, inV1, inColor);
}

void CXRayJoltDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight)
{
    // Text drawing not implemented for physics debugging.
}

#endif // JPH_DEBUG_RENDERER
