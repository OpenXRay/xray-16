#include "pch_script.h"
#include "InventoryOwner.h"
#include "GameObject.h"
#include "Inventory.h"
#include "inventory_space.h"
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
			.enum_("slots")
			[
				luabind::value("NO_ACTIVE_SLOT", NO_ACTIVE_SLOT),
				luabind::value("KNIFE_SLOT", KNIFE_SLOT),
				luabind::value("INV_SLOT_2", INV_SLOT_2),
				luabind::value("INV_SLOT_3", INV_SLOT_3),
				luabind::value("GRENADE_SLOT", GRENADE_SLOT),
				luabind::value("BINOCULAR_SLOT", BINOCULAR_SLOT),
				luabind::value("BOLT_SLOT", BOLT_SLOT),
				luabind::value("OUTFIT_SLOT", OUTFIT_SLOT),
				luabind::value("PDA_SLOT", PDA_SLOT),
				luabind::value("DETECTOR_SLOT", DETECTOR_SLOT),
				luabind::value("TORCH_SLOT", TORCH_SLOT),
				luabind::value("ARTEFACT_SLOT", ARTEFACT_SLOT),
				luabind::value("HELMET_SLOT", HELMET_SLOT),
				luabind::value("BACKPACK_SLOT", BACKPACK_SLOT),
				luabind::value("PATCH_SLOT", PATCH_SLOT),
#ifdef MORE_INVENTORY_SLOTS
				luabind::value("CUSTOM_SLOT_1", CUSTOM_SLOT_1),
				luabind::value("CUSTOM_SLOT_2", CUSTOM_SLOT_2),
				luabind::value("CUSTOM_SLOT_3", CUSTOM_SLOT_3),
				luabind::value("CUSTOM_SLOT_4", CUSTOM_SLOT_4),
				luabind::value("CUSTOM_SLOT_5", CUSTOM_SLOT_5),
				luabind::value("CUSTOM_SLOT_6", CUSTOM_SLOT_6),
				luabind::value("CUSTOM_SLOT_7", CUSTOM_SLOT_7),
				luabind::value("CUSTOM_SLOT_8", CUSTOM_SLOT_8),
				luabind::value("CUSTOM_SLOT_9", CUSTOM_SLOT_9),
				luabind::value("CUSTOM_SLOT_10", CUSTOM_SLOT_10),
#endif
				luabind::value("LAST_SLOT", LAST_SLOT)
			]
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
