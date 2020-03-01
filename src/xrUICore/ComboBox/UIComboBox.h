#pragma once
#include "xrUICore/EditBox/UIEditBox.h"
#include "xrUICore/ListBox/UIListBox.h"
#include "xrUICore/InteractiveBackground/UIInteractiveBackground.h"
#include "xrUICore/Options/UIOptionsItem.h"
#include "xrUICore/Windows/UIFrameWindow.h"

class CUIListBoxItem;

class XRUICORE_API CUIComboBox : public CUIWindow, public CUIOptionsItem, public pureRender
{
    friend class CUIXmlInitBase;
    typedef enum { LIST_EXPANDED, LIST_FONDED } E_COMBO_STATE;

    xr_vector<int> m_disabled;

public:
    CUIComboBox();
    virtual ~CUIComboBox();
    // CUIOptionsItem
    virtual void SetCurrentOptValue(); // opt->current
    virtual void SaveBackUpOptValue(); // current->backup
    virtual void SaveOptValue(); // current->opt
    virtual void UndoOptValue(); // backup->current
    virtual bool IsChangedOptValue() const; // backup!=current

    virtual void OnRender(); // only for list-box

    LPCSTR GetText();
    LPCSTR GetTextOf(int index);
    void SetText(LPCSTR text);

    void SetListLength(int length);
    void SetVertScroll(bool bVScroll = true) { m_list_box.SetFixedScrollBar(bVScroll); };
    CUIListBoxItem* AddItem_(LPCSTR str, int _data);
    void InitComboBox(Fvector2 pos, float width);
    void SetItemIDX(int idx);
    void SetItemToken(int tok);

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = 0);
    virtual void OnFocusLost();
    virtual void OnFocusReceive();
    int CurrentID() { return m_itoken_id; }
    void disable_id(int id);
    void enable_id(int id);

protected:
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void OnBtnClicked();
    void ShowList(bool bShow);
    void OnListItemSelect();
    virtual void Update();
    virtual void Draw();

public:
    void ClearList();

    u32 GetSize();

protected:
    bool m_bInited;
    int m_iListHeight;
    int m_itoken_id;
    E_COMBO_STATE m_eState;
    int m_opt_backup_value;

    CUI_IB_FrameLineWnd m_frameLine;
    CUITextWnd m_text;
    CUIFrameWindow m_list_frame;

    u32 m_textColor[2];

public:
    CUIListBox m_list_box;
    void SetTextColor(u32 color) { m_textColor[0] = color; };
    void SetTextColorD(u32 color) { m_textColor[1] = color; };
};
