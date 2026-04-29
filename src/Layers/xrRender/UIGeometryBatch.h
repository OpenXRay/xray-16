// xrRender/UIGeometryBatch.h
#pragma once

#include "xrCore/xr_types.h"
#include "xrCommon/xr_vector.h"
#include "Layers/xrRender/Shader.h"

namespace xray::render::ui
{
using namespace xray::render::fg;  // For HW

// Vertex format for UI rendering - matches FVF::TL and FVF::LIT
// Memory layout matches vanilla (28 bytes total):
// - POSITIONT: float4 at offset 0 (16 bytes) - shader expects RGBA32_FLOAT
// - COLOR: u32 at offset 16 (4 bytes) - RGBA8_UNORM
// - TEXCOORD: float2 at offset 20 (8 bytes) - RG32_FLOAT
struct UIVertex
{
    float x, y, z, w;  // Position (w=1.0 for homogeneous coords) - POSITIONT as float4!
    u32 color;         // RGBA color (packed DWORD)
    float u, v;        // Texture coordinates

    void set(float _x, float _y, float _z, u32 _color, float _u, float _v)
    {
        x = _x;
        y = _y;
        z = _z;
        w = 1.0f;      // Homogeneous coordinate
        color = _color;
        u = _u;
        v = _v;
    }
};

// Primitive type - matches IUIRender::ePrimitiveType
enum class UIPrimitiveType
{
    TriList,
    TriStrip,
    LineStrip,
    LineList
};

// A batch of UI geometry with the same shader/texture
class UIGeometryBatch
{
public:
    UIGeometryBatch() = default;
    ~UIGeometryBatch() = default;

    // Geometry data
    xr_vector<UIVertex> vertices;
    xr_vector<u16> indices;
    UIPrimitiveType primitiveType{UIPrimitiveType::TriList};

    // Rendering state
    IUIShader* uiShader{nullptr};  // Backend-agnostic: Pointer to shader (dxUIShader has NVRHI handles)
    u32 shaderElement{0};       // Shader element index (always 0 for UI = SE_R2_NORMAL_HQ)
    int alphaRef{0};            // Alpha reference value
    bool hasScissor{false};     // Whether scissor rect is active
    Irect scissorRect;          // Scissor rectangle
    Fmatrix xformWorld;         // World transform matrix
    int cullMode{0};            // Cull mode (0=none, 1=CW, 2=CCW)

    void Clear()
    {
        vertices.clear();
        indices.clear();
        uiShader = nullptr;
        alphaRef = 0;
        hasScissor = false;
        cullMode = 0;
    }

    bool IsEmpty() const
    {
        return vertices.empty();
    }

    // Add a primitive to this batch
    void AddPrimitive(const xr_vector<UIVertex>& verts, UIPrimitiveType primType)
    {
        primitiveType = primType;

        u16 baseIndex = static_cast<u16>(vertices.size());

        // Add vertices
        vertices.insert(vertices.end(), verts.begin(), verts.end());

        // Generate indices based on primitive type
        switch (primType)
        {
        case UIPrimitiveType::TriList:
            // Triangle list: every 3 vertices form a triangle
            for (u16 i = 0; i < verts.size(); i += 3)
            {
                if (i + 2 < verts.size())
                {
                    indices.push_back(baseIndex + i);
                    indices.push_back(baseIndex + i + 1);
                    indices.push_back(baseIndex + i + 2);
                }
            }
            break;

        case UIPrimitiveType::TriStrip:
            // Triangle strip: convert to indexed triangles
            for (u16 i = 0; i + 2 < verts.size(); ++i)
            {
                if (i % 2 == 0)
                {
                    indices.push_back(baseIndex + i);
                    indices.push_back(baseIndex + i + 1);
                    indices.push_back(baseIndex + i + 2);
                }
                else
                {
                    indices.push_back(baseIndex + i);
                    indices.push_back(baseIndex + i + 2);
                    indices.push_back(baseIndex + i + 1);
                }
            }
            break;

        case UIPrimitiveType::LineList:
            // Line list: every 2 vertices form a line
            for (u16 i = 0; i + 1 < verts.size(); i += 2)
            {
                indices.push_back(baseIndex + i);
                indices.push_back(baseIndex + i + 1);
            }
            break;

        case UIPrimitiveType::LineStrip:
            // Line strip: consecutive vertices form lines
            for (u16 i = 0; i + 1 < verts.size(); ++i)
            {
                indices.push_back(baseIndex + i);
                indices.push_back(baseIndex + i + 1);
            }
            break;
        }
    }

    // Check if this batch can be merged with given state
    bool CanMergeWith(IUIShader* otherShader, int otherAlphaRef, bool otherScissor,
                      const Irect* otherScissorRect, int otherCullMode) const
    {
        if (uiShader != otherShader)
            return false;
        if (alphaRef != otherAlphaRef)
            return false;
        if (hasScissor != otherScissor)
            return false;
        if (hasScissor && otherScissor)
        {
            if (memcmp(&scissorRect, otherScissorRect, sizeof(Irect)) != 0)
                return false;
        }
        if (cullMode != otherCullMode)
            return false;

        return true;
    }
};

} // namespace xray::render::ui
