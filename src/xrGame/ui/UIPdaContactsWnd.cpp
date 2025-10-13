#include "stdafx.h"
#include "UIPdaContactsWnd.h"
#include "../Pda.h"
#include "UIXmlInit.h"
#include "../actor.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "UICharacterInfo.h"

#define PDA_CONTACT_HEIGHT 70
#define PDA_CONTACTS_XML "pda_contacts_new.xml"
#define PDA_CONTACT_CHAR "pda_character.xml"

extern CSE_ALifeTraderAbstract* ch_info_get_from_id(u16 id);

CUIPdaContactsWnd::CUIPdaContactsWnd() : CUIWindow("UIPdaContactsWnd")
{
    m_flags.zero();
}

CUIPdaContactsWnd::~CUIPdaContactsWnd()
{
}

void CUIPdaContactsWnd::Show(bool status)
{
    inherited::Show(status);
    if (status) UIDetailsWnd->Clear();

}

bool CUIPdaContactsWnd::Init()
{
    CUIXml uiXml;

    if (!uiXml.Load(CONFIG_PATH, UI_PATH, PDA_CONTACTS_XML, ShadowOfChernobylMode))
        return false;

    CUIXmlInit xml_init;
    xml_init.InitWindow(uiXml, "main_wnd", 0, this);

    UIFrameContacts = xr_new<CUIFrameWindow>("UIFrameContacts");
    UIFrameContacts->SetAutoDelete(true);
    AttachChild(UIFrameContacts);
    xml_init.InitFrameWindow(uiXml, "left_frame_window", 0, UIFrameContacts);

    UIContactsHeader = xr_new<CUIFrameLineWnd>("UIContactsHeader");
    UIContactsHeader->SetAutoDelete(true);
    UIFrameContacts->AttachChild(UIContactsHeader);
    xml_init.InitFrameLine(uiXml, "left_frame_line", 0, UIContactsHeader);

    UIRightFrame = xr_new<CUIFrameWindow>("contacts_right_frame_window");
    UIRightFrame->SetAutoDelete(true);
    AttachChild(UIRightFrame);
    xml_init.InitFrameWindow(uiXml, "right_frame_window", 0, UIRightFrame);

    UIRightFrameHeader = xr_new<CUIFrameLineWnd>("contacts_right_frame_line");
    UIRightFrameHeader->SetAutoDelete(true);
    UIRightFrame->AttachChild(UIRightFrameHeader);
    xml_init.InitFrameLine(uiXml, "right_frame_line", 0, UIRightFrameHeader);

    UIAnimation = xr_new<CUIAnimatedStatic>();
    UIAnimation->SetAutoDelete(true);
    UIContactsHeader->AttachChild(UIAnimation);
    xml_init.InitAnimatedStatic(uiXml, "a_static", 0, UIAnimation);

    UIListWnd = xr_new<CUIScrollView>();
    UIListWnd->SetAutoDelete(true);
    UIFrameContacts->AttachChild(UIListWnd);
    xml_init.InitScrollView(uiXml, "list", 0, UIListWnd);

    UIDetailsWnd = xr_new<CUIScrollView>();
    UIDetailsWnd->SetAutoDelete(true);
    UIRightFrame->AttachChild(UIDetailsWnd);
    xml_init.InitScrollView(uiXml, "detail_list", 0, UIDetailsWnd);

    xml_init.InitAutoStaticGroup(uiXml, "left_auto_static", 0, UIFrameContacts);
    xml_init.InitAutoStaticGroup(uiXml, "right_auto_static", 0, UIRightFrame);

    return true;
}


void CUIPdaContactsWnd::Update()
{
    if (TRUE == m_flags.test(flNeedUpdate)) {
        RemoveAll();

        CPda* pPda = Actor()->GetPDA();
        if (!pPda) return;

        pPda->ActivePDAContacts(m_pda_list);

        for (const auto &kv : m_pda_list)
            AddContact(kv);

        m_flags.set(flNeedUpdate, FALSE);
    }
    inherited::Update();
}

void CUIPdaContactsWnd::AddContact(CPda* pda)
{
    VERIFY(pda);

    CUICharacterInfo* pItem = NULL;
    pItem = xr_new<CUICharacterInfo>();
    UIListWnd->AddWindow(pItem, true);
    VERIFY(pda->GetOriginalOwner());
    pItem->InitCharacterInfo(Fvector2(), Fvector2(UIListWnd->GetWidth(), 85), PDA_CONTACT_CHAR);
    pItem->InitCharacter(pda->GetOriginalOwner()->object_id());
}

void CUIPdaContactsWnd::RemoveContact(CPda* pda)
{
    u32 cnt = UIListWnd->GetSize();

    for (u32 i = 0; i < cnt; ++i) {
        CUIWindow* w = UIListWnd->GetItem(i);
        CUIPdaContactItem* itm = (CUIPdaContactItem*)(w);

        if (itm->m_data == pda) {
            if (itm->GetSelected())
                UIDetailsWnd->Clear();
            UIListWnd->RemoveWindow(w);
            break;
        }
    }
}

//удалить все контакты из списка
void CUIPdaContactsWnd::RemoveAll()
{
    UIListWnd->Clear();
    UIDetailsWnd->Clear();
}

void CUIPdaContactsWnd::Reload()
{
    m_flags.set(flNeedUpdate, TRUE);
}

void CUIPdaContactsWnd::Reset()
{
    inherited::Reset();
    Reload();
}

CUIPdaContactItem::~CUIPdaContactItem()
{
}

void CUIPdaContactItem::SetSelected(bool b)
{
    CUISelectable::SetSelected(b);
    if (b) {
        m_cw->UIDetailsWnd->Clear();
        CCharacterInfo chInfo;
        CSE_ALifeTraderAbstract* T = ch_info_get_from_id(UIInfo->OwnerID());
        chInfo.Init(T);
    }
}

bool CUIPdaContactItem::OnMouseDown(int mouse_btn)
{
    bool ThisP = mouse_btn == MOUSE_1;
    if (ThisP) m_cw->UIListWnd->SetSelected(this);
    return ThisP;
}
