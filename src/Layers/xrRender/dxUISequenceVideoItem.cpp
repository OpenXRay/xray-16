#include "stdafx.h"
#include "dxUISequenceVideoItem.h"

namespace xray::render::RENDER_NAMESPACE
{
dxUISequenceVideoItem::dxUISequenceVideoItem()
    : m_texture(nullptr)
    , m_framegraph_mode(true)
{
}

void dxUISequenceVideoItem::Copy(IUISequenceVideoItem& _in)
{
    *this = *((dxUISequenceVideoItem*)&_in);
}

bool dxUISequenceVideoItem::HasTexture()
{
    return true;
}

void dxUISequenceVideoItem::CaptureTexture()
{
    m_framegraph_mode = true;
}

void dxUISequenceVideoItem::ResetTexture()
{
    m_texture = nullptr;
    m_framegraph_mode = true;
}

BOOL dxUISequenceVideoItem::video_IsPlaying()
{
    return TRUE;
}

void dxUISequenceVideoItem::video_Sync(u32 _time)
{
}

void dxUISequenceVideoItem::video_Play(BOOL looped, u32 _time)
{
}

void dxUISequenceVideoItem::video_Stop()
{
}

} // namespace xray::render::RENDER_NAMESPACE
