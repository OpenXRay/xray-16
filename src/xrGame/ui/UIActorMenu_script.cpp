////////////////////////////////////////////////////////////////////////////
//	Module 		: UIActorMenu_script.cpp
//	Created 	: 18.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI ActorMenu script implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "UIActorMenu.h"
#include "UIPdaWnd.h"
#include "Actor.h"
#include "inventory_item.h"
#include "UICellItem.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrUICore/TabControl/UITabControl.h"
#include "xrGame/ui/UIMainIngameWnd.h"
#include "eatable_item.h"

void CUIActorMenu::TryRepairItem(CUIWindow* w, void* d)
{
    PIItem item = get_upgrade_item();
    if (!item)
    {
        return;
    }
    if (item->GetCondition() > 0.99f)
    {
        return;
    }
    LPCSTR item_name = item->m_section_id.c_str();

    CEatableItem* EItm = smart_cast<CEatableItem*>(item);
    if (EItm)
    {
        bool allow_repair = !!READ_IF_EXISTS(pSettings, r_bool, item_name, "allow_repair", false);
        if (!allow_repair)
            return;
    }

    LPCSTR partner = m_pPartnerInvOwner->CharacterInfo().Profile().c_str();

    luabind::functor<bool> funct;
    R_ASSERT2(GEnv.ScriptEngine->functor("inventory_upgrades.can_repair_item", funct),
        make_string("Failed to get functor <inventory_upgrades.can_repair_item>, item = %s", item_name));
    bool can_repair = funct(item_name, item->GetCondition(), partner);

    luabind::functor<LPCSTR> funct2;
    R_ASSERT2(GEnv.ScriptEngine->functor("inventory_upgrades.question_repair_item", funct2),
        make_string("Failed to get functor <inventory_upgrades.question_repair_item>, item = %s", item_name));
    LPCSTR question = funct2(item_name, item->GetCondition(), can_repair, partner);

    if (can_repair)
    {
        m_repair_mode = true;
        CallMessageBoxYesNo(question);
    }
    else
        CallMessageBoxOK(question);
}

void CUIActorMenu::RepairEffect_CurItem()
{
    PIItem item = CurrentIItem();
    if (!item)
    {
        return;
    }
    LPCSTR item_name = item->m_section_id.c_str();

    luabind::functor<void> funct;
    if (GEnv.ScriptEngine->functor("inventory_upgrades.effect_repair_item", funct))
        funct(item_name, item->GetCondition());

    item->SetCondition(1.0f);
    SeparateUpgradeItem();
    CUICellItem* itm = CurrentItem();
    if (itm)
        itm->UpdateConditionProgressBar();
}

bool CUIActorMenu::CanUpgradeItem(PIItem item)
{
    VERIFY(item && m_pPartnerInvOwner);
    LPCSTR item_name = item->m_section_id.c_str();
    LPCSTR partner = m_pPartnerInvOwner->CharacterInfo().Profile().c_str();

    luabind::functor<bool> funct;
    R_ASSERT2(GEnv.ScriptEngine->functor("inventory_upgrades.can_upgrade_item", funct),
        make_string("Failed to get functor <inventory_upgrades.can_upgrade_item>, item = %s, mechanic = %s", item_name,
            partner));

    return funct(item_name, partner);
}

void CUIActorMenu::CurModeToScript()
{
    int mode = (int)m_currMenuMode;
    luabind::functor<void> funct;
    if (GEnv.ScriptEngine->functor("actor_menu.actor_menu_mode", funct))
        funct(mode);
}

template<class T>
class enum_dummy {};

SCRIPT_EXPORT(CUIActorMenu, (CUIDialogWnd),
{
    using namespace luabind;

    module(luaState)
    [
        class_<enum_dummy<EDDListType>>("EDDListType")
            .enum_("EDDListType")
            [
                value("iActorBag", int(EDDListType::iActorBag)),
                value("iActorBelt", int(EDDListType::iActorBelt)),
                value("iActorSlot", int(EDDListType::iActorSlot)),
                value("iActorTrade", int(EDDListType::iActorTrade)),
                value("iDeadBodyBag", int(EDDListType::iDeadBodyBag)),
                value("iInvalid", int(EDDListType::iInvalid)),
                value("iPartnerTrade", int(EDDListType::iPartnerTrade)),
                value("iPartnerTradeBag", int(EDDListType::iPartnerTradeBag)),
                value("iQuickSlot", int(EDDListType::iQuickSlot)),
                value("iTrashSlot", int(EDDListType::iTrashSlot))
            ],

        class_<CUIActorMenu, CUIDialogWnd>("CUIActorMenu")
            .def(constructor<>())
            .def("get_drag_item", &CUIActorMenu::GetCurrentItemAsGameObject)
            .def("highlight_section_in_slot", &CUIActorMenu::HighlightSectionInSlot)
            .def("highlight_for_each_in_slot", &CUIActorMenu::HighlightForEachInSlot)
            .def("refresh_current_cell_item", &CUIActorMenu::RefreshCurrentItemCell)
            .def("IsShown", &CUIActorMenu::IsShown)
            .def("ShowDialog", &CUIActorMenu::ShowDialog)
            .def("HideDialog", &CUIActorMenu::HideDialog)
    ];

    using namespace luabind;

    module(luaState, "ActorMenu")
    [
        def("get_pda_menu", +[](){ return &CurrentGameUI()->GetPdaMenu(); }),
        def("get_actor_menu", +[](){ return &CurrentGameUI()->GetActorMenu(); }),
        def("get_menu_mode", +[](){ return CurrentGameUI()->GetActorMenu().GetMenuMode(); }),
        def("get_maingame", +[](){ return CurrentGameUI()->UIMainIngameWnd; })
    ];
});

SCRIPT_EXPORT(CUIPdaWnd, (CUIDialogWnd),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIPdaWnd, CUIDialogWnd>("CUIPdaWnd")
            .def(constructor<>())
            .def("IsShown", &CUIPdaWnd::IsShown)
            .def("ShowDialog", &CUIPdaWnd::ShowDialog)
            .def("HideDialog", &CUIPdaWnd::HideDialog)
            .def("SetActiveSubdialog", &CUIPdaWnd::SetActiveSubdialog_script)
            .def("GetActiveSection", &CUIPdaWnd::GetActiveSection)
            .def("GetTabControl", &CUIPdaWnd::GetTabControl)
    ];
});
