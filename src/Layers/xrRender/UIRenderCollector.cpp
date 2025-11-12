// xrRender/UIRenderCollector.cpp
#include "stdafx.h"
#include "UIRenderCollector.h"
#include "dxUIShader.h"

namespace xray::render::ui
{

UIRenderCollector::UIRenderCollector()
{
    m_xformWorld.identity();
}

void UIRenderCollector::CreateUIGeom()
{
    // No-op for collector - we don't create actual GPU geometry
}

void UIRenderCollector::DestroyUIGeom()
{
    // No-op for collector
}

void UIRenderCollector::SetShader(IUIShader& shader)
{
    dxUIShader* pShader = static_cast<dxUIShader*>(&shader);
    VERIFY(pShader);
    VERIFY(pShader->hShader);
    m_currentShader = pShader->hShader;
}

void UIRenderCollector::SetAlphaRef(int aref)
{
    m_currentAlphaRef = aref;
}

void UIRenderCollector::SetScissor(Irect* rect)
{
    if (rect)
    {
        m_hasScissor = true;
        m_scissorRect = *rect;
    }
    else
    {
        m_hasScissor = false;
    }
}

void UIRenderCollector::PushPoint(float x, float y, float z, u32 C, float u, float v)
{
    VERIFY(m_primitiveType != ptNone);
    VERIFY(m_currentVertices.size() < m_maxVerts);

    UIVertex vert;
    vert.set(x, y, z, C, u, v);
    m_currentVertices.push_back(vert);
}

void UIRenderCollector::StartPrimitive(u32 iMaxVerts, ePrimitiveType primType, ePointType pointType)
{
    VERIFY(m_primitiveType == ptNone);
    VERIFY(m_pointType == pttNone);

    m_maxVerts = iMaxVerts;
    m_primitiveType = primType;
    m_pointType = pointType;
    m_currentVertices.clear();
    m_currentVertices.reserve(iMaxVerts);
}

void UIRenderCollector::FlushPrimitive()
{
    if (m_currentVertices.empty())
    {
        m_primitiveType = ptNone;
        m_pointType = pttNone;
        return;
    }

    // Get or create a batch that can hold this primitive
    UIGeometryBatch* batch = GetOrCreateBatch();
    VERIFY(batch);

    // Add the primitive to the batch
    UIPrimitiveType uiPrimType = ConvertPrimitiveType(m_primitiveType);
    batch->AddPrimitive(m_currentVertices, uiPrimType);

    // Reset state
    m_currentVertices.clear();
    m_primitiveType = ptNone;
    m_pointType = pttNone;
}

LPCSTR UIRenderCollector::UpdateShaderName(LPCSTR tex_name, LPCSTR sh_name)
{
    // Same logic as dxUIRender
    string_path buff;
    u32 v_dev = CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
    u32 v_need = CAP_VERSION(2, 0);

    if ((v_dev >= v_need) && FS.exist(buff, "$game_textures$", tex_name, ".ogm"))
        return "hud" DELIMITER "movie";
    else
        return sh_name;
}

void UIRenderCollector::CacheSetXformWorld(const Fmatrix& M)
{
    m_xformWorld = M;
}

void UIRenderCollector::CacheSetCullMode(CullMode mode)
{
    m_cullMode = static_cast<int>(mode);
}

void UIRenderCollector::Clear()
{
    m_batches.clear();
    m_currentBatch = nullptr;
    m_currentVertices.clear();
    m_primitiveType = ptNone;
    m_pointType = pttNone;
    m_currentShader = nullptr;
    m_currentAlphaRef = 0;
    m_hasScissor = false;
    m_cullMode = 0;
    m_xformWorld.identity();
}

UIPrimitiveType UIRenderCollector::ConvertPrimitiveType(ePrimitiveType primType)
{
    switch (primType)
    {
    case ptTriList:
        return UIPrimitiveType::TriList;
    case ptTriStrip:
        return UIPrimitiveType::TriStrip;
    case ptLineStrip:
        return UIPrimitiveType::LineStrip;
    case ptLineList:
        return UIPrimitiveType::LineList;
    default:
        VERIFY(!"Unknown primitive type");
        return UIPrimitiveType::TriList;
    }
}

UIGeometryBatch* UIRenderCollector::GetOrCreateBatch()
{
    // Try to reuse the last batch if it has compatible state
    if (!m_batches.empty())
    {
        UIGeometryBatch& lastBatch = m_batches.back();
        if (lastBatch.CanMergeWith(m_currentShader, m_currentAlphaRef, m_hasScissor,
                                   m_hasScissor ? &m_scissorRect : nullptr, m_cullMode))
        {
            return &lastBatch;
        }
    }

    // Create a new batch
    m_batches.emplace_back();
    UIGeometryBatch& newBatch = m_batches.back();

    // Set state
    newBatch.shader = m_currentShader;
    newBatch.alphaRef = m_currentAlphaRef;
    newBatch.hasScissor = m_hasScissor;
    if (m_hasScissor)
        newBatch.scissorRect = m_scissorRect;
    newBatch.xformWorld = m_xformWorld;
    newBatch.cullMode = m_cullMode;

    return &newBatch;
}

} // namespace xray::render::ui
