#ifndef UI_SKIN_SELECTOR_H_
#define UI_SKIN_SELECTOR_H_

#include "UIDialogWnd.h"

const u32			SKIN_TEX_HEIGHT			= 232;
const u32			SKIN_TEX_WIDTH			= 111;

class CUIStatic;
class CUIStatix;
class CUI3tButton;
class CUIAnimatedStatic;

typedef enum{
	SKIN_MENU_BACK = 0,
    SKIN_MENU_SPECTATOR,
	SKIN_MENU_AUTOSELECT
} ESKINMENU_BTN;

class CUISkinSelectorWnd: public CUIDialogWnd
{
	typedef CUIDialogWnd inherited;

public:	
	CUISkinSelectorWnd(const char* strSectionName, s16 team);
	~CUISkinSelectorWnd();
	
	virtual void	Init(const char* strSectionName);
	virtual void	SendMessage(CUIWindow *pWnd, s16 msg, void *pData = NULL);
	virtual bool	OnMouseAction(float x, float y, EUIMessages mouse_action);
	virtual bool	OnKeyboardAction(int dik, EUIMessages keyboard_action);
			void	SetVisibleForBtn(ESKINMENU_BTN btn, bool state);
			void	SetCurSkin(int skin);

	int				GetActiveIndex();
	s16				GetTeam()				{return m_team;};
	virtual void	Update			();
protected:
			void	OnBtnOK();
			void	OnBtnCancel();
			void	OnKeyLeft();
			void	OnKeyRight();

			void	InitSkins();
			void	UpdateSkins();
	CUIStatic*		m_pCaption;
	CUIStatic*		m_pBackground;
	CUIStatic*		m_pFrames;
#define p_image_count	6
	CUIStatix*		m_pImage[p_image_count];
//	CUI3tButton*	m_pButtons[2];
//	CUIAnimatedStatic* m_pAnims[2];
	CUI3tButton*	m_pBtnAutoSelect;
	CUI3tButton*	m_pBtnSpectator;
	CUI3tButton*	m_pBtnBack;
	
	shared_str		m_strSection;
	shared_str		m_shader;
	int				m_iActiveIndex; 
	xr_vector<xr_string> m_skins;
	xr_vector<int>	m_skinsEnabled;
	int				m_firstSkin;
	s16				m_team;
};

#endif