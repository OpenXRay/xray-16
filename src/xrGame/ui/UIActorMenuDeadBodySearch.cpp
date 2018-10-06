#include "StdAfx.h"
#include "UIActorMenu.h"
#include "UIDragDropListEx.h"
#include "UICharacterInfo.h"
#include "UIInventoryUtilities.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrMessages.h"
#include "alife_registry_wrappers.h"
#include "GameObject.h"
#include "InventoryOwner.h"
#include "Inventory.h"
#include "inventory_item.h"
#include "InventoryBox.h"
#include "string_table.h"
#include "ai/monsters/basemonster/base_monster.h"

void move_item_from_to(u16 from_id, u16 to_id, u16 what_id)
{
    NET_Packet P;
    CGameObject::u_EventGen(P, GE_TRADE_SELL, from_id);
    P.w_u16(what_id);
    CGameObject::u_EventSend(P);

    //другому инвентарю - взять вещь
    CGameObject::u_EventGen(P, GE_TRADE_BUY, to_id);
    P.w_u16(what_id);
    CGameObject::u_EventSend(P);
}

bool move_item_check(PIItem itm, CInventoryOwner* from, CInventoryOwner* to, bool weight_check)
{
    if (weight_check)
    {
        float invWeight = to->inventory().CalcTotalWeight();
        float maxWeight = to->MaxCarryWeight();
        float itmWeight = itm->Weight();
        if (invWeight + itmWeight >= maxWeight)
        {
            return false;
        }
    }
    move_item_from_to(from->object_id(), to->object_id(), itm->object_id());
    return true;
}

// -------------------------------------------------------------------------------------------------

void CUIActorMenu::InitDeadBodySearchMode()
{
    m_pDeadBodyBagList->Show(true);
    m_LeftBackground->Show(true);
    m_PartnerBottomInfo->Show(true);
    m_PartnerWeight->Show(true);
    m_takeall_button->Show(true);

    if (m_pPartnerInvOwner)
    {
        m_PartnerCharacterInfo->Show(true);
    }
    else
    {
        m_PartnerCharacterInfo->Show(false);
    }

    InitInventoryContents(m_pInventoryBagList);

    TIItemContainer items_list;
    if (m_pPartnerInvOwner)
    {
        m_pPartnerInvOwner->inventory().AddAvailableItems(items_list, false); // true
        UpdatePartnerBag();
    }
    else
    {
        VERIFY(m_pInvBox);
        m_pInvBox->set_in_use(true);
        m_pInvBox->AddAvailableItems(items_list);
    }

    std::sort(items_list.begin(), items_list.end(), InventoryUtilities::GreaterRoomInRuck);

    TIItemContainer::iterator it = items_list.begin();
    TIItemContainer::iterator it_e = items_list.end();
    for (; it != it_e; ++it)
    {
        CUICellItem* itm = create_cell_item(*it);
        m_pDeadBodyBagList->SetItem(itm);
    }

    CBaseMonster* monster = smart_cast<CBaseMonster*>(m_pPartnerInvOwner);

    // only for partner, box = no, monster = no
    if (m_pPartnerInvOwner && !monster)
    {
        CInfoPortionWrapper known_info_registry;
        known_info_registry.registry().init(m_pPartnerInvOwner->object_id());
        KNOWN_INFO_VECTOR& known_infos = known_info_registry.registry().objects();

        auto it = known_infos.begin();
        for (int i = 0; it != known_infos.end(); ++it, ++i)
        {
            NET_Packet P;
            CGameObject::u_EventGen(P, GE_INFO_TRANSFER, m_pActorInvOwner->object_id());
            P.w_u16(0);
            P.w_stringZ(*it);
            P.w_u8(1);
            CGameObject::u_EventSend(P);
        }
        known_infos.clear();
    }
    UpdateDeadBodyBag();
}

void CUIActorMenu::DeInitDeadBodySearchMode() const
{
    m_pDeadBodyBagList->Show(false);
    m_PartnerCharacterInfo->Show(false);
    m_LeftBackground->Show(false);
    m_PartnerBottomInfo->Show(false);
    m_PartnerWeight->Show(false);
    m_takeall_button->Show(false);

    if (m_pInvBox)
    {
        m_pInvBox->set_in_use(false);
    }
}

