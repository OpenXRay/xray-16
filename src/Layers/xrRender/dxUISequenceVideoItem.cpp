#include "stdafx.h"
#include "dxUISequenceVideoItem.h"

namespace xray::render::fg
{
dxUISequenceVideoItem::dxUISequenceVideoItem()
    : m_texture(nullptr)
    , m_framegraph_mode(true)  // Always framegraph mode
{
}

void dxUISequenceVideoItem::Copy(IUISequenceVideoItem& _in)
{
    *this = *((dxUISequenceVideoItem*)&_in);
}

bool dxUISequenceVideoItem::HasTexture()
{
    // With framegraph, video texture is already loaded via TextureManager
    // and auto-updates every frame via UpdateVideoTextures()
    return true;
}

void dxUISequenceVideoItem::CaptureTexture()
{
    // Framegraph mode: Video texture is already loaded and auto-updating
    // We don't need to capture it from RCache
    m_framegraph_mode = true;
}

void dxUISequenceVideoItem::ResetTexture()
{
    m_texture = nullptr;
    m_framegraph_mode = true;
}

BOOL dxUISequenceVideoItem::video_IsPlaying()
{
    // With framegraph, video is always playing (auto-updates)
    return TRUE;
}

void dxUISequenceVideoItem::video_Sync(u32 _time)
{
    // With framegraph, video syncs automatically via UpdateVideoTextures()
}

void dxUISequenceVideoItem::video_Play(BOOL looped, u32 _time)
{
    // With framegraph, video starts playing automatically when loaded
}

void dxUISequenceVideoItem::video_Stop()
{
    // With framegraph, we don't stop videos (they auto-update)
}

} // namespace xray::render::fg
