#include "stdafx.h"
#include "UIPdaContactsWnd.h"
#include "../Pda.h"
#include "UIXmlInit.h"
#include "../actor.h"
#include "UIFrameWindow.h"
#include "UIFrameLineWnd.h"
#include "UIAnimatedStatic.h"
#include "UIScrollView.h"
#include "../actor.h"
#include "../string_table.h"

#define PDA_CONTACT_HEIGHT 70

CUIPdaContactsWnd::CUIPdaContactsWnd()
{
	m_flags.zero();
}

CUIPdaContactsWnd::~CUIPdaContactsWnd()
{
}

void CUIPdaContactsWnd::Show(bool status)
{
	inherited::Show(status);
	if(status) UIDetailsWnd->Clear();

}

void CUIPdaContactsWnd::Init()
{
	CUIXml								uiXml;
	uiXml.Load							(CONFIG_PATH, UI_PATH, "pda_contacts_new.xml");

	CUIXmlInit	xml_init;

	xml_init.InitWindow					(uiXml, "main_wnd", 0, this);

	UIFrameContacts						= xr_new<CUIFrameWindow>();UIFrameContacts->SetAutoDelete(true);
	AttachChild							(UIFrameContacts);
	xml_init.InitFrameWindow			(uiXml, "left_frame_window", 0, UIFrameContacts);


	UIContactsHeader					= xr_new<CUIFrameLineWnd>();UIContactsHeader->SetAutoDelete(true);
	UIFrameContacts->AttachChild		(UIContactsHeader);
	xml_init.InitFrameLine				(uiXml, "left_frame_line", 0, UIContactsHeader);

	UIRightFrame						= xr_new<CUIFrameWindow>();UIRightFrame->SetAutoDelete(true);
	AttachChild							(UIRightFrame);
	xml_init.InitFrameWindow			(uiXml, "right_frame_window", 0, UIRightFrame);

	UIRightFrameHeader					= xr_new<CUIFrameLineWnd>();UIRightFrameHeader->SetAutoDelete(true);
	UIRightFrame->AttachChild			(UIRightFrameHeader);
	xml_init.InitFrameLine				(uiXml, "right_frame_line", 0, UIRightFrameHeader);

	UIAnimation							= xr_new<CUIAnimatedStatic>();UIAnimation->SetAutoDelete(true);
	UIContactsHeader->AttachChild		(UIAnimation);
	xml_init.InitAnimatedStatic			(uiXml, "a_static", 0, UIAnimation);

	UIListWnd							= xr_new<CUIScrollView>();UIListWnd->SetAutoDelete(true);
	UIFrameContacts->AttachChild		(UIListWnd);
	xml_init.InitScrollView				(uiXml, "list", 0, UIListWnd);

	UIDetailsWnd						= xr_new<CUIScrollView>();UIDetailsWnd->SetAutoDelete(true);
	UIRightFrame->AttachChild			(UIDetailsWnd);
	xml_init.InitScrollView				(uiXml, "detail_list", 0, UIDetailsWnd);
	

	xml_init.InitAutoStatic				(uiXml, "left_auto_static", UIFrameContacts);
	xml_init.InitAutoStatic				(uiXml, "right_auto_static", UIRightFrame);
}


void CUIPdaContactsWnd::Update()
{
	if(TRUE==m_flags.test(flNeedUpdate)){
		RemoveAll			();

		CPda*	pPda		= Actor()->GetPDA	();
		if(!pPda)			return;

		pPda->ActivePDAContacts	(m_pda_list);

		xr_vector<CPda*>::iterator it = m_pda_list.begin();

		for(; it!=m_pda_list.end();++it){
			AddContact(*it);
		}
		m_flags.set(flNeedUpdate, FALSE);
	}
	inherited::Update();
}

void CUIPdaContactsWnd::AddContact(CPda* pda)
{
	VERIFY(pda);


	CUIPdaContactItem* pItem		= NULL;
	pItem							= xr_new<CUIPdaContactItem>(this);
	UIListWnd->AddWindow			(pItem, true);
	pItem->InitPdaListItem			(Fvector2().set(0,0), Fvector2().set(UIListWnd->GetWidth(),85.0f));
	pItem->InitCharacter			(pda->GetOriginalOwner());
	pItem->m_data					= (void*)pda;
}

void CUIPdaContactsWnd::RemoveContact(CPda* pda)
{
	u32 cnt = UIListWnd->GetSize();

	for(u32 i=0 ; i<cnt; ++i ){
		CUIWindow* w = UIListWnd->GetItem(i);
		CUIPdaContactItem* itm = (CUIPdaContactItem*)(w);

		if(itm->m_data==(void*)pda){
			if(itm->GetSelected())
				UIDetailsWnd->Clear();
			UIListWnd->RemoveWindow(w);
			return;
		}
	}
}

//удалить все контакты из списка
void CUIPdaContactsWnd::RemoveAll()
{
	UIListWnd->Clear		();
	UIDetailsWnd->Clear		();
}

void CUIPdaContactsWnd::Reload()
{
	m_flags.set(flNeedUpdate,TRUE);
}

void CUIPdaContactsWnd::Reset()
{
	inherited::Reset			();
	Reload						();
}

CUIPdaContactItem::~CUIPdaContactItem()
{
}

extern CSE_ALifeTraderAbstract* ch_info_get_from_id (u16 id);

#include "UICharacterInfo.h"

void CUIPdaContactItem::SetSelected	(bool b)
{
	CUISelectable::SetSelected(b);
	if(b){
		m_cw->UIDetailsWnd->Clear		();
		CCharacterInfo				chInfo;
		CSE_ALifeTraderAbstract*	T = ch_info_get_from_id(UIInfo->OwnerID());
		chInfo.Init					(T);

//.		ADD_TEXT_TO_VIEW2( *(chInfo.Bio()), m_cw->UIDetailsWnd);
	}
}

bool CUIPdaContactItem::OnMouseDown(int mouse_btn)
{
	if(mouse_btn==MOUSE_1){
		m_cw->UIListWnd->SetSelected(this);
		return true;
	}
	return false;
}
