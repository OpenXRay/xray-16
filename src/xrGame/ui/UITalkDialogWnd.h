#pragma once
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "InfoPortion.h"
#include "UICharacterInfo.h"
#include "UIItemInfo.h"
#include "xrUICore/Callbacks/UIWndCallback.h"

class CUIScrollView;
class CUIXml;
class CUITalkWnd;

class CUITalkDialogWnd final : public CUIWindow, public CUIWndCallback
{
    using inherited = CUIWindow;
    CUIXml* m_uiXml;

public:
    CUITalkDialogWnd();
    ~CUITalkDialogWnd() override;

    pcstr GetDebugType() override { return "CUITalkDialogWnd"; }

    void InitTalkDialogWnd();

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    virtual void Show();
    virtual void Hide();
    CUITalkWnd* m_pParent;
    u32 GetHeaderColor() { return m_iNameTextColor; }
    CGameFont* GetHeaderFont() { return m_pNameTextFont; }
    u32 GetOurReplicsColor() { return m_uOurReplicsColor; }
    bool mechanic_mode; // for inventory upgrades

    // Selected question number
    shared_str m_ClickedQuestionID;

    // Dialog interface elements
    CUIStatic* UIStaticTop;
    CUIStatic* UIStaticBottom;

    CUIFrameLineWnd* UIDialogFrame;
    CUIFrameLineWnd* UIOurPhrasesFrame;

    CUIStatic* UIDialogFrameTop;
    CUIStatic* UIDialogFrameBottom;

    Fvector2 m_btn_pos[3];
    CUI3tButton UIToTradeButton;
    CUI3tButton* UIToExitButton;

    // Characters info
    CUIStatic* UIOurIcon;
    CUIStatic* UIOthersIcon;
    CUICharacterInfo UICharacterInfoLeft;
    CUICharacterInfo UICharacterInfoRight;

    void AddQuestion(LPCSTR str, LPCSTR value, int number, bool b_finalizer);
    void AddAnswer(LPCSTR SpeakerName, const char* str, bool bActor);
    void AddIconedAnswer(LPCSTR caption, LPCSTR text, LPCSTR texture_name, LPCSTR templ_name);
    void AddIconedAnswer(pcstr text, pcstr texture_name, Frect texture_rect, pcstr templ_name);
    void ClearAll();
    void ClearQuestions();

    void SetOsoznanieMode(bool b);
    void SetTradeMode();
    void UpdateButtonsLayout(bool b_disable_break, bool trade_enabled);

    void TryScrollAnswersList(bool down);

private:
    // List of questions we can ask the character
    CUIScrollView* UIQuestionsList;
    CUIScrollView* UIAnswersList;

    // Font and colour of text with character name
    CGameFont* m_pNameTextFont;
    u32 m_iNameTextColor;

    // Text colour of our replics
    u32 m_uOurReplicsColor;

    void OnTradeClicked(CUIWindow* w, void*);
    void OnUpgradeClicked(CUIWindow* w, void*);
    void OnQuestionClicked(CUIWindow* w, void*);
    void OnExitClicked(CUIWindow* w, void*);
};

class CUIQuestionItem final : public CUIWindow, public CUIWndCallback
{
    typedef CUIWindow inherited;
    float m_min_height;

public:
    CUITextWnd* m_num_text;
    CUI3tButton* m_text;
    shared_str m_s_value;
    CUIQuestionItem(CUIXml* xml_doc, LPCSTR path);
    void Init(LPCSTR val, LPCSTR text);

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
    void OnTextClicked(CUIWindow* w, void*);

    pcstr GetDebugType() override { return "CUIQuestionItem"; }
};

class CUIAnswerItem: public CUIWindow
{
    typedef CUIWindow inherited;

    float m_min_height;
    float m_bottom_footer;
    CUITextWnd* m_text;
    CUITextWnd* m_name;

public:
    CUIAnswerItem(CUIXml* xml_doc, LPCSTR path);
    void Init(LPCSTR text, LPCSTR name);
    pcstr GetDebugType() override { return "CUIAnswerItem"; }
};

class CUIAnswerItemIconed final : public CUIAnswerItem
{
    typedef CUIAnswerItem inherited;
    CUIStatic* m_icon;

public:
    CUIAnswerItemIconed(CUIXml* xml_doc, LPCSTR path);
    void Init(LPCSTR text, LPCSTR name, LPCSTR texture_name);
    void Init(pcstr text, pcstr texture_name, Frect texture_rect);
    pcstr GetDebugType() override { return "CUIAnswerItemIconed"; }
};
