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
        // With framegraph, video texture is already loaded via TextureManager
        // and auto-updates every frame via UpdateVideoTextures()
        return true;
    }
    return !!m_texture;
}

void dxUISequenceVideoItem::CaptureTexture()
{
    if (ps_r4_use_framegraph)
    {
        // Framegraph mode: Video texture is already loaded and auto-updating
        // We don't need to capture it from RCache
        m_framegraph_mode = true;
    }
    else
    {
        // Legacy mode: Capture from RCache
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
        // With framegraph, video is always playing (auto-updates)
        return TRUE;
    }
    return m_texture->video_IsPlaying();
}

void dxUISequenceVideoItem::video_Sync(u32 _time)
{
    if (m_framegraph_mode)
    {
        // With framegraph, video syncs automatically via UpdateVideoTextures()
        return;
    }
    m_texture->video_Sync(_time);
}

void dxUISequenceVideoItem::video_Play(BOOL looped, u32 _time)
{
    if (m_framegraph_mode)
    {
        // With framegraph, video starts playing automatically when loaded
        return;
    }
    m_texture->video_Play(looped, _time);
}

void dxUISequenceVideoItem::video_Stop()
{
    if (m_framegraph_mode)
    {
        // With framegraph, we don't stop videos (they auto-update)
        return;
    }
    m_texture->video_Stop();
}

} // namespace xray::render::RENDER_NAMESPACE
