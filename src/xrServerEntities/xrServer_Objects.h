////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrMessages.h"
#include "xrServer_Object_Base.h"
#include "phnetstate.h"

#pragma warning(push)
#pragma warning(disable:4005)

//------------------------------------------------------------------------------
// Version history
//------------------------------------------------------------------------------
// 10 - xrSE_ALifeObjectPhysic			appended with property 'fixed_bone'
// 11 - xrSE_ALifeObjectHangingLamp		appended with property 'spot_brightness'
// 12 - xrSE_ALifeObjectHangingLamp		appended with property 'flags'
// 13 - xrSE_ALifeObjectHangingLamp		appended with property 'mass'
// 14 - xrSE_ALifeObjectPhysic			inherited from CSE_ALifeObject
// 15 - xrSE_ALifeAnomalousZone			inherited calls from CSE_ALifeDynamicObject
// 16 - xrSE_ALifeObjectPhysic			inherited from CSE_ALifeDynamicObject
// 17 - xrSE_...						inherited from CSE_Visual for smart Level Editor
// 18 - xrSE_ALifeObjectHangingLamp		appended with 'startup_animation'
// 19 - xrSE_Teamed						didn't save health parameter
// 20 - xrSE_ALife...					saving vectors in UPDATE_Read/UPDATE_Write changed to STATE_Read/STATE_Write
// 21 -	GLOBAL CLASS HIERARCHY UPDATE
// 22 - CSE_AnomalousZone				appended with a artefact spawns
// 23 - CSE_ALifeObject					appended with a spawn ID
// 24 - CSE_ALifeObject					appended with a group control
// 25 - CSE_ALifeObject					changed type of the property probability from u8 to float
// 26 - CSE_AnomalousZone				appended with artefact spawn information
// 27 - CSE_AnomalousZone				weights changed type from u32 to float
// 28 - CSE_AnomalousZone				appended with an anomalous zone type
// 29 - CSE_ALifeObjectPhysic			appended with an animation property
// 30 - CSE_ALifeTrader					appended with an ordered artefacts property
// 31 - CSE_ALifeTrader					appended with a supplies property
// 32 - CSE_ALifeDynamicObjectVisual	the only this class saves and loads visual object
// 33 - CSE_ALifeGraphPoint and CSE_ALifeLevelChanger	level id changed to level name
// 34 - CSE_ALifeLevelPoint and CSE_ALifeLevelChanger	appended several _new properties
// 35 - CSE_ALifeTrader					artefact order structures changed
// 36 - CSE_ALifeTrader					appended with the organization identifier property
//		and CSE_ALifeHumanAbstract		appended with known_traders property
//		and CSE_ALifeTask				appended with try count property
//		and CSE_ALifePersonalTask		no more exists
// 37 - бинокль имеет ServerEntity не CSE_ALifeItemWeapon, а CSE_ALifeItem
// 38 - CSE_ALifeHumanAbstract			appended with the equipment and weapon preferences
// 39 - CSE_ALifeAnomalousZone			appended with the start power property
// 40 - CSE_ALifeObjectPhysic			appended with activate flag
//		and CSE_ALifeItemWeapon			appended with weapon addons state flag
// 41 - CSE_ALifeItemTorch				appended glow
// 42 - CSE_ALifeItemTorch				appended guid_bone
// 43 - CSE_ALifeObjectHangingLamp		appended glow_texture and glow_radius
// 44 - xrSE_ALifeObjectHangingLamp		appended with property 'fixed bones'
// 45 - xrSE_ALifeObjectHangingLamp		appended with property 'health'
// 46 - xrSE_ALifeObjectSearchLight		appended with property 'guid_bone', 
//										appended with property 'rotation_bone'
// 47 - CSE_ALifeItemWeapon				appended with ammo type index
// 48 - CSE_ALifeObjectSearchlight		appended with property 'cone_bone'
// 49 - CSE_ALifeObjectHangingLamp		remove some property
// 50 - CSE_ALifeObject					appended with ALife flags property
// 51 - CSE_ALifeItemBolt				new class based on CSE_ALifeDynamicObject and CSE_AlifeInventoryItem
// 52 - CSE_ALifeItemExplosive			new class based on CSE_ALifeItem
// 53 - CSE_AlifeInventoryItem			appended with property 'condition'
// 54 - CSE_AlifeLevelChanger			property 'angles' changed from float to fvector
// 55 - CSE_AlifeItemCar->CSE_ALifeCar	heritage changed
// 56 - CSE_ALifeObjectHangingLamp		restore removed props + appended some new
// 57 - CSE_ALifeObjectPhysic			appended with source_id
// 58 - CSE_ALifeObject					appended with ini_string
// 59 - CSE_ALifeItemPDA				appended with m_original_owner
// 60 - CSE_AlifeInventoryItem			appended with property m_eItemPlace
// 61 - CSE_AlifeObjectPhysic			appended with bones_mask, root_bone
// 62 - CSE_AlifeObject					appended with m_story_id
// 63 - CSE_AlifeTraderAbstract			remove bug with money
// 64 - CSE_ALifeObjectPhysic			flags, source_id, saved_bones moved to CSE_AlifePHSkeletonObject
// 65 - CSE_ALifeObjectPhysic			startup_anim moved to CSE_AlifePHSkeletonObject
// 66 - CSE_ALifeObjectPhysic			CSE_ALifeCar - heritage changed
// 67 - CSE_ALifeCustomZone				new class appended, heritage changed
// 68 - CSE_ALifeHumanStalker,				
//		CSE_ALifeMonsterBase			new class appended, heritage changed
// 69 -	object broker changed from this version
//		CSE_ALifeObjectHangingLamp,				
//		CSE_ALifeHelicopter				heritage changed
// 70 -	CSE_Abstract					appended with m_script_version, script version support
// 71 -	CSE_Abstract					appended with m_client_data, ability to save/load client data
// 72 -	CSE_AlifeInventoryItem			removed property m_eItemPlace, added flag
// 73 -	CSE_AlifeMonsterAbstract		appended with property m_space_restrictors
// 74 -	CSE_AlifeMonsterAbstract		appended with property m_in_space_restrictors
// 75 -	CSE_AlifeSpaceRestrictor		new class added, hierarchy changed
// 76 - CSE_AlifeTraderAbstract			appended with m_iSpecificCharacter
// 77 - CSE_AlifeTraderAbstract			added CSE_ObjectClimable
// 78 - CSE_AlifeTraderAbstract			appended with flags for infinite ammo
// 79 - CSE_AlifeAnomalousZone			appended with power properties (3)
// 80 - CSE_Abstract					m_tSpawnID property moved from CSE_ALifeObject to CSE_Abstract
// 81 - CSE_SpawnGroup					appended with 4 properties
// 82 - CSE_AlifeTraderAbstract			load/save m_iCharacterProfile on state read/write
// 83 - CSE_AlifeObject					m_fProbability moved to CSE_Abstract, m_dwSpawnGroup is removed completely
// 84 - CSE_SpawnGroup					appended with a property
// 85 - CSE_SpawnGroup					all the properties moved to CSE_Abstract
// 86 - CSE_AlifeTraderAbstract			appended with m_community_index
// 87 - CSE_AlifeTraderAbstract			appended with m_rank, m_reputation
// 88 - CSE_ALifeCreatureAbstract		appended with m_dynamic_restrictions property
// 89 - CSE_AlifeCreatureActor			appended with m_holderID
// 90 - CSE_ALifeItemPDA				appended with m_specific_character, m_info_portion
// 91 - CSE_ALifeHumanStalker			appended with property demo_mode
// 92 - CSE_AlifeCreatureActor			added base class CSE_PHSkeleton
// 93 - CSE_AlifeCar					health added in STATE_Read STATE_Write
// 94 - CSE_Abstract					client_data size increased
// 95 - CSE_ALifeCreatureAbstract		appended with m_killer_id property
// 96 - CSE_ALifeTraderAbstract			changed m_iCharacterProfile(int) to m_sCharacterProfile(shared_str)
// 97 - CSE_ALifeItemPDA				changed m_info_portion(int) to m_info_portion(shared_str) +m_specific_character +CSE_ALifeItemDocument m_wDoc (int-> shared_str)
// 98 - CSE_ALifeItemPDA				changed m_info_portion(int) to m_info_portion(shared_str) +m_specific_character +CSE_ALifeItemDocument m_wDoc (int-> shared_str)
// 99 - CSE_ALifeObjectClimable			inheritance changed CSE_Abstruct -> CSE_AlifeObject
// 100 - CSE_ALifeObjectClimable		inheritance changed CSE_AlifeObject -> CSE_AlifeDynamicObject
// 101 - CSE_ALifeCreaturePhantom		new class based on CSE_ALifeCreatureAbstract
// 102 - CSE_ALifeAnomalousZone			appended m_owner_id
// 103 - CSE_ALifeCustomZone			appended m_owner_id,removed from CSE_ALifeAnomalousZone
// 104 - CSE_Visual						appended flags
// 105 - CSE_ALifeTraderAbstract		added full name
// 106 - CSE_ALifeCustomZone			enabled\disabled time
// 107 - CSE_ALifeCustomZone			enabled\disabled time extended
// 108 - CSE_ALifeTraderAbstract		removed property m_tpEvents
// 109 - CSE_ALifeBaseMonster			added property m_spec_object
// 110 - CSE_ALifeHumanAbstract			removed a lot
// 111 - CSE_ALifeHumanStalker			removed demo mode
// 112 - CSE_Abstract					removed all the spawn properties, sad but true
//	   - CSE_SpawnGroup					is no more
//	   - CSE_Event						is no more
//	   - CSE_Target_CS					is no more
//	   - CSE_Target_CS_Cask				is no more
//	   - CSE_Target_CS_Base				is no more
//	   - CSE_TargetAssault				is no more
//	   - CSE_Target						is no more
//	   - CSE_ALifeObject				appended with property m_spawn_story_id
// 113 - CSE_ALifeAnomalousZone			removed property m_fStartPower
//	   - CSE_ALifeAnomalousZone			removed property m_faWeights
//	   - CSE_ALifeAnomalousZone			removed property m_cppArtefactSections
//	   - CSE_ALifeAnomalousZone			removed property m_min_start_power
//	   - CSE_ALifeAnomalousZone			removed property m_max_start_power
//	   - CSE_ALifeAnomalousZone			removed property m_power_artefact_factor
//	   - CSE_ALifeCustomZone			removed property m_attn
//	   - CSE_ALifeCustomZone			removed property m_period
// 114 - CSE_ALifeMonsterAbstract		m_task_reached, internal member, added
// 115 - CSE_ALifeCreatureAbstract		fHealth now in range 0..1
// 116 - CSE_ALifeCreatureAbstract		appended with property m_game_death_time
// 117 - CSE_ALifeLevelChanger			appended with property m_bSilentMode
// 118 - CALifeHumanBrain				removed property m_tpKnownCustomers, sad but true
// 119 - CSE_ALifeObjectHangingLamp		added property	m_volumetric_quality
//										added property	m_volumetric_intensity
//										added property	m_volumetric_distance
// 120 - CSE_SmartCover					added property	m_enter_min_enemy_distance
//										added property	m_exit_min_enemy_distance
// 121 - GameTypeChooser				m_gameType using instead og u8 value;
// 122 - CSE_ALifeItemWeapon			appended with count of grenades to spawn in grenade launcher (if it attached)
// 123 - CSE_ALifeInventoryItem			added functions has_upgrade & add_upgrade, xr_vector<shared_str> m_upgrades
// 124 - CSE_ALifeInventoryBox			added property m_can_take, m_closed
//       CSE_ALifeTraderAbstract		added property m_deadbody_can_take, m_deadbody_closed
// 127	 CSE_ALifeObjectClimable		added material;
// 128	 CSE_ALifeObjectClimable		added can_fire in smart covers;
//------------------------------------------------------------------------------
#define SPAWN_VERSION	u16(128)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_Shape,ISE_Shape,CShapeData)
public:
	void							cform_read		(NET_Packet& P);
	void							cform_write		(NET_Packet& P);
									CSE_Shape		();
	virtual							~CSE_Shape		();
	virtual ISE_Shape*  __stdcall	shape			() = 0;
	virtual void __stdcall			assign_shapes	(CShapeData::shape_def* shapes, u32 cnt);
};
add_to_type_list(CSE_Shape)
#define script_type_list save_type_list(CSE_Shape)

