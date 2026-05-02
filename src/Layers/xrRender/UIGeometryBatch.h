#pragma once

#include "xrCore/xr_types.h"
#include "xrCommon/xr_vector.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/fgUIShader.h"

namespace xray::render::ui
{
using namespace xray::render::fg;

struct UIVertex
{
    float x, y, z, w;
    u32 color;
    float u, v;
    u32 texIndex;

    void set(float _x, float _y, float _z, u32 _color, float _u, float _v, u32 _texIndex)
    {
        x = _x;
        y = _y;
        z = _z;
        w = 1.0f;
        color = _color;
        u = _u;
        v = _v;
        texIndex = _texIndex;
    }
};

enum class UIPrimitiveType
{
    TriList,
    TriStrip,
    LineStrip,
    LineList
};

class UIGeometryBatch
{
public:
    UIGeometryBatch() = default;
    ~UIGeometryBatch() = default;

    xr_vector<UIVertex> vertices;
    xr_vector<u16> indices;
    UIPrimitiveType primitiveType{UIPrimitiveType::TriList};

    IUIShader* uiShader{nullptr};
    u32 shaderElement{0};
    int alphaRef{0};
    bool hasScissor{false};
    Irect scissorRect;
    Fmatrix xformWorld;
    int cullMode{0};

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

    void AddPrimitive(const xr_vector<UIVertex>& verts, UIPrimitiveType primType)
    {
        primitiveType = primType;

        u16 baseIndex = static_cast<u16>(vertices.size());

        vertices.insert(vertices.end(), verts.begin(), verts.end());

        switch (primType)
        {
        case UIPrimitiveType::TriList:
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
            for (u16 i = 0; i + 1 < verts.size(); i += 2)
            {
                indices.push_back(baseIndex + i);
                indices.push_back(baseIndex + i + 1);
            }
            break;

        case UIPrimitiveType::LineStrip:
            for (u16 i = 0; i + 1 < verts.size(); ++i)
            {
                indices.push_back(baseIndex + i);
                indices.push_back(baseIndex + i + 1);
            }
            break;
        }
    }

    bool CanMergeWith(IUIShader* incomingShader, int incomingAlphaRef, bool incomingScissor,
                      const Irect* incomingScissorRect, int incomingCullMode) const
    {
        if (alphaRef != incomingAlphaRef) return false;
        if (hasScissor != incomingScissor) return false;
        if (hasScissor && incomingScissor) {
            if (memcmp(&scissorRect, incomingScissorRect, sizeof(Irect)) != 0)
                return false;
        }
        if (cullMode != incomingCullMode) return false;

        auto* current = static_cast<fgUIShader*>(uiShader);
        auto* incoming = static_cast<fgUIShader*>(incomingShader);
        if (!current || !incoming) return uiShader == incomingShader;
        return current->SamePipelineAs(*incoming);
    }
};

}
