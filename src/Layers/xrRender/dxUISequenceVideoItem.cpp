#include "stdafx.h"
#include "dxUISequenceVideoItem.h"

extern ENGINE_API int ps_r4_use_framegraph;

namespace xray::render::RENDER_NAMESPACE
{
dxUISequenceVideoItem::dxUISequenceVideoItem()
    : m_texture(nullptr)
    , m_framegraph_mode(false)
{
}

void dxUISequenceVideoItem::Copy(IUISequenceVideoItem& _in)
{
    *this = *((dxUISequenceVideoItem*)&_in);
}

bool dxUISequenceVideoItem::HasTexture()
{
    if (m_framegraph_mode)
    {
        return true;
    }
    return !!m_texture;
}

void dxUISequenceVideoItem::CaptureTexture()
{
    if (ps_r4_use_framegraph)
    {
        m_framegraph_mode = true;
    }
    else
    {
        R_constant* C = RCache.get_c(c_sbase)._get();
        m_texture = RCache.get_ActiveTexture(C ? C->samp.index : 0);
        R_ASSERT(m_texture);
        m_framegraph_mode = false;
    }
}

void dxUISequenceVideoItem::ResetTexture()
{
    m_texture = nullptr;
    m_framegraph_mode = false;
}

BOOL dxUISequenceVideoItem::video_IsPlaying()
{
    if (m_framegraph_mode)
    {
        return TRUE;
    }
    return m_texture->video_IsPlaying();
}

void dxUISequenceVideoItem::video_Sync(u32 _time)
{
    if (m_framegraph_mode)
    {
        return;
    }
    m_texture->video_Sync(_time);
}

void dxUISequenceVideoItem::video_Play(BOOL looped, u32 _time)
{
    if (m_framegraph_mode)
    {
        return;
    }
    m_texture->video_Play(looped, _time);
}

void dxUISequenceVideoItem::video_Stop()
{
    if (m_framegraph_mode)
    {
        return;
    }
    m_texture->video_Stop();
}

} // namespace xray::render::RENDER_NAMESPACE
