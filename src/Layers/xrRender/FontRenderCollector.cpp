// xrRender/FontRenderCollector.cpp
#include "stdafx.h"
#include "FontRenderCollector.h"
#include "dxFontRender.h"

namespace xray::render::ui
{

void FontRenderCollector::BeginFont(dxFontRender* fontRender)
{
    m_currentFont = fontRender;

    // Create or reuse a batch for this font
    // Fonts with the same texture/shader can share a batch
    m_currentBatch = nullptr;

    // Try to find existing batch with same shader
    // For now, just create a new batch per font - we can optimize batching later
    m_batches.emplace_back();
    m_currentBatch = &m_batches.back();

    // Set batch state from font
    m_currentBatch->uiShader = nullptr;  // Font doesn't have IUIShader interface
    m_currentBatch->shaderElement = 0;   // Always 0 for fonts
    m_currentBatch->alphaRef = 0;        // No alpha test for fonts (uses alpha blending)
    m_currentBatch->hasScissor = false;  // No scissor for fonts
    m_currentBatch->cullMode = 0;        // No culling for fonts
    m_currentBatch->primitiveType = UIPrimitiveType::TriList;
}

void FontRenderCollector::AddQuad(const UIVertex& v0, const UIVertex& v1, const UIVertex& v2, const UIVertex& v3)
{
    if (!m_currentBatch)
    {
        Msg("! [FontRenderCollector] AddQuad called without BeginFont");
        return;
    }

    // Add 4 vertices (quad corners)
    u16 baseIndex = static_cast<u16>(m_currentBatch->vertices.size());

    m_currentBatch->vertices.push_back(v0);
    m_currentBatch->vertices.push_back(v1);
    m_currentBatch->vertices.push_back(v2);
    m_currentBatch->vertices.push_back(v3);

    // Add 6 indices (2 triangles forming a quad)
    // Triangle 1: v0, v1, v2
    m_currentBatch->indices.push_back(baseIndex + 0);
    m_currentBatch->indices.push_back(baseIndex + 1);
    m_currentBatch->indices.push_back(baseIndex + 2);

    // Triangle 2: v0, v2, v3
    m_currentBatch->indices.push_back(baseIndex + 0);
    m_currentBatch->indices.push_back(baseIndex + 2);
    m_currentBatch->indices.push_back(baseIndex + 3);
}

void FontRenderCollector::EndFont()
{
    m_currentFont = nullptr;
    m_currentBatch = nullptr;
}

void FontRenderCollector::Clear()
{
    m_batches.clear();
    m_currentBatch = nullptr;
    m_currentFont = nullptr;
}

} // namespace xray::render::ui
