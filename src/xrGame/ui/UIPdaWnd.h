#pragma once
#include "UIDialogWnd.h"
#include "encyclopedia_article_defs.h"

class CInventoryOwner;
class CUIFrameLineWnd;
class CUI3tButton;
class CUITabControl;
class CUIStatic;
class CUIXml;
class CUIFrameWindow;
class UIHint;

class CUIMapWnd;
class CUITaskWnd;
class CUIFactionWarWnd;
class CUIActorInfoWnd;
class CUIRankingWnd;
class CUILogsWnd;
class CUIAnimatedStatic;
class UIHint;

class CUIPdaWnd final : public CUIDialogWnd
{
    typedef CUIDialogWnd inherited;

protected:
    CUITabControl* UITabControl;
    CUI3tButton* m_btn_close;

    CUIStatic* UIMainPdaFrame;
    CUIStatic* UINoice;

    CUIStatic* m_caption;
    shared_str m_caption_const;
    CUIStatic* m_clock;

    // Текущий активный диалог
    CUIWindow* m_pActiveDialog;
    shared_str m_sActiveSection;

    UIHint* m_hint_wnd;

public:
    // Поддиалоги PDA
    CUIMapWnd* pUIMapWnd;
    CUITaskWnd* pUITaskWnd;
    CUIFactionWarWnd* pUIFactionWarWnd;
    CUIActorInfoWnd* pUIActorInfo;
    CUIRankingWnd* pUIRankingWnd;
    CUILogsWnd* pUILogsWnd;

    virtual void Reset();

public:
    CUIPdaWnd();
    virtual ~CUIPdaWnd();

    virtual void Init();

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    virtual void Draw();
    virtual void Update();
    virtual void Show(bool status);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action)
    {
        CUIDialogWnd::OnMouseAction(x, y, mouse_action);
        return true;
    } // always true because StopAnyMove() == false
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);

    UIHint* get_hint_wnd() const { return m_hint_wnd; }
    void DrawHint();

    void SetActiveCaption();
    void SetCaption(pcstr text);
    void Show_SecondTaskWnd(bool status);
    void Show_MapLegendWnd(bool status);

    void SetActiveDialog(CUIWindow* wnd) { m_pActiveDialog = wnd; }
    CUIWindow* GetActiveDialog() const { return m_pActiveDialog; }
    pcstr GetActiveSection() const { return m_sActiveSection.c_str(); }
    void SetActiveSubdialog(const shared_str& section);
    CUITabControl* GetTabControl() const { return UITabControl; }

    virtual bool StopAnyMove() { return false; }
    void UpdatePda();
    void UpdateRankingWnd();

    pcstr GetDebugType() override { return "CUIPdaWnd"; }
};
