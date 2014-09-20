#pragma once

#include "UIWindow.h"
#include "UIWndCallback.h"
#include "UIXmlInit.h"

class CUIFrameWindow;
class CUIFrameLineWnd;
class CUIAnimatedStatic;
class CUIMapWnd;
class CUI3tButton;
class CUITabControl;
class CGameTask;
class CUITaskDescrWnd;
class CUIScrollView;
class CUITaskItem;

class CUIEventsWnd	:public CUIWindow, public CUIWndCallback{
	typedef CUIWindow			inherited;
	enum ETaskFilters{	eActiveTask			=	0,
						eAccomplishedTask,
						eFailedTask,
//.						eOwnTask,
						eMaxTask};
	enum EEventWndFlags{
						flNeedReload	=(1<<0),
						flMapMode		=(1<<1),
	};
	Flags16						m_flags;
	ETaskFilters				m_currFilter;
	CUIFrameWindow*				m_UILeftFrame;
	CUIWindow*					m_UIRightWnd;
	CUIFrameLineWnd*			m_UILeftHeader;
	CUIAnimatedStatic*			m_UIAnimation;
	CUIMapWnd*					m_UIMapWnd;
	CUITaskDescrWnd*			m_UITaskInfoWnd;
	CUIScrollView*				m_ListWnd;
	CUITabControl*				m_TaskFilter;

	bool						Filter					(CGameTask* t);
	void __stdcall				OnFilterChanged			(CUIWindow*,void*);
	void						ReloadList				(bool bClearOnly);

public:
	void						SetDescriptionMode		(bool bMap);
	bool						GetDescriptionMode		();
	void						ShowDescription			(CGameTask* t, int idx);
	bool						ItemHasDescription		(CUITaskItem*);
public:

								CUIEventsWnd			();
	virtual						~CUIEventsWnd			();
	virtual void				SendMessage				(CUIWindow* pWnd, s16 msg, void* pData);
			void				Init					();
	virtual void				Update					();
	virtual void				Draw					();
	virtual void				Show					(bool status);
			void				Reload					();
	virtual void				Reset					();

	CUIXml						m_ui_task_item_xml;

};
