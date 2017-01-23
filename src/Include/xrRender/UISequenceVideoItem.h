#ifndef	UISequenceVideoItem_included
#define	UISequenceVideoItem_included
#pragma once

class IUISequenceVideoItem
{
public:
	virtual ~IUISequenceVideoItem() {;}
	virtual void Copy(IUISequenceVideoItem &_in) = 0;

	virtual bool HasTexture() = 0;
	virtual void CaptureTexture() = 0;
	virtual void ResetTexture() = 0;
	virtual BOOL video_IsPlaying() = 0;
	virtual void video_Sync(u32 _time) = 0;
	virtual void video_Play(BOOL looped, u32 _time=0xFFFFFFFF) = 0;
	virtual void video_Stop() = 0;
};

#endif	//	UISequenceVideoItem_included
