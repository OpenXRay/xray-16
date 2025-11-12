#pragma once

#include "Include/xrRender/UISequenceVideoItem.h"

namespace xray::render::RENDER_NAMESPACE
{
class dxUISequenceVideoItem : public IUISequenceVideoItem
{
public:
    dxUISequenceVideoItem();
    virtual void Copy(IUISequenceVideoItem& _in);

    virtual bool HasTexture();
    virtual void CaptureTexture();
    virtual void ResetTexture();
    virtual BOOL video_IsPlaying();
    virtual void video_Sync(u32 _time);
    virtual void video_Play(BOOL looped, u32 _time = 0xFFFFFFFF);
    virtual void video_Stop();
private:
    CTexture* m_texture;
    bool m_framegraph_mode;  // True when using framegraph (texture auto-updates)
};
} // namespace xray::render::RENDER_NAMESPACE
