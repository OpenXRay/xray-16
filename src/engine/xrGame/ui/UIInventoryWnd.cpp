#include "pch_script.h"
#include "UIInventoryWnd.h"
/*
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../string_table.h"

#include "../actor.h"
#include "../uigamesp.h"
#include "../hudmanager.h"

#include "../CustomOutfit.h"

#include "../weapon.h"

#include "../../xrServerEntities/script_process.h"

#include "../eatable_item.h"
#include "../inventory.h"

#include "UIInventoryUtilities.h"
using namespace InventoryUtilities;


#include "../InfoPortion.h"
#include "../level.h"
#include "game_base_space.h"
#include "../entitycondition.h"

#include "../game_cl_base.h"
#include "../ActorCondition.h"
#include "UIDragDropListEx.h"
#include "UIOutfitSlot.h"
#include "UI3tButton.h"

#define				INVENTORY_ITEM_XML		"inventory_item.xml"
#define				INVENTORY_XML			"inventory_new.xml"



CUIInventoryWnd*	g_pInvWnd = NULL;

CUIInventoryWnd::CUIInventoryWnd()
{
	m_iCurrentActiveSlot				= NO_ACTIVE_SLOT;
	UIRank								= NULL;
	Init								();
	SetCurrentItem						(NULL);

	g_pInvWnd							= this;	
	m_b_need_reinit						= false;
	Hide								();	
}

void CUIInventoryWnd::Init()
{
	CUIXml								uiXml;
	uiXml.Load							(CONFIG_PATH, UI_PATH, INVENTORY_XML);

	CUIXmlInit							xml_init;

	xml_init.InitWindow					(uiXml, "main", 0, this);

	AttachChild							(&UIBeltSlots);
	xml_init.InitStatic					(uiXml, "belt_slots", 0, &UIBeltSlots);

	AttachChild							(&UIBack);
	xml_init.InitStatic					(uiXml, "back", 0, &UIBack);

	AttachChild							(&UIStaticBottom);
	xml_init.InitStatic					(uiXml, "bottom_static", 0, &UIStaticBottom);

	AttachChild							(&UIBagWnd);
	xml_init.InitStatic					(uiXml, "bag_static", 0, &UIBagWnd);
	
	AttachChild							(&UIMoneyWnd);
	xml_init.InitStatic					(uiXml, "money_static", 0, &UIMoneyWnd);

	AttachChild							(&UIDescrWnd);
	xml_init.InitStatic					(uiXml, "descr_static", 0, &UIDescrWnd);


	UIDescrWnd.AttachChild				(&UIItemInfo);
	UIItemInfo.InitItemInfo				(Fvector2().set(0,0), UIDescrWnd.GetWndSize(), INVENTORY_ITEM_XML);

	AttachChild							(&UIPersonalWnd);
	xml_init.InitFrameWindow			(uiXml, "character_frame_window", 0, &UIPersonalWnd);

	AttachChild							(&UIProgressBack);
	xml_init.InitStatic					(uiXml, "progress_background", 0, &UIProgressBack);

	if (GameID() != eGameIDSingle){
		AttachChild						(&UIProgressBack_rank);
		xml_init.InitStatic				(uiXml, "progress_back_rank", 0, &UIProgressBack_rank);

		UIProgressBack_rank.AttachChild	(&UIProgressBarRank);
		xml_init.InitProgressBar		(uiXml, "progress_bar_rank", 0, &UIProgressBarRank);
		UIProgressBarRank.SetProgressPos(100);

	}
	

	UIProgressBack.AttachChild (&UIProgressBarHealth);
	xml_init.InitProgressBar (uiXml, "progress_bar_health", 0, &UIProgressBarHealth);
	
	UIProgressBack.AttachChild	(&UIProgressBarPsyHealth);
	xml_init.InitProgressBar (uiXml, "progress_bar_psy", 0, &UIProgressBarPsyHealth);

	UIProgressBack.AttachChild	(&UIProgressBarRadiation);
	xml_init.InitProgressBar (uiXml, "progress_bar_radiation", 0, &UIProgressBarRadiation);

	UIPersonalWnd.AttachChild			(&UIStaticPersonal);
	xml_init.InitStatic					(uiXml, "static_personal",0, &UIStaticPersonal);

	AttachChild							(&UIOutfitInfo);
	UIOutfitInfo.InitFromXml			(uiXml);

	//Ёлементы автоматического добавлени€
	xml_init.InitAutoStatic				(uiXml, "auto_static", this);


	if (!IsGameTypeSingle())
	{
		UIRankFrame = xr_new<CUIStatic> (); 
		UIRankFrame->SetAutoDelete		(true);
		UIRank = xr_new<CUIStatic>		(); 
		UIRank->SetAutoDelete			(true);

		CUIXmlInit::InitStatic			(uiXml, "rank", 0, UIRankFrame);
		CUIXmlInit::InitStatic			(uiXml, "rank:pic", 0, UIRank);
		AttachChild						(UIRankFrame);
		UIRankFrame->AttachChild		(UIRank);		
	}

	m_pUIBagList						= xr_new<CUIDragDropListEx>(); UIBagWnd.AttachChild(m_pUIBagList); m_pUIBagList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_bag", 0, m_pUIBagList);
	BindDragDropListEnents				(m_pUIBagList);

	m_pUIBeltList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIBeltList); m_pUIBeltList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_belt", 0, m_pUIBeltList);
	BindDragDropListEnents				(m_pUIBeltList);

	m_pUIOutfitList						= xr_new<CUIOutfitDragDropList>(); AttachChild(m_pUIOutfitList); m_pUIOutfitList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_outfit", 0, m_pUIOutfitList);
	BindDragDropListEnents				(m_pUIOutfitList);

	m_pUIPistolList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIPistolList); m_pUIPistolList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_pistol", 0, m_pUIPistolList);
	BindDragDropListEnents				(m_pUIPistolList);

	m_pUIAutomaticList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIAutomaticList); m_pUIAutomaticList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_automatic", 0, m_pUIAutomaticList);
	BindDragDropListEnents				(m_pUIAutomaticList);

	//pop-up menu
	AttachChild							(&UIPropertiesBox);
	UIPropertiesBox.InitPropertiesBox	(Fvector2().set(0,0),Fvector2().set(300,300));
	UIPropertiesBox.Hide				();

	AttachChild							(&UIStaticTime);
	xml_init.InitStatic					(uiXml, "time_static", 0, &UIStaticTime);

	UIStaticTime.AttachChild			(&UIStaticTimeString);
	xml_init.InitStatic					(uiXml, "time_static_str", 0, &UIStaticTimeString);

	UIExitButton						= xr_new<CUI3tButton>();UIExitButton->SetAutoDelete(true);
	AttachChild							(UIExitButton);
	xml_init.Init3tButton				(uiXml, "exit_button", 0, UIExitButton);

//Load sounds

	XML_NODE* stored_root				= uiXml.GetLocalRoot		();
	uiXml.SetLocalRoot					(uiXml.NavigateToNode		("action_sounds",0));
	::Sound->create						(sounds[eInvSndOpen],		uiXml.Read("snd_open",			0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvSndClose],		uiXml.Read("snd_close",			0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToSlot],	uiXml.Read("snd_item_to_slot",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToBelt],	uiXml.Read("snd_item_to_belt",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToRuck],	uiXml.Read("snd_item_to_ruck",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvProperties],	uiXml.Read("snd_properties",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvDropItem],		uiXml.Read("snd_drop_item",		0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvAttachAddon],	uiXml.Read("snd_attach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemUse],		uiXml.Read("snd_item_use",		0,	NULL),st_Effect,sg_SourceType);

	uiXml.SetLocalRoot					(stored_root);
}

EListType CUIInventoryWnd::GetType(CUIDragDropListEx* l)
{
	if(l==m_pUIBagList)			return iwActorBag;
	if(l==m_pUIBeltList)		return iwActorBelt;

	if(l==m_pUIAutomaticList)	return iwActorSlot;
	if(l==m_pUIPistolList)		return iwActorSlot;
	if(l==m_pUIOutfitList)		return iwActorSlot;

	NODEFAULT;
#ifdef DEBUG
	return iwActorSlot;
#endif // DEBUG
}

void CUIInventoryWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
        sounds[a].play					(NULL, sm_2D);
}

CUIInventoryWnd::~CUIInventoryWnd()
{
//.	ClearDragDrop(m_vDragDropItems);
	ClearAllLists						();
}

bool CUIInventoryWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if(m_b_need_reinit)
		return true;

	//вызов дополнительного меню по правой кнопке
	if(mouse_action == WINDOW_RBUTTON_DOWN)
	{
		if(UIPropertiesBox.IsShown())
		{
			UIPropertiesBox.Hide		();
			return						true;
		}
	}

	CUIWindow::OnMouse					(x, y, mouse_action);

	return true; // always returns true, because ::StopAnyMove() == true;
}

void CUIInventoryWnd::Draw()
{
	CUIWindow::Draw						();
}


void CUIInventoryWnd::Update()
{
	if(m_b_need_reinit)
		InitInventory					();


	CEntityAlive *pEntityAlive			= smart_cast<CEntityAlive*>(Level().CurrentEntity());

	if(pEntityAlive) 
	{
		float v = pEntityAlive->conditions().GetHealth()*100.0f;
		UIProgressBarHealth.SetProgressPos		(v);

		v = pEntityAlive->conditions().GetPsyHealth()*100.0f;
		UIProgressBarPsyHealth.SetProgressPos	(v);

		v = pEntityAlive->conditions().GetRadiation()*100.0f;
		UIProgressBarRadiation.SetProgressPos	(v);

		CInventoryOwner* pOurInvOwner	= smart_cast<CInventoryOwner*>(pEntityAlive);
		u32 _money						= 0;

		if (GameID() != eGameIDSingle){
			game_PlayerState* ps = Game().GetPlayerByGameID(pEntityAlive->ID());
			if (ps){
				UIProgressBarRank.SetProgressPos(ps->experience_D*100);
				_money							= ps->money_for_round;
			}
		}else
		{
			_money							= pOurInvOwner->get_money();
		}
		// update money
		string64						sMoney;
		sprintf_s							(sMoney,"%d RU", _money);
		UIMoneyWnd.SetText				(sMoney);

		// update outfit parameters
		CCustomOutfit* outfit			= smart_cast<CCustomOutfit*>(pOurInvOwner->inventory().m_slots[OUTFIT_SLOT].m_pIItem);		
		UIOutfitInfo.Update				(outfit);		
	}

	UIStaticTimeString.SetText(*InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes));

	CUIWindow::Update					();
}

void CUIInventoryWnd::Show() 
{ 
	InitInventory			();
	inherited::Show			();

	if (!IsGameTypeSingle())
	{
		CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
		if(!pActor) return;

		pActor->SetWeaponHideState(INV_STATE_INV_WND, true);

		//rank icon		
		int team = Game().local_player->team;
		int rank = Game().local_player->rank;
		string256 _path;	
		
		u32	gameid = GameID();
		if ((gameid == eGameIDTeamDeathmatch) || (gameid == eGameIDArtefactHunt))
		{
			if (1==team)
		        sprintf_s(_path, "ui_hud_status_green_0%d", rank+1);
			else
				sprintf_s(_path, "ui_hud_status_blue_0%d", rank+1);
		} else
		{
			if (team == 0)
				sprintf_s(_path, "ui_hud_status_green_0%d", rank+1);
			else
				sprintf_s(_path, "ui_hud_status_blue_0%d", rank+1);
		}

		UIRank->InitTexture(_path);
	}

	SendInfoToActor						("ui_inventory");

	Update								();
	PlaySnd								(eInvSndOpen);
}

void CUIInventoryWnd::Hide()
{
	PlaySnd								(eInvSndClose);
	inherited::Hide						();

	SendInfoToActor						("ui_inventory_hide");
	ClearAllLists						();

	//достать вещь в активный слот
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && m_iCurrentActiveSlot != NO_ACTIVE_SLOT && 
		pActor->inventory().m_slots[m_iCurrentActiveSlot].m_pIItem)
	{
		pActor->inventory().Activate(m_iCurrentActiveSlot);
		m_iCurrentActiveSlot = NO_ACTIVE_SLOT;
	}

	if (!IsGameTypeSingle())
	{
		CActor *pActor		= smart_cast<CActor*>(Level().CurrentEntity());
		if(!pActor)			return;

		pActor->SetWeaponHideState(INV_STATE_INV_WND, false);
	}
}

void CUIInventoryWnd::AttachAddon(PIItem item_to_upgrade)
{
	PlaySnd										(eInvAttachAddon);
	R_ASSERT									(item_to_upgrade);
	if (OnClient())
	{
		NET_Packet								P;
		item_to_upgrade->object().u_EventGen	(P, GE_ADDON_ATTACH, item_to_upgrade->object().ID());
		P.w_u16									(CurrentIItem()->object().ID());
		item_to_upgrade->object().u_EventSend	(P);
	};

	item_to_upgrade->Attach						(CurrentIItem(), true);


	//спр€тать вещь из активного слота в инвентарь на врем€ вызова менюшки
	CActor *pActor								= smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && item_to_upgrade == pActor->inventory().ActiveItem())
	{
			m_iCurrentActiveSlot				= pActor->inventory().GetActiveSlot();
			pActor->inventory().Activate		(NO_ACTIVE_SLOT);
	}
	SetCurrentItem								(NULL);
}

void CUIInventoryWnd::DetachAddon(const char* addon_name)
{
	PlaySnd										(eInvDetachAddon);
	if (OnClient())
	{
		NET_Packet								P;
		CurrentIItem()->object().u_EventGen		(P, GE_ADDON_DETACH, CurrentIItem()->object().ID());
		P.w_stringZ								(addon_name);
		CurrentIItem()->object().u_EventSend	(P);
	};
	CurrentIItem()->Detach						(addon_name, true);

	//спр€тать вещь из активного слота в инвентарь на врем€ вызова менюшки
	CActor *pActor								= smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && CurrentIItem() == pActor->inventory().ActiveItem())
	{
			m_iCurrentActiveSlot				= pActor->inventory().GetActiveSlot();
			pActor->inventory().Activate		(NO_ACTIVE_SLOT);
	}
}


void CUIInventoryWnd::SendEvent_ActivateSlot(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ACTIVATE_SLOT, pItem->object().H_Parent()->ID());
	P.w_u32							(pItem->GetSlot());
	pItem->object().u_EventSend		(P);
}

void CUIInventoryWnd::SendEvent_Item2Slot(PIItem pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2SLOT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
	g_pInvWnd->PlaySnd				(eInvItemToSlot);
};

void CUIInventoryWnd::SendEvent_Item2Belt(PIItem pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2BELT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
	g_pInvWnd->PlaySnd				(eInvItemToBelt);
};

void CUIInventoryWnd::SendEvent_Item2Ruck(PIItem pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2RUCK, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);

	g_pInvWnd->PlaySnd				(eInvItemToRuck);
};

void CUIInventoryWnd::SendEvent_Item_Drop(PIItem pItem)
{
	pItem->SetDropManual			(TRUE);
	g_pInvWnd->PlaySnd				(eInvDropItem);
};

void CUIInventoryWnd::SendEvent_Item_Eat(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM_EAT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
};


void CUIInventoryWnd::BindDragDropListEnents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemRButtonClick);
}


#include "../xr_level_controller.h"
#include <dinput.h>

bool CUIInventoryWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if(m_b_need_reinit)
		return true;

	if (UIPropertiesBox.GetVisible())
		UIPropertiesBox.OnKeyboard(dik, keyboard_action);

	if ( is_binded(kDROP, dik) )
	{
		if(WINDOW_KEY_PRESSED==keyboard_action)
			DropCurrentItem(false);
		return true;
	}

	if (WINDOW_KEY_PRESSED == keyboard_action)
	{
#ifdef DEBUG
		if(DIK_NUMPAD7 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(-0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
		else if(DIK_NUMPAD8 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
#endif
	}
	if( inherited::OnKeyboard(dik,keyboard_action) )return true;

	return false;
}
*/