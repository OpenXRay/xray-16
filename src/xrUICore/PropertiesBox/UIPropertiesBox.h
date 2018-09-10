#pragma once
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/ListBox/UIListBox.h"

class XRUICORE_API CUIPropertiesBox : public CUIFrameWindow, public CUIWndCallback
{
private:
    typedef CUIFrameWindow inherited;

public:
    CUIPropertiesBox(CUIPropertiesBox* sub_property_box = NULL);
    virtual ~CUIPropertiesBox();

    void InitPropertiesBox(Fvector2 pos, Fvector2 size);

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);

    bool AddItem(LPCSTR str, void* pData = NULL, u32 tag_value = 0);
    bool AddItem_script(LPCSTR str) { return AddItem(str); };
    u32 GetItemsCount() { return m_UIListWnd.GetSize(); };
    void RemoveItemByTAG(u32 tag_value);
    void RemoveAll();

    virtual void Show(const Frect& parent_rect, const Fvector2& point);
    virtual void Hide();

    virtual void Update();
    virtual void Draw();

    CUIListBoxItem* GetClickedItem();

    void AutoUpdateSize();

    void ShowSubMenu();
    void xr_stdcall OnItemReceivedFocus(CUIWindow* w, void* d);

protected:
    CUIListBox m_UIListWnd;

private:
    // I must not hide this menu, and my child sub menu must hide me...
    CUIPropertiesBox* m_sub_property_box;
    void SetParentSubMenu(CUIPropertiesBox* parent_menu) { m_parent_sub_menu = parent_menu; };
    Frect m_last_show_rect;
    CUIPropertiesBox* m_parent_sub_menu; // warning !!! dubling pointers to the same object !!!
    CUIWindow* m_item_sub_menu_initiator; // fills in ShowSubMenu
};
