//-----------------------------------------------------------------------------/
//  Окно выбора скина в сетевой игре
//-----------------------------------------------------------------------------/

#pragma once

#include "UIDialogWnd.h"

class CUIStatic;
class CUIStatix;
class CUI3tButton;
class CUIAnimatedStatic;

typedef enum { SKIN_MENU_BACK = 0, SKIN_MENU_SPECTATOR, SKIN_MENU_AUTOSELECT } ESKINMENU_BTN;

class CUISkinSelectorWnd final : public CUIDialogWnd
{
    typedef CUIDialogWnd inherited;

public:
    CUISkinSelectorWnd(const char* strSectionName, s16 team);
    ~CUISkinSelectorWnd();

    virtual void Init(const char* strSectionName);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    void SetVisibleForBtn(ESKINMENU_BTN btn, bool state);
    void SetCurSkin(int skin);

    int GetActiveIndex();
    s16 GetTeam() { return m_team; };
    virtual void Update();

    pcstr GetDebugType() override { return "CUISkinSelectorWnd"; }

protected:
    void OnBtnOK();
    void OnBtnCancel();
    void OnKeyLeft();
    void OnKeyRight();

    void InitSkins();
    void UpdateSkins();

    xr_vector<CUIStatix*> m_pImage;
    CUI3tButton* m_pButtons[2];
    CUIAnimatedStatic* m_pAnims[2];
    CUI3tButton* m_pBtnAutoSelect;
    CUI3tButton* m_pBtnSpectator;
    CUI3tButton* m_pBtnBack;

    shared_str m_strSection;
    shared_str m_shader;
    int m_iActiveIndex;
    xr_vector<xr_string> m_skins;
    xr_vector<int> m_skinsEnabled;
    int m_firstSkin;
    s16 m_team;
};
