
#pragma once

#include "xrUICore/Windows/UIWindow.h"

class CUIFrameWindow;
class CUIFrameLineWnd;
class CUIStatic;
class CUIAnimatedStatic;
class CUIScrollView;
class CPda;
class CInventoryOwner;

class CUIPdaContactsWnd : public CUIWindow
{
private:
    typedef CUIWindow inherited;
    enum
    {
        flNeedUpdate = (1 << 0),
    };
    Flags8 m_flags;

public:
    CUIPdaContactsWnd();
    virtual ~CUIPdaContactsWnd();

    void Init();

    virtual void Update();
    virtual void Reset();

    virtual void Show(bool status);

    void AddContact(CPda* pda, u16 owner_id);
    void AddContact(CInventoryOwner* owner);
    void RemoveContact(CPda* pda);
    void RemoveAll();
    void Reload();

    pcstr GetDebugType() override { return "CUIPdaContactsWnd"; }

    CUIScrollView* UIListWnd;
    CUIScrollView* UIDetailsWnd;

protected:
    CUIFrameWindow* UIFrameContacts;
    CUIFrameLineWnd* UIContactsHeader;
    CUIFrameWindow* UIRightFrame;
    CUIFrameLineWnd* UIRightFrameHeader;
    CUIAnimatedStatic* UIAnimation;
};

#include "UIPdaListItem.h"
class CUIPdaContactItem : public CUIPdaListItem, public CUISelectable
{
    CUIPdaContactsWnd* m_cw;

public:
    CUIPdaContactItem(CUIPdaContactsWnd* cw) : m_cw(cw) {}
    virtual ~CUIPdaContactItem();
    virtual void SetSelected(bool b);
    virtual bool OnMouseDown(int mouse_btn);
};
