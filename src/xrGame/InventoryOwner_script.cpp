#include "pch_script.h"
#include "InventoryOwner.h"
#include "GameObject.h"
#include "Inventory.h"
using namespace luabind;



static void CInventoryOwner_Export(lua_State* luaState)
{
        module(luaState)
		[
            class_<CInventory>("CInventory")
			.def("TotalWeight", &CInventory::TotalWeight)
			.def("CalcTotalWeight", &CInventory::CalcTotalWeight)
			.def("GetActiveSlot", &CInventory::GetActiveSlot)
			.def("SetActiveSlot", &CInventory::SetActiveSlot)
			.def("GetMaxWeight", &CInventory::GetMaxWeight)
			.def("SetMaxWeight", &CInventory::SetMaxWeight)
			.def("BeltWidth", &CInventory::BeltWidth)
			.def("Activate", &CInventory::Activate)
			.def("InSlot", &CInventory::InSlot)
			.def("InBelt", &CInventory::InBelt)
			.def("InRuck", &CInventory::InRuck)
			.def("ItemFromSlot", &CInventory::ItemFromSlot)
			,
			class_<CInventoryOwner>("CInventoryOwner")
			//.def(constructor<>())
			.def("trade_section", &CInventoryOwner::trade_section)
			.def("sell_useless_items", &CInventoryOwner::sell_useless_items)
			.def("buy_supplies", &CInventoryOwner::buy_supplies)
			.def("inventory", (CInventory& (CInventoryOwner::*)())&CInventoryOwner::inventory)
			.def("IconName", &CInventoryOwner::IconName)
			.def("get_money", &CInventoryOwner::get_money)
			.def("EnableTalk", &CInventoryOwner::EnableTalk)
			.def("DisableTalk", &CInventoryOwner::DisableTalk)
			.def("IsTalkEnabled", &CInventoryOwner::IsTalkEnabled)
			.def("EnableTrade", &CInventoryOwner::EnableTrade)
			.def("DisableTrade", &CInventoryOwner::DisableTrade)
			.def("IsTradeEnabled", &CInventoryOwner::IsTradeEnabled)
			.def("EnableInvUpgrade", &CInventoryOwner::EnableInvUpgrade)
			.def("DisableInvUpgrade", &CInventoryOwner::DisableInvUpgrade)
			.def("IsInvUpgradeEnabled", &CInventoryOwner::IsInvUpgradeEnabled)
			.def("GetTalkPartner", &CInventoryOwner::GetTalkPartner)
			.def("OfferTalk", &CInventoryOwner::OfferTalk)
			.def("StartTalk", &CInventoryOwner::StartTalk)
			.def("StopTalk", &CInventoryOwner::StopTalk)
			.def("IsTalking", &CInventoryOwner::IsTalking)
			.def("deadbody_can_take", &CInventoryOwner::deadbody_can_take)
			.def("deadbody_can_take_status", &CInventoryOwner::deadbody_can_take_status)
			.def("deadbody_closed", &CInventoryOwner::deadbody_closed)
			.def("deadbody_closed_status", &CInventoryOwner::deadbody_closed_status)
		];
};
SCRIPT_EXPORT_FUNC(CInventoryOwner, (), CInventoryOwner_Export);