SERVER_ENTITY_DECLARE_BEGIN(CSE_Spectator,CSE_Abstract)
									CSE_Spectator	(LPCSTR caSection);
	virtual							~CSE_Spectator	();
	virtual u8						g_team			();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_Spectator)
#define script_type_list save_type_list(CSE_Spectator)

SERVER_ENTITY_DECLARE_BEGIN(CSE_Temporary,CSE_Abstract)
	u32								m_tNodeID;
									CSE_Temporary	(LPCSTR caSection);
	virtual							~CSE_Temporary	();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_Temporary)
#define script_type_list save_type_list(CSE_Temporary)

SERVER_ENTITY_DECLARE_BEGIN0(CSE_PHSkeleton)
								CSE_PHSkeleton(LPCSTR caSection);
virtual							~CSE_PHSkeleton();

enum{
	flActive					= (1<<0),
	flSpawnCopy					= (1<<1),
	flSavedData					= (1<<2),
	flNotSave					= (1<<3)
};
	Flags8							_flags;
	SPHBonesData					saved_bones;
	u16								source_id;//for break only
	virtual	void					load					(NET_Packet &tNetPacket);
	virtual bool					need_save				() const{return(!_flags.test(flNotSave));}
	virtual	void					set_sorce_id			(u16 si){source_id=si;}
	virtual u16						get_source_id			(){return source_id;}
	virtual CSE_Abstract			*cast_abstract			() {return 0;}
protected:
	virtual void					data_load				(NET_Packet &tNetPacket);
	virtual void					data_save				(NET_Packet &tNetPacket);
public:
SERVER_ENTITY_DECLARE_END
		add_to_type_list(CSE_PHSkeleton)
#define script_type_list save_type_list(CSE_PHSkeleton)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_AbstractVisual,CSE_Abstract,CSE_Visual)
	typedef CSE_Abstract			inherited1;
	typedef CSE_Visual				inherited2;

	CSE_AbstractVisual										(LPCSTR caSection);
	virtual	~CSE_AbstractVisual								();
	virtual CSE_Visual* __stdcall	visual					();
	LPCSTR							getStartupAnimation		();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_AbstractVisual)
#define script_type_list save_type_list(CSE_AbstractVisual)

#ifndef AI_COMPILER
extern CSE_Abstract	*F_entity_Create	(LPCSTR caSection);
#endif

/**
SERVER_ENTITY_DECLARE_BEGIN(CSE_SpawnGroup,CSE_Abstract)
public:
									CSE_SpawnGroup	(LPCSTR caSection);
	virtual							~CSE_SpawnGroup	();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_SpawnGroup)
#define script_type_list save_type_list(CSE_SpawnGroup)
/**/

#pragma warning(pop)
