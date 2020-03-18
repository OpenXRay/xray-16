#pragma once
class XRayUISequenceVideoItem:public IUISequenceVideoItem
{
public:
	XRayUISequenceVideoItem();
	virtual void Copy(IUISequenceVideoItem &_in);

	virtual bool HasTexture();
	virtual void CaptureTexture();
	virtual void ResetTexture() ;
	virtual BOOL video_IsPlaying() ;
	virtual void video_Sync(u32 _time);;
	virtual void video_Play(BOOL looped, u32 _time = 0xFFFFFFFF);;
	virtual void video_Stop();
};
