#include "stdafx.h"
#include "FontRenderCollector.h"
#include "dxFontRender.h"

namespace xray::render::ui
{

void FontRenderCollector::BeginFont(dxFontRender* fontRender)
{
    m_currentFont = fontRender;

    m_currentBatch = nullptr;

    m_batches.emplace_back();
    m_currentBatch = &m_batches.back();

    m_currentBatch->uiShader = nullptr;
    m_currentBatch->shaderElement = 0;
    m_currentBatch->alphaRef = 0;
    m_currentBatch->hasScissor = false;
    m_currentBatch->cullMode = 0;
    m_currentBatch->primitiveType = UIPrimitiveType::TriList;
}

void FontRenderCollector::AddQuad(const UIVertex& v0, const UIVertex& v1, const UIVertex& v2, const UIVertex& v3)
{
    if (!m_currentBatch)
    {
        Msg("! [FontRenderCollector] AddQuad called without BeginFont");
        return;
    }

    u16 baseIndex = static_cast<u16>(m_currentBatch->vertices.size());

    m_currentBatch->vertices.push_back(v0);
    m_currentBatch->vertices.push_back(v1);
    m_currentBatch->vertices.push_back(v2);
    m_currentBatch->vertices.push_back(v3);

    m_currentBatch->indices.push_back(baseIndex + 0);
    m_currentBatch->indices.push_back(baseIndex + 1);
    m_currentBatch->indices.push_back(baseIndex + 2);

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

}
