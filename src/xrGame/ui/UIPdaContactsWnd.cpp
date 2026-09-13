#include "StdAfx.h"
#include "UIPdaContactsWnd.h"
#include "UIPdaAux.h"
#include "../PDA.h"
#include "UIXmlInit.h"
#include "../Actor.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "../Actor.h"
#include "xrEngine/StringTable/StringTable.h"

#define PDA_CONTACT_HEIGHT 70

#define PDA_CONTACTS_XML "pda_contacts_new.xml"

CUIPdaContactsWnd::CUIPdaContactsWnd() : CUIWindow("CUIPdaContactsWnd") { m_flags.zero(); }

CUIPdaContactsWnd::~CUIPdaContactsWnd() {}

void CUIPdaContactsWnd::Show(bool status)
{
    inherited::Show(status);
    if (status)
    {
        UIDetailsWnd->Clear();
        Reload();
    }
}

void CUIPdaContactsWnd::Init()
{
    CUIXml uiXml;
    bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, PDA_CONTACTS_XML);
    R_ASSERT3(xml_result, "xml file not found", PDA_CONTACTS_XML);

    CUIXmlInit xml_init;

    xml_init.InitWindow(uiXml, "main_wnd", 0, this);

    UIFrameContacts = xr_new<CUIFrameWindow>();
    UIFrameContacts->SetAutoDelete(true);
    AttachChild(UIFrameContacts);
    xml_init.InitFrameWindow(uiXml, "left_frame_window", 0, UIFrameContacts);

    UIContactsHeader = xr_new<CUIFrameLineWnd>();
    UIContactsHeader->SetAutoDelete(true);
    UIFrameContacts->AttachChild(UIContactsHeader);
    xml_init.InitFrameLine(uiXml, "left_frame_line", 0, UIContactsHeader);

    UIRightFrame = xr_new<CUIFrameWindow>();
    UIRightFrame->SetAutoDelete(true);
    AttachChild(UIRightFrame);
    xml_init.InitFrameWindow(uiXml, "right_frame_window", 0, UIRightFrame);

    UIRightFrameHeader = xr_new<CUIFrameLineWnd>();
    UIRightFrameHeader->SetAutoDelete(true);
    UIRightFrame->AttachChild(UIRightFrameHeader);
    xml_init.InitFrameLine(uiXml, "right_frame_line", 0, UIRightFrameHeader);

    if (uiXml.NavigateToNode("a_static"))
    {
        UIAnimation = xr_new<CUIAnimatedStatic>();
        UIAnimation->SetAutoDelete(true);
        UIContactsHeader->AttachChild(UIAnimation);
        xml_init.InitAnimatedStatic(uiXml, "a_static", 0, UIAnimation);
    }

    UIListWnd = xr_new<CUIScrollView>();
    UIListWnd->SetAutoDelete(true);
    UIFrameContacts->AttachChild(UIListWnd);
    xml_init.InitScrollView(uiXml, "list", 0, UIListWnd);

    UIDetailsWnd = xr_new<CUIScrollView>();
    UIDetailsWnd->SetAutoDelete(true);
    UIRightFrame->AttachChild(UIDetailsWnd);
    xml_init.InitScrollView(uiXml, "detail_list", 0, UIDetailsWnd);

    CUIXmlInit::InitAutoStaticGroup(uiXml, "left_auto_static", 0, UIFrameContacts);
    CUIXmlInit::InitAutoStaticGroup(uiXml, "right_auto_static", 0, UIRightFrame);
}

void CUIPdaContactsWnd::Update()
{
    if (TRUE == m_flags.test(flNeedUpdate))
    {
        RemoveAll();

        CPda* pPda = Actor()->GetPDA();
        if (!pPda)
            return;

        if (ShadowOfChernobylMode)
        {
            xr_vector<CInventoryOwner*> owners;
            pPda->ActivePDAContactOwners(owners);
            for (CInventoryOwner* owner : owners)
                AddContact(owner);
        }
        else
        {
            xr_vector<CPda*> pda_list;
            pPda->ActivePDAContacts(pda_list);
            for (CPda* pda : pda_list)
                AddContact(pda, pda->GetOriginalOwnerID());
        }

        m_flags.set(flNeedUpdate, FALSE);
    }
    inherited::Update();
}

void CUIPdaContactsWnd::AddContact(CPda* pda, u16 owner_id)
{
    VERIFY(pda);

    auto pItem = xr_new<CUIPdaContactItem>(this);
    UIListWnd->AddWindow(pItem, true);
    pItem->Init(0, 0, UIListWnd->GetWidth(), 85);
    pItem->InitCharacter(pda->GetOriginalOwner());
    pItem->m_data = (void*)pda;
}

void CUIPdaContactsWnd::AddContact(CInventoryOwner* owner)
{
    VERIFY(owner);

    auto pItem = xr_new<CUIPdaContactItem>(this);
    UIListWnd->AddWindow(pItem, true);
    pItem->Init(0, 0, UIListWnd->GetWidth(), 85);
    pItem->InitCharacter(owner);
    pItem->m_data = (void*)owner;
}

void CUIPdaContactsWnd::RemoveContact(CPda* pda)
{
    u32 cnt = UIListWnd->GetSize();

    for (u32 i = 0; i < cnt; ++i)
    {
        CUIWindow* w = UIListWnd->GetItem(i);
        CUIPdaContactItem* itm = (CUIPdaContactItem*)(w);

        if (itm->m_data == (void*)pda)
        {
            if (itm->GetSelected())
                UIDetailsWnd->Clear();
            UIListWnd->RemoveWindow(w);
            return;
        }
    }
}

//удалить все контакты из списка
void CUIPdaContactsWnd::RemoveAll()
{
    UIListWnd->Clear();
    UIDetailsWnd->Clear();
}

void CUIPdaContactsWnd::Reload() { m_flags.set(flNeedUpdate, TRUE); }

void CUIPdaContactsWnd::Reset()
{
    inherited::Reset();
    Reload();
}

CUIPdaContactItem::~CUIPdaContactItem() {}

extern CSE_ALifeTraderAbstract* ch_info_get_from_id(u16 id);

#include "UICharacterInfo.h"
#include "../game_object_space.h"

void CUIPdaContactItem::SetSelected(bool b)
{
    CUISelectable::SetSelected(b);
    if (b)
    {
        m_cw->UIDetailsWnd->Clear();

        CCharacterInfo chInfo;
        CSE_ALifeTraderAbstract* T = ch_info_get_from_id(UIInfo->OwnerID());
        chInfo.Init(T);

        CUIStatic* pSt = xr_new<CUIStatic>();

        pSt->SetText(CStringTable().translate(chInfo.Bio().c_str()).c_str());
        pSt->SetTextComplexMode(true);
        pSt->SetWidth(m_cw->UIDetailsWnd->GetDesiredChildWidth());
        pSt->AdjustHeightToText();

        if (m_cw->UIDetailsWnd->GetFont())
            pSt->SetFont(m_cw->UIDetailsWnd->GetFont());

        m_cw->UIDetailsWnd->AddWindow(pSt, true);

    }
}

bool CUIPdaContactItem::OnMouseDown(int mouse_btn)
{
    if (mouse_btn == MOUSE_1)
    {
        m_cw->UIListWnd->SetSelected(this);
        return true;
    }
    return false;
}
