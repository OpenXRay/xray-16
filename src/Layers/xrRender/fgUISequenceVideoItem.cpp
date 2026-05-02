#include "stdafx.h"
#include "fgUISequenceVideoItem.h"

namespace xray::render::fg
{
fgUISequenceVideoItem::fgUISequenceVideoItem()
    : m_texture(nullptr)
    , m_framegraph_mode(true)  // Always framegraph mode
{
}

void fgUISequenceVideoItem::Copy(IUISequenceVideoItem& _in)
{
    *this = *((fgUISequenceVideoItem*)&_in);
}

bool fgUISequenceVideoItem::HasTexture()
{
    // With framegraph, video texture is already loaded via TextureManager
    // and auto-updates every frame via UpdateVideoTextures()
    return true;
}

void fgUISequenceVideoItem::CaptureTexture()
{
    // Framegraph mode: Video texture is already loaded and auto-updating
    // We don't need to capture it from RCache
    m_framegraph_mode = true;
}

void fgUISequenceVideoItem::ResetTexture()
{
    m_texture = nullptr;
    m_framegraph_mode = true;
}

BOOL fgUISequenceVideoItem::video_IsPlaying()
{
    // With framegraph, video is always playing (auto-updates)
    return TRUE;
}

void fgUISequenceVideoItem::video_Sync(u32 _time)
{
    // With framegraph, video syncs automatically via UpdateVideoTextures()
}

void fgUISequenceVideoItem::video_Play(BOOL looped, u32 _time)
{
    // With framegraph, video starts playing automatically when loaded
}

void fgUISequenceVideoItem::video_Stop()
{
    // With framegraph, we don't stop videos (they auto-update)
}

} // namespace xray::render::fg
