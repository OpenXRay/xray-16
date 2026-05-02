#include "stdafx.h"
#include "fgUISequenceVideoItem.h"

namespace xray::render::fg
{
fgUISequenceVideoItem::fgUISequenceVideoItem()
    : m_texture(nullptr)
    , m_framegraph_mode(true)
{
}

void fgUISequenceVideoItem::Copy(IUISequenceVideoItem& _in)
{
    *this = *((fgUISequenceVideoItem*)&_in);
}

bool fgUISequenceVideoItem::HasTexture()
{
    return true;
}

void fgUISequenceVideoItem::CaptureTexture()
{
    m_framegraph_mode = true;
}

void fgUISequenceVideoItem::ResetTexture()
{
    m_texture = nullptr;
    m_framegraph_mode = true;
}

BOOL fgUISequenceVideoItem::video_IsPlaying()
{
    return TRUE;
}

void fgUISequenceVideoItem::video_Sync(u32 _time)
{
}

void fgUISequenceVideoItem::video_Play(BOOL looped, u32 _time)
{
}

void fgUISequenceVideoItem::video_Stop()
{
}

}
