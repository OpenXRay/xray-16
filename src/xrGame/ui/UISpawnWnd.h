#pragma once

#include "UIDialogWnd.h"

class CUIStatix;
class CUIStatic;
class CUI3tButton;
class CUIScrollView;

// typedef	void (*ButtonClickCallback) (int);
typedef enum { TEAM_MENU_BACK = 0, TEAM_MENU_SPECTATOR, TEAM_MENU_AUTOSELECT } ETEAMMENU_BTN;

class CUISpawnWnd final : public CUIDialogWnd
{
    using inherited = CUIDialogWnd;

public:
    CUISpawnWnd();
    ~CUISpawnWnd() override;

    virtual void Init();
    void SendMessage(CUIWindow* pWnd, s16 msg, void* pData) override;
    bool OnKeyboardAction(int dik, EUIMessages keyboard_action) override;
    void SetVisibleForBtn(ETEAMMENU_BTN btn, bool state);
    void SetCurTeam(int team);

    pcstr GetDebugType() override { return "CUISpawnWnd"; }

protected:
    void InitTeamLogo();

    // Установка нового режима отображения
    //	void SetDisplayMode(bool bDual = false);

    // -1 - еще не нажималась, 0 - primary (левая), 1 - secondary (правая)
    //	int GetPressingResult() { return 1; }

    //	void	SetCallbackFunc (ButtonClickCallback pFunc);

protected:
    CUIStatic* m_pCaption;
    CUIStatic* m_pBackground;
    CUIStatic* m_pFrames[2];
    CUIScrollView* m_pTextDesc;
    CUIStatix* m_pImage1;
    CUIStatix* m_pImage2;
    CUI3tButton* m_pBtnAutoSelect;
    CUI3tButton* m_pBtnSpectator;
    CUI3tButton* m_pBtnBack;

    int m_iCurTeam;
};
