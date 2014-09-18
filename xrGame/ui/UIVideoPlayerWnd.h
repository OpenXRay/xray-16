#pragma once
#include "UIDialogWnd.h"
#include "UIWndCallback.h"

class CUITabControl;
class CUIStatic;
class CUIXml;

class CUIVideoPlayerWnd :public CUIDialogWnd, public CUIWndCallback
{
	typedef CUIDialogWnd inherited;
	CUITabControl*	m_tabControl;
	CUIStatic*		m_surface;
protected:
	ref_sound		m_sound;
// Igor	ref_texture		m_texture;
	shared_str		m_fn;
	Flags8			m_flags;
	enum			{eStoping=(1<<0),ePlaying=(1<<1),eLooped=(1<<2)};
private:
	void			OnBtnPlayClicked		();
	void			OnBtnPauseClicked		();
	void __stdcall	OnTabChanged			(CUIWindow* pWnd, void* pData);
public:
	void			Init					(CUIXml* doc, LPCSTR start_from);
	void			Init					(LPCSTR name);
	virtual void	SendMessage				(CUIWindow* pWnd, s16 msg, void* pData);
	void			SetFile					(LPCSTR fn);

	virtual void	Draw					();
	virtual void	Update					();
			void	Play					();
			void	Stop					();
			bool	IsPlaying				();
};

class CUIActorSleepVideoPlayer : public CUIVideoPlayerWnd
{
	typedef CUIVideoPlayerWnd inherited;
public:
			void	Activate				();
			void	DeActivate				();
	virtual bool	NeedCursor				() const {return false;}
	virtual bool OnKeyboard					(int dik, EUIMessages keyboard_action);
};