bool CUIActorMenu::ToDeadBodyBag(CUICellItem* itm, bool b_use_cursor_pos)
{
    if (m_pPartnerInvOwner)
    {
        if (!m_pPartnerInvOwner->deadbody_can_take_status())
        {
            return false;
        }
    }
    else // box
    {
        if (!m_pInvBox->can_take())
        {
            return false;
        }
    }
    PIItem quest_item = (PIItem)itm->m_pData;
    if (quest_item->IsQuestItem())
        return false;

    CUIDragDropListEx* old_owner = itm->OwnerList();
    CUIDragDropListEx* new_owner = nullptr;

    if (b_use_cursor_pos)
    {
        new_owner = CUIDragDropListEx::m_drag_item->BackList();
        VERIFY(new_owner == m_pDeadBodyBagList);
    }
    else
        new_owner = m_pDeadBodyBagList;

    CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

    if (b_use_cursor_pos)
        new_owner->SetItem(i, old_owner->GetDragItemPosition());
    else
        new_owner->SetItem(i);

    PIItem iitem = (PIItem)i->m_pData;

    if (m_pPartnerInvOwner)
    {
        move_item_from_to(m_pActorInvOwner->object_id(), m_pPartnerInvOwner->object_id(), iitem->object_id());
    }
    else // box
    {
        move_item_from_to(m_pActorInvOwner->object_id(), m_pInvBox->ID(), iitem->object_id());
    }

    UpdateDeadBodyBag();
    return true;
}

void CUIActorMenu::UpdateDeadBodyBag()
{
    string64 buf;

    LPCSTR kg_str = StringTable().translate("st_kg").c_str();
    float total = CalcItemsWeight(m_pDeadBodyBagList);
    xr_sprintf(buf, "%.1f %s", total, kg_str);
    m_PartnerWeight->SetText(buf);
    m_PartnerWeight->AdjustWidthToText();

    Fvector2 pos = m_PartnerWeight->GetWndPos();
    pos.x = m_PartnerWeight_end_x - m_PartnerWeight->GetWndSize().x - 5.0f;
    m_PartnerWeight->SetWndPos(pos);
    pos.x = pos.x - m_PartnerBottomInfo->GetWndSize().x - 5.0f;
    m_PartnerBottomInfo->SetWndPos(pos);
}

void CUIActorMenu::TakeAllFromPartner(CUIWindow* w, void* d)
{
    VERIFY(m_pActorInvOwner);
    if (!m_pPartnerInvOwner)
    {
        if (m_pInvBox)
        {
            TakeAllFromInventoryBox();
        }
        return;
    }

    u32 const cnt = m_pDeadBodyBagList->ItemsCount();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUICellItem* ci = m_pDeadBodyBagList->GetItemIdx(i);
        for (u32 j = 0; j < ci->ChildsCount(); ++j)
        {
            PIItem j_item = (PIItem)(ci->Child(j)->m_pData);
            move_item_check(j_item, m_pPartnerInvOwner, m_pActorInvOwner, false);
        }
        PIItem item = (PIItem)(ci->m_pData);
        move_item_check(item, m_pPartnerInvOwner, m_pActorInvOwner, false);
    } // for i
    m_pDeadBodyBagList->ClearAll(true); // false
}

void CUIActorMenu::TakeAllFromInventoryBox()
{
    u16 actor_id = m_pActorInvOwner->object_id();

    u32 const cnt = m_pDeadBodyBagList->ItemsCount();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUICellItem* ci = m_pDeadBodyBagList->GetItemIdx(i);
        for (u32 j = 0; j < ci->ChildsCount(); ++j)
        {
            PIItem j_item = (PIItem)(ci->Child(j)->m_pData);
            move_item_from_to(m_pInvBox->ID(), actor_id, j_item->object_id());
        }

        PIItem item = (PIItem)(ci->m_pData);
        move_item_from_to(m_pInvBox->ID(), actor_id, item->object_id());
    } // for i
    m_pDeadBodyBagList->ClearAll(true); // false
}
