#include "pch_script.h"

/*
#include "UICarBodyWnd.h"
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../HUDManager.h"
#include "../level.h"
#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UIFrameWindow.h"
#include "UIItemInfo.h"
#include "UIPropertiesBox.h"
#include "../ai/monsters/BaseMonster/base_monster.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "../WeaponMagazined.h"
#include "../Actor.h"
#include "../eatable_item.h"
#include "../alife_registry_wrappers.h"
#include "UI3tButton.h"
#include "UIListBoxItem.h"
#include "../InventoryBox.h"
#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../BottleItem.h"

void move_item (u16 from_id, u16 to_id, u16 what_id);

CUICarBodyWnd::CUICarBodyWnd()
{
	m_pInventoryBox		= NULL;
	Init				();
	Hide				();
	m_b_need_update		= false;
}

CUICarBodyWnd::~CUICarBodyWnd()
{
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);
}

void CUICarBodyWnd::Init()
{
	CUIXml						uiXml;
	uiXml.Load					(CONFIG_PATH, UI_PATH, "carbody_new.xml");
	
	CUIXmlInit					xml_init;

	xml_init.InitWindow			(uiXml, "main", 0, this);

	m_pUIStaticTop				= xr_new<CUIStatic>(); m_pUIStaticTop->SetAutoDelete(true);
	AttachChild					(m_pUIStaticTop);
	xml_init.InitStatic			(uiXml, "top_background", 0, m_pUIStaticTop);


	m_pUIStaticBottom			= xr_new<CUIStatic>(); m_pUIStaticBottom->SetAutoDelete(true);
	AttachChild					(m_pUIStaticBottom);
	xml_init.InitStatic			(uiXml, "bottom_background", 0, m_pUIStaticBottom);

	m_pUIOurIcon				= xr_new<CUIStatic>(); m_pUIOurIcon->SetAutoDelete(true);
	AttachChild					(m_pUIOurIcon);
	xml_init.InitStatic			(uiXml, "static_icon", 0, m_pUIOurIcon);

	m_pUIOthersIcon				= xr_new<CUIStatic>(); m_pUIOthersIcon->SetAutoDelete(true);
	AttachChild					(m_pUIOthersIcon);
	xml_init.InitStatic			(uiXml, "static_icon", 1, m_pUIOthersIcon);


	m_pUICharacterInfoLeft		= xr_new<CUICharacterInfo>(); m_pUICharacterInfoLeft->SetAutoDelete(true);
	m_pUIOurIcon->AttachChild	(m_pUICharacterInfoLeft);
	m_pUICharacterInfoLeft->InitCharacterInfo(Fvector2().set(0,0), m_pUIOurIcon->GetWndSize(), "trade_character.xml");


	m_pUICharacterInfoRight			= xr_new<CUICharacterInfo>(); m_pUICharacterInfoRight->SetAutoDelete(true);
	m_pUIOthersIcon->AttachChild	(m_pUICharacterInfoRight);
	m_pUICharacterInfoRight->InitCharacterInfo(Fvector2().set(0,0), m_pUIOthersIcon->GetWndSize(), "trade_character.xml");

	m_pUIOurBagWnd					= xr_new<CUIStatic>(); m_pUIOurBagWnd->SetAutoDelete(true);
	AttachChild						(m_pUIOurBagWnd);
	xml_init.InitStatic				(uiXml, "our_bag_static", 0, m_pUIOurBagWnd);


	m_pUIOthersBagWnd				= xr_new<CUIStatic>(); m_pUIOthersBagWnd->SetAutoDelete(true);
	AttachChild						(m_pUIOthersBagWnd);
	xml_init.InitStatic				(uiXml, "others_bag_static", 0, m_pUIOthersBagWnd);

	m_pUIOurBagList					= xr_new<CUIDragDropListEx>(); m_pUIOurBagList->SetAutoDelete(true);
	m_pUIOurBagWnd->AttachChild		(m_pUIOurBagList);	
	xml_init.InitDragDropListEx		(uiXml, "dragdrop_list_our", 0, m_pUIOurBagList);

	m_pUIOthersBagList				= xr_new<CUIDragDropListEx>(); m_pUIOthersBagList->SetAutoDelete(true);
	m_pUIOthersBagWnd->AttachChild	(m_pUIOthersBagList);	
	xml_init.InitDragDropListEx		(uiXml, "dragdrop_list_other", 0, m_pUIOthersBagList);


	//информация о предмете
	m_pUIDescWnd					= xr_new<CUIFrameWindow>(); m_pUIDescWnd->SetAutoDelete(true);
	AttachChild						(m_pUIDescWnd);
	xml_init.InitFrameWindow		(uiXml, "frame_window", 0, m_pUIDescWnd);

	m_pUIStaticDesc					= xr_new<CUIStatic>(); m_pUIStaticDesc->SetAutoDelete(true);
	m_pUIDescWnd->AttachChild		(m_pUIStaticDesc);
	xml_init.InitStatic				(uiXml, "descr_static", 0, m_pUIStaticDesc);

	m_pUIItemInfo					= xr_new<CUIItemInfo>(); m_pUIItemInfo->SetAutoDelete(true);
	m_pUIDescWnd->AttachChild		(m_pUIItemInfo);
	m_pUIItemInfo->InitItemInfo		(Fvector2().set(0,0), m_pUIDescWnd->GetWndSize(), "carbody_item.xml");


	xml_init.InitAutoStatic			(uiXml, "auto_static", this);

	m_pUIPropertiesBox				= xr_new<CUIPropertiesBox>(); m_pUIPropertiesBox->SetAutoDelete(true);
	AttachChild						(m_pUIPropertiesBox);
	m_pUIPropertiesBox->InitPropertiesBox(Fvector2().set(0,0), Fvector2().set(300,300) );
	m_pUIPropertiesBox->Hide		();

	SetCurrentItem					(NULL);
	m_pUIStaticDesc->SetText		(NULL);

	m_pUITakeAll					= xr_new<CUI3tButton>(); m_pUITakeAll->SetAutoDelete(true);
	AttachChild						(m_pUITakeAll);
	xml_init.Init3tButton				(uiXml, "take_all_btn", 0, m_pUITakeAll);

	BindDragDropListEnents			(m_pUIOurBagList);
	BindDragDropListEnents			(m_pUIOthersBagList);


}

void CUICarBodyWnd::InitCarBody(CInventoryOwner* pOur, CInventoryBox* pInvBox)
{
    m_pOurObject									= pOur;
	m_pOthersObject									= NULL;
	m_pInventoryBox									= pInvBox;
	m_pInventoryBox->m_in_use						= true;

	u16 our_id										= smart_cast<CGameObject*>(m_pOurObject)->ID();
	m_pUICharacterInfoLeft->InitCharacter			(our_id);
	m_pUIOthersIcon->Show							(false);
	m_pUICharacterInfoRight->ClearInfo				();
	m_pUIPropertiesBox->Hide						();
	EnableAll										();
	UpdateLists										();

}

void CUICarBodyWnd::InitCarBody(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{

    m_pOurObject									= pOur;
	m_pOthersObject									= pOthers;
	m_pInventoryBox									= NULL;
	
	u16 our_id										= smart_cast<CGameObject*>(m_pOurObject)->ID();
	u16 other_id									= smart_cast<CGameObject*>(m_pOthersObject)->ID();

	m_pUICharacterInfoLeft->InitCharacter			(our_id);
	m_pUIOthersIcon->Show							(true);
	
	CBaseMonster *monster = NULL;
	if(m_pOthersObject) {
		monster										= smart_cast<CBaseMonster *>(m_pOthersObject);
		if (monster || m_pOthersObject->use_simplified_visual() ) 
		{
			m_pUICharacterInfoRight->ClearInfo		();
			if(monster)
			{
				shared_str monster_tex_name = pSettings->r_string(monster->cNameSect(),"icon");
				m_pUICharacterInfoRight->UIIcon().InitTexture(monster_tex_name.c_str());
				m_pUICharacterInfoRight->UIIcon().SetStretchTexture(true);
			}
		}else 
		{
			m_pUICharacterInfoRight->InitCharacter	(other_id);
		}
	}

	m_pUIPropertiesBox->Hide						();
	EnableAll										();
	UpdateLists										();

	if(!monster){
		CInfoPortionWrapper	*known_info_registry	= xr_new<CInfoPortionWrapper>();
		known_info_registry->registry().init		(other_id);
		KNOWN_INFO_VECTOR& known_info				= known_info_registry->registry().objects();

		KNOWN_INFO_VECTOR_IT it = known_info.begin();
		for(int i=0;it!=known_info.end();++it,++i){
			(*it).info_id;	
			NET_Packet		P;
			CGameObject::u_EventGen		(P,GE_INFO_TRANSFER, our_id);
			P.w_u16						(0);//not used
			P.w_stringZ					((*it).info_id);			//сообщение
			P.w_u8						(1);						//добавление сообщения
			CGameObject::u_EventSend	(P);
		}
		known_info.clear	();
		xr_delete			(known_info_registry);
	}
}  

void CUICarBodyWnd::UpdateLists_delayed()
{
		m_b_need_update = true;
}

#include "UIInventoryUtilities.h"

void CUICarBodyWnd::Hide()
{
	InventoryUtilities::SendInfoToActor			("ui_car_body_hide");
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);
	inherited::Hide								();
	if(m_pInventoryBox)
		m_pInventoryBox->m_in_use				= false;
}

void CUICarBodyWnd::UpdateLists()
{
	TIItemContainer								ruck_list;
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);

	ruck_list.clear								();
	m_pOurObject->inventory().AddAvailableItems	(ruck_list, true);
	std::sort									(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	//Наш рюкзак
	TIItemContainer::iterator it;
	for(it =  ruck_list.begin(); ruck_list.end() != it; ++it) 
	{
		CUICellItem* itm				= create_cell_item(*it);
		m_pUIOurBagList->SetItem		(itm);
	}


	ruck_list.clear									();
	if(m_pOthersObject)
		m_pOthersObject->inventory().AddAvailableItems	(ruck_list, false);
	else
		m_pInventoryBox->AddAvailableItems			(ruck_list);

	std::sort										(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	//Чужой рюкзак
	for(it =  ruck_list.begin(); ruck_list.end() != it; ++it) 
	{
		CUICellItem* itm							= create_cell_item(*it);
		m_pUIOthersBagList->SetItem					(itm);
	}

//	InventoryUtilities::UpdateWeight				(*m_pUIOurBagWnd);
	m_b_need_update									= false;
}

void CUICarBodyWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if (BUTTON_CLICKED == msg && m_pUITakeAll == pWnd)
	{
		TakeAll					();
	}
	else if(pWnd == m_pUIPropertiesBox &&	msg == PROPERTY_CLICKED)
	{
		if(m_pUIPropertiesBox->GetClickedItem())
		{
			switch(m_pUIPropertiesBox->GetClickedItem()->GetTAG())
			{
			case INVENTORY_EAT_ACTION:	//съесть объект
				EatItem();
				break;
			case INVENTORY_UNLOAD_MAGAZINE:
				{
				CUICellItem * itm = CurrentItem();
				(smart_cast<CWeaponMagazined*>((CWeapon*)itm->m_pData))->UnloadMagazine();
				for(u32 i=0; i<itm->ChildsCount(); ++i)
				{
					CUICellItem * child_itm			= itm->Child(i);
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->UnloadMagazine();
				}
				}break;
			}
		}
	}

	inherited::SendMessage			(pWnd, msg, pData);
}

void CUICarBodyWnd::Draw()
{
	inherited::Draw	();
}


void CUICarBodyWnd::Update()
{
	if(	m_b_need_update||
		m_pOurObject->inventory().ModifyFrame()==Device.dwFrame || 
		(m_pOthersObject&&m_pOthersObject->inventory().ModifyFrame()==Device.dwFrame))

		UpdateLists		();

	
	if(m_pOthersObject && (smart_cast<CGameObject*>(m_pOurObject))->Position().distance_to((smart_cast<CGameObject*>(m_pOthersObject))->Position()) > 3.0f)
	{
		GetHolder()->StartStopMenu(this,true);
	}
	inherited::Update();
}


void CUICarBodyWnd::Show() 
{ 
	InventoryUtilities::SendInfoToActor		("ui_car_body");
	inherited::Show							();
	SetCurrentItem							(NULL);
//	InventoryUtilities::UpdateWeight		(*m_pUIOurBagWnd);
}

void CUICarBodyWnd::DisableAll()
{
	m_pUIOurBagWnd->Enable			(false);
	m_pUIOthersBagWnd->Enable		(false);
}

void CUICarBodyWnd::EnableAll()
{
	m_pUIOurBagWnd->Enable			(true);
	m_pUIOthersBagWnd->Enable		(true);
}

CUICellItem* CUICarBodyWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUICarBodyWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUICarBodyWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;
	m_pCurrentCellItem		= itm;
	m_pUIItemInfo->InitItem(CurrentIItem());
}

void CUICarBodyWnd::TakeAll()
{
	u32 cnt				= m_pUIOthersBagList->ItemsCount();
	u16 tmp_id = 0;
	if(m_pInventoryBox){
		tmp_id	= (smart_cast<CGameObject*>(m_pOurObject))->ID();
	}

	for(u32 i=0; i<cnt; ++i)
	{
		CUICellItem*	ci = m_pUIOthersBagList->GetItemIdx(i);
		for(u32 j=0; j<ci->ChildsCount(); ++j)
		{
			PIItem _itm		= (PIItem)(ci->Child(j)->m_pData);
			if(m_pOthersObject)
				TransferItem	(_itm, m_pOthersObject, m_pOurObject, false);
			else{
				move_item		(m_pInventoryBox->ID(), tmp_id, _itm->object().ID());
//.				Actor()->callback(GameObject::eInvBoxItemTake)( m_pInventoryBox->lua_game_object(), _itm->object().lua_game_object() );
			}
		
		}
		PIItem itm		= (PIItem)(ci->m_pData);
		if(m_pOthersObject)
			TransferItem	(itm, m_pOthersObject, m_pOurObject, false);
		else{
			move_item		(m_pInventoryBox->ID(), tmp_id, itm->object().ID());
//.			Actor()->callback(GameObject::eInvBoxItemTake)(m_pInventoryBox->lua_game_object(), itm->object().lua_game_object() );
		}

	}
}


#include "../xr_level_controller.h"

bool CUICarBodyWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if( inherited::OnKeyboard(dik,keyboard_action) )return true;

	if(keyboard_action==WINDOW_KEY_PRESSED && is_binded(kUSE, dik)) 
	{
			GetHolder()->StartStopMenu(this,true);
			return true;
	}
	return false;
}

#include "../Medkit.h"
#include "../Antirad.h"

void CUICarBodyWnd::ActivatePropertiesBox()
{
	if(m_pInventoryBox)	return;
		
	m_pUIPropertiesBox->RemoveAll();
	
//.	CWeaponMagazined*		pWeapon			= smart_cast<CWeaponMagazined*>(CurrentIItem());
	CEatableItem*			pEatableItem	= smart_cast<CEatableItem*>(CurrentIItem());
	CMedkit*				pMedkit			= smart_cast<CMedkit*>			(CurrentIItem());
	CAntirad*				pAntirad		= smart_cast<CAntirad*>			(CurrentIItem());
	CBottleItem*			pBottleItem		= smart_cast<CBottleItem*>		(CurrentIItem());
    bool					b_show			= false;
	
	LPCSTR _action				= NULL;
	if(pMedkit || pAntirad)
	{
		_action						= "st_use";
		b_show						= true;
	}
	else if(pEatableItem)
	{
		if(pBottleItem)
			_action					= "st_drink";
		else
			_action					= "st_eat";
		b_show						= true;
	}
	if(_action)
		m_pUIPropertiesBox->AddItem(_action,  NULL, INVENTORY_EAT_ACTION);


	if(b_show){
		m_pUIPropertiesBox->AutoUpdateSize	();
		m_pUIPropertiesBox->BringAllToTop	();

		Fvector2						cursor_pos;
		Frect							vis_rect;

		GetAbsoluteRect					(vis_rect);
		cursor_pos						= GetUICursor()->GetCursorPosition();
		cursor_pos.sub					(vis_rect.lt);
		m_pUIPropertiesBox->Show		(vis_rect, cursor_pos);
	}
}

void CUICarBodyWnd::EatItem()
{
	CActor *pActor				= smart_cast<CActor*>(Level().CurrentEntity());
	if(!pActor)					return;

	NET_Packet					P;
	CGameObject::u_EventGen		(P, GEG_PLAYER_ITEM_EAT, Actor()->ID());
	P.w_u16						(CurrentIItem()->object().ID());
	CGameObject::u_EventSend	(P);

}


bool CUICarBodyWnd::OnItemDrop(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= CUIDragDropListEx::m_drag_item->BackList();
	
	if(old_owner==new_owner || !old_owner || !new_owner || (false&&new_owner==m_pUIOthersBagList&&m_pInventoryBox))
					return true;

	if(m_pOthersObject)
	{
		if( TransferItem		(	CurrentIItem(),
								(old_owner==m_pUIOthersBagList)?m_pOthersObject:m_pOurObject, 
								(old_owner==m_pUIOurBagList)?m_pOthersObject:m_pOurObject, 
								(old_owner==m_pUIOurBagList)
							)
			)
		{
			CUICellItem* ci					= old_owner->RemoveItem(CurrentItem(), false);
			new_owner->SetItem				(ci);
		}
	}else
	{
		u16 tmp_id	= (smart_cast<CGameObject*>(m_pOurObject))->ID();

		bool bMoveDirection		= (old_owner==m_pUIOthersBagList);

		move_item				(
								bMoveDirection?m_pInventoryBox->ID():tmp_id,
								bMoveDirection?tmp_id:m_pInventoryBox->ID(),
								CurrentIItem()->object().ID());


//		Actor()->callback		(GameObject::eInvBoxItemTake)(m_pInventoryBox->lua_game_object(), CurrentIItem()->object().lua_game_object() );

		CUICellItem* ci			= old_owner->RemoveItem(CurrentItem(), false);
		new_owner->SetItem		(ci);
	}
	SetCurrentItem					(NULL);

	return				true;
}

bool CUICarBodyWnd::OnItemStartDrag(CUICellItem* itm)
{
	return				false; //default behaviour
}

bool CUICarBodyWnd::OnItemDbClick(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= (old_owner==m_pUIOthersBagList)?m_pUIOurBagList:m_pUIOthersBagList;

	if(m_pOthersObject)
	{
		if( TransferItem		(	CurrentIItem(),
								(old_owner==m_pUIOthersBagList)?m_pOthersObject:m_pOurObject, 
								(old_owner==m_pUIOurBagList)?m_pOthersObject:m_pOurObject, 
								(old_owner==m_pUIOurBagList)
								)
			)
		{
			CUICellItem* ci			= old_owner->RemoveItem(CurrentItem(), false);
			new_owner->SetItem		(ci);
		}
	}else
	{
		if(false && old_owner==m_pUIOurBagList) return true;
		bool bMoveDirection		= (old_owner==m_pUIOthersBagList);

		u16 tmp_id				= (smart_cast<CGameObject*>(m_pOurObject))->ID();
		move_item				(
								bMoveDirection?m_pInventoryBox->ID():tmp_id,
								bMoveDirection?tmp_id:m_pInventoryBox->ID(),
								CurrentIItem()->object().ID());
//.		Actor()->callback		(GameObject::eInvBoxItemTake)(m_pInventoryBox->lua_game_object(), CurrentIItem()->object().lua_game_object() );

	}
	SetCurrentItem				(NULL);

	return						true;
}

bool CUICarBodyWnd::OnItemSelected(CUICellItem* itm)
{
	SetCurrentItem		(itm);
	return				false;
}

bool CUICarBodyWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox		();
	return						false;
}

void move_item (u16 from_id, u16 to_id, u16 what_id)
{
	NET_Packet P;
	CGameObject::u_EventGen					(	P,
												GE_OWNERSHIP_REJECT,
												from_id
											);

	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);

	//другому инвентарю - взять вещь 
	CGameObject::u_EventGen					(	P,
												GE_OWNERSHIP_TAKE,
												to_id
											);
	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);

}

bool CUICarBodyWnd::TransferItem(PIItem itm, CInventoryOwner* owner_from, CInventoryOwner* owner_to, bool b_check)
{
	VERIFY									(NULL==m_pInventoryBox);
	CGameObject* go_from					= smart_cast<CGameObject*>(owner_from);
	CGameObject* go_to						= smart_cast<CGameObject*>(owner_to);

	if(smart_cast<CBaseMonster*>(go_to))	return false;
	if(b_check)
	{
		float invWeight						= owner_to->inventory().CalcTotalWeight();
		float maxWeight						= owner_to->inventory().GetMaxWeight();
		float itmWeight						= itm->Weight();
		if(invWeight+itmWeight >=maxWeight)	return false;
	}

	move_item(go_from->ID(), go_to->ID(), itm->object().ID());

	return true;
}

void CUICarBodyWnd::BindDragDropListEnents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemRButtonClick);
}
*/