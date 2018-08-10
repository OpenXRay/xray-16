#include "pch_script.h"
#include "UIActorMenu.h"
#include "UIInventoryUpgradeWnd.h"
#include "UIInvUpgradeInfo.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UICharacterInfo.h"
#include "inventory_item.h"
#include "UICellItem.h"
#include "InventoryOwner.h"
#include "Inventory.h"
#include "Actor.h"
#include "UIGameSP.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "inventory_upgrade.h"
#include "UITalkWnd.h"

void CUIActorMenu::InitUpgradeMode()
{
    m_PartnerCharacterInfo->Show(true);
    m_PartnerMoney->Show(false);
    m_pUpgradeWnd->Show(true);
    m_pQuickSlot->Show(true);

    InitInventoryContents(m_pInventoryBagList);
    VERIFY(m_pPartnerInvOwner);
    m_pPartnerInvOwner->StartTrading();
    //-	UpdateUpgradeItem();
}

void CUIActorMenu::DeInitUpgradeMode()
{
    m_PartnerCharacterInfo->Show(false);
    m_pUpgradeWnd->Show(false);
    m_pUpgradeWnd->set_info_cur_upgrade(NULL);
    m_pUpgradeWnd->m_btn_repair->Enable(false);

    if (m_upgrade_selected)
    {
        m_upgrade_selected->Mark(false);
        m_upgrade_selected = NULL;
    }
    if (m_pPartnerInvOwner)
    {
        m_pPartnerInvOwner->StopTrading();
    }

    if (!CurrentGameUI())
        return;
    //только если находимся в режиме single
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (!pGameSP)
        return;

    if (pGameSP->TalkMenu->IsShown())
    {
        pGameSP->TalkMenu->NeedUpdateQuestions();
    }
}

void CUIActorMenu::SetupUpgradeItem()
{
    if (m_upgrade_selected)
    {
        m_upgrade_selected->Mark(false);
    }

    bool can_upgrade = false;
    PIItem item = CurrentIItem();
    if (item)
    {
        m_upgrade_selected = CurrentItem();
        m_upgrade_selected->Mark(true);
        can_upgrade = CanUpgradeItem(item);
    }

    m_pUpgradeWnd->InitInventory(item, can_upgrade);
    if (m_upgrade_info)
    {
        m_upgrade_info->Show(false);
    }

    UpdateUpgradeItem();
}

void CUIActorMenu::UpdateUpgradeItem()
{
    //	m_pUpgradeWnd->InitInventory( CurrentIItem() );
}

void CUIActorMenu::TrySetCurUpgrade()
{
    if (!m_upgrade_info)
        return;
    Upgrade_type const* upgr = m_upgrade_info->get_upgrade();
    if (!upgr)
        return;
    m_pUpgradeWnd->DBClickOnUIUpgrade(upgr);
}

bool CUIActorMenu::SetInfoCurUpgrade(Upgrade_type* upgrade_type, CInventoryItem* inv_item)
{
    if (!m_upgrade_info)
        return false;
    bool res = m_upgrade_info->init_upgrade(upgrade_type, inv_item);

    if (!upgrade_type)
    {
        return false;
    }

    fit_in_rect(m_upgrade_info, Frect().set(0.0f, 0.0f, UI_BASE_WIDTH, UI_BASE_HEIGHT), 0.0f, GetWndRect().left);
    return res;
}

PIItem CUIActorMenu::get_upgrade_item() { return (m_upgrade_selected) ? (PIItem)m_upgrade_selected->m_pData : NULL; }
void CUIActorMenu::SeparateUpgradeItem()
{
    VERIFY(m_upgrade_selected);
    if (!m_upgrade_selected || !m_upgrade_selected->m_pData)
    {
        return;
    }
    CUIDragDropListEx* list_owner = m_upgrade_selected->OwnerList();
    if (list_owner && (GetListType(list_owner) != iActorBag))
    {
        return;
    }

    //	if ( m_upgrade_selected->ChildsCount() > 0 )
    //	{
    // PIItem item = get_upgrade_item();
    m_upgrade_selected->Mark(false);
    CUICellItem* ci = list_owner->RemoveItem(m_upgrade_selected, false);
    list_owner->SetItem(ci);
    //		m_upgrade_selected = ci;
    //		m_upgrade_selected->Mark( true );
    //	}
}
