////////////////////////////////////////////////////////////////////////////
//	Module 		: script_game_object_script.cpp
//	Created 	: 25.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script game object script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"
#include "game_object_space.h"
#include "script_ini_file.h"
#include "sight_manager_space.h"

using namespace luabind;

extern class_<CScriptGameObject> &script_register_game_object1(class_<CScriptGameObject> &);
extern class_<CScriptGameObject> &script_register_game_object2(class_<CScriptGameObject> &);
extern class_<CScriptGameObject> &script_register_game_object_trader(class_<CScriptGameObject> &);

#pragma optimize("s",on)
void CScriptGameObject::script_register(lua_State *L)
{
	class_<CScriptGameObject>	instance("game_object");

	module(L)
	[
		class_<CSightParams>("CSightParams")
			.enum_("bla-bla")
			[
				value("eSightTypeCurrentDirection",		int(SightManager::eSightTypeCurrentDirection	)),
				value("eSightTypePathDirection",		int(SightManager::eSightTypePathDirection		)),
				value("eSightTypeDirection",			int(SightManager::eSightTypeDirection			)),
				value("eSightTypePosition",				int(SightManager::eSightTypePosition			)),
				value("eSightTypeObject",				int(SightManager::eSightTypeObject				)),
				value("eSightTypeCover",				int(SightManager::eSightTypeCover				)),
				value("eSightTypeSearch",				int(SightManager::eSightTypeSearch				)),
				value("eSightTypeLookOver",				int(SightManager::eSightTypeLookOver			)),
				value("eSightTypeCoverLookOver",		int(SightManager::eSightTypeCoverLookOver		)),
				value("eSightTypeFireObject",			int(SightManager::eSightTypeFireObject			)),
				value("eSightTypeFirePosition",			int(SightManager::eSightTypeFirePosition		)),
				value("eSightTypeAnimationDirection",	int(SightManager::eSightTypeAnimationDirection	)),
				value("eSightTypeDummy",				int(SightManager::eSightTypeDummy				))
			]
			.def(							constructor<>())
			.def_readonly("m_object",		&CSightParams::m_object)
			.def_readonly("m_vector",		&CSightParams::m_vector)
			.def_readonly("m_sight_type",	&CSightParams::m_sight_type),
		
		script_register_game_object2(
			script_register_game_object1(
				script_register_game_object_trader(instance)
			)
		),

		class_<enum_exporter<GameObject::ECallbackType> >("callback")
			.enum_("callback_types")
			[
				value("trade_start",				int(GameObject::eTradeStart)),
				value("trade_stop",					int(GameObject::eTradeStop)),
				value("trade_sell_buy_item",		int(GameObject::eTradeSellBuyItem)),
				value("trade_perform_operation",	int(GameObject::eTradePerformTradeOperation)),
				value("trader_global_anim_request",	int(GameObject::eTraderGlobalAnimationRequest)),
				value("trader_head_anim_request",	int(GameObject::eTraderHeadAnimationRequest)),
				value("trader_sound_end",			int(GameObject::eTraderSoundEnd)),
				value("zone_enter",					int(GameObject::eZoneEnter)),
				value("zone_exit",					int(GameObject::eZoneExit)),
				value("level_border_exit",			int(GameObject::eExitLevelBorder)),
				value("level_border_enter",			int(GameObject::eEnterLevelBorder)),
				value("death",						int(GameObject::eDeath)),
				value("patrol_path_in_point",		int(GameObject::ePatrolPathInPoint)),
				value("inventory_pda",				int(GameObject::eInventoryPda)),
				value("inventory_info",				int(GameObject::eInventoryInfo)),
				value("article_info",				int(GameObject::eArticleInfo)),
				value("use_object",					int(GameObject::eUseObject)),
				value("hit",						int(GameObject::eHit)),
				value("sound",						int(GameObject::eSound)),
				value("action_removed",				int(GameObject::eActionTypeRemoved)),
				value("action_movement",			int(GameObject::eActionTypeMovement)),
				value("action_watch",				int(GameObject::eActionTypeWatch)),
				value("action_animation",			int(GameObject::eActionTypeAnimation)),
				value("action_sound",				int(GameObject::eActionTypeSound)),
				value("action_particle",			int(GameObject::eActionTypeParticle)),
				value("action_object",				int(GameObject::eActionTypeObject)),
				value("actor_sleep",				int(GameObject::eActorSleep)),
				value("helicopter_on_point",		int(GameObject::eHelicopterOnPoint)),
				value("helicopter_on_hit",			int(GameObject::eHelicopterOnHit)),
				value("on_item_take",				int(GameObject::eOnItemTake)),
				value("on_item_drop",				int(GameObject::eOnItemDrop)),
				value("script_animation",			int(GameObject::eScriptAnimation)),
				value("task_state",					int(GameObject::eTaskStateChange)),
				value("take_item_from_box",			int(GameObject::eInvBoxItemTake)),
				value("weapon_no_ammo",				int(GameObject::eWeaponNoAmmoAvailable)),
				
				value("map_location_added",			int(GameObject::eMapLocationAdded))
			],

		def("buy_condition",				(void (*)(CScriptIniFile*,LPCSTR))(&::buy_condition)),
		def("buy_condition",				(void (*)(float,float))(&::buy_condition)),
		def("sell_condition",				(void (*)(CScriptIniFile*,LPCSTR))(&::sell_condition)),
		def("sell_condition",				(void (*)(float,float))(&::sell_condition)),
		def("show_condition",				&::show_condition)
	];
}