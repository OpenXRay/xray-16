global
	function alife()
	function app_ready()
	function bind_to_dik(enum EGameActions, number)
	function bit_and(number, number)
	function bit_not(number)
	function bit_or(number, number)
	function bit_xor(number, number)
	function buy_condition(ini_file*, string)
	function buy_condition(number, number)
	function cast_planner(action_base*)
	function command_line()
	function create_ini_file(string)
	function create_ini_file(string, string)
	function device()
	function dik_to_bind(number)
	function editor()
	function ef_storage()
	function error_log(string)
	function FitInRect(CUIWindow*, const Frect&, number, number)
	function flush()
	function game_graph()
	function game_ini()
	function get_console()
	function get_hud()
	function GetARGB(number, number, number, number)
	function GetCursorPosition()
	function GetFontDI()
	function GetFontGraffiti19Russian()
	function GetFontGraffiti22Russian()
	function GetFontGraffiti32Russian()
	function GetFontGraffiti50Russian()
	function GetFontLetterica16Russian()
	function GetFontLetterica18Russian()
	function GetFontLetterica25()
	function GetFontMedium()
	function GetFontSmall()
	function getFS()
	function is_enough_address_space_available()
	function IsDynamicMusic()
	function IsGameTypeSingle()
	function IsImportantSave()
	function key_state(number)
	function log(string)
	function prefetch(string)
	function print_stack()
	function reload_system_ini()
	function render_get_dx_level()
	function script_server_object_version()
	function sell_condition(ini_file*, string)
	function sell_condition(number, number)
	function SetCursorPosition(vector2&)
	function show_condition(ini_file*, string)
	function system_ini()
	function time_global_async()
	function user_name()
	function valid_saved_game(string)
	function verify_if_thread_is_running()

namespace actor_stats
	function add_points(string, string, number, number)
	function add_points_str(string, string, string)
	function get_points(string)

namespace ActorMenu
	function get_actor_menu()
	function get_maingame()
	function get_menu_mode()
	function get_pda_menu()

namespace game
	function active_tutorial_name()
	function get_game_time()
	function has_active_tutorial()
	function log_stack_trace(string)
	function reload_language()
	function start_tutorial(string)
	function stop_tutorial()
	function time()
	function translate_string(string)

namespace level
	function add_call(const function<boolean>&, const function<void>&)
	function add_call(object, const function<boolean>&, const function<void>&)
	function add_call(object, string, string)
	function add_cam_effector(string, number, boolean, string)
	function add_cam_effector2(string, number, boolean, string, number)
	function add_complex_effector(string, number)
	function add_dialog_to_render(CUIDialogWnd*)
	function add_pp_effector(string, number, boolean)
	function change_game_time(number, number, number)
	function check_object(game_object*)
	function client_spawn_manager()
	function debug_actor()
	function debug_object(string)
	function disable_input()
	function enable_input()
	function environment()
	function game_id()
	function get_active_cam()
	function get_bounding_volume()
	function get_game_difficulty()
	function get_snd_volume()
	function get_start_time()
	function get_target_dist()
	function get_target_element()
	function get_target_obj()
	function get_time_days()
	function get_time_factor()
	function get_time_hours()
	function get_time_minutes()
	function get_view_entity()
	function get_weather()
	function get_wfx_time()
	function hide_indicators()
	function hide_indicators_safe()
	function high_cover_in_direction(number, const vector&)
	function hold_action(enum EGameActions)
	function is_wfx_playing()
	function iterate_sounds(string, number, function<void>)
	function iterate_sounds(string, number, object, function<void>)
	function low_cover_in_direction(number, const vector&)
	function map_add_object_spot(number, string, string)
	function map_add_object_spot_ser(number, string, string)
	function map_change_spot_hint(number, string, string)
	function map_has_object_spot(number, string)
	function map_manager()
	function map_remove_object_spot(number, string)
	function name()
	function object_by_id(number)
	function patrol_path_exists(string)
	function physics_world()
	function prefetch_sound(string)
	function present()
	function press_action(enum EGameActions)
	function rain_factor()
	function ray_pick(const vector&, const vector&, number, enum collide::rq_target, rq_result&, game_object*)
	function release_action(enum EGameActions)
	function remove_call(const function<boolean>&, const function<void>&)
	function remove_call(object, const function<boolean>&, const function<void>&)
	function remove_call(object, string, string)
	function remove_calls_for_object(object)
	function remove_cam_effector(number)
	function remove_complex_effector(number)
	function remove_dialog_to_render(CUIDialogWnd*)
	function remove_pp_effector(number)
	function send(net_packet&, boolean, boolean, boolean, boolean)
	function set_active_cam(number)
	function set_game_difficulty(enum ESingleGameDifficulty)
	function set_pp_effector_factor(number, number)
	function set_pp_effector_factor(number, number, number)
	function set_snd_volume(number)
	function set_time_factor(number)
	function set_view_entity(game_object*)
	function set_weather(string, boolean)
	function set_weather_fx(string)
	function show_indicators()
	function show_weapon(boolean)
	function spawn_item(string, vector, number, number, boolean)
	function spawn_phantom(const vector&)
	function start_weather_fx_from_time(string, number)
	function stop_weather_fx()
	function u_event_gen(net_packet&, number, number)
	function u_event_send(net_packet&)
	function valid_vertex(number)
	function vertex_id(vector)
	function vertex_in_direction(number, vector, number)
	function vertex_position(number)

namespace main_menu
	function get_main_menu()

namespace relation_registry
	function change_community_goodwill(string, number, number)
	function community_goodwill(string, number)
	function community_relation(string, string)
	function get_general_goodwill_between(number, number)
	function set_community_goodwill(string, number, number)
	function set_community_relation(string, string, number)

class account_manager
	function create_profile(string, string, string, string, account_operation_cb)
	function delete_profile(account_operation_cb)
	function get_account_profiles(string, string, account_profiles_cb)
	function get_found_profiles() const
	function get_suggested_unicks() const
	function get_verify_error_descr() const
	function is_email_searching_active() const
	function is_get_account_profiles_active() const
	function search_for_email(string, found_email_cb)
	function stop_fetching_account_profiles()
	function stop_searching_email()
	function stop_suggest_unique_nicks()
	function suggest_unique_nicks(string, suggest_nicks_cb)
	function verify_email(string)
	function verify_password(string)
	function verify_unique_nick(string)

class account_operation_cb
	account_operation_cb()
	account_operation_cb(object, function<void>)

	function bind(object, function<void>)
	function clear()

class account_profiles_cb
	account_profiles_cb()
	account_profiles_cb(object, function<void>)

	function bind(object, function<void>)
	function clear()

class act
	const attack = 2
	const eat = 1
	const panic = 3
	const rest = 0

	act()
	act(enum MonsterSpace::EScriptMonsterGlobalAction)
	act(enum MonsterSpace::EScriptMonsterGlobalAction, game_object*)

class action_base
	action_base()
	action_base(game_object*)
	action_base(game_object*, string)

	property object
	property storage

	function add_effect(const world_property&)
	function add_precondition(const world_property&)
	function execute()
	function finalize()
	function initialize()
	function remove_effect(const number&)
	function remove_precondition(const number&)
	function set_weight(const number&)
	function setup(game_object*, property_storage*)
	function show(string)

class action_planner
	action_planner()

	property object
	property storage

	function action(const number&)
	function actual(const action_planner*)
	function add_action(const number&, action_base*)
	function add_evaluator(const number&, property_evaluator*)
	function clear()
	function current_action()
	function current_action_id() const
	function evaluator(const number&)
	function initialized() const
	function remove_action(const number&)
	function remove_evaluator(const number&)
	function set_goal_world_state(action_planner*, world_state*)
	function setup(game_object*)
	function show(string)
	function update()

class alife_simulator
	function actor(const alife_simulator*)
	function add_in_restriction(alife_simulator*, cse_alife_monster_abstract*, number)
	function add_out_restriction(alife_simulator*, cse_alife_monster_abstract*, number)
	function clone_weapon(alife_simulator*, cse_abstract*, string, const vector&, number, number, number, boolean)
	function create(alife_simulator*, number)
	function create(alife_simulator*, string, const vector&, number, number)
	function create(alife_simulator*, string, const vector&, number, number, number)
	function create(alife_simulator*, string, const vector&, number, number, number, boolean)
	function create_ammo(alife_simulator*, string, const vector&, number, number, number, number)
	function disable_info(const alife_simulator*, const number&, string)
	function dont_has_info(const alife_simulator*, const number&, string)
	function get_children(const alife_simulator*, cse_abstract*)
	function give_info(const alife_simulator*, const number&, string)
	function has_info(const alife_simulator*, const number&, string)
	function iterate_info(const alife_simulator*, const number&, const function<boolean>&)
	function kill_entity(alife_simulator*, cse_alife_monster_abstract*)
	function kill_entity(alife_simulator*, cse_alife_monster_abstract*, const number&)
	function kill_entity(cse_alife_monster_abstract*, const number&, cse_alife_schedulable*)
	function level_id(alife_simulator*)
	function level_name(const alife_simulator*, number)
	function object(const alife_simulator*, number)
	function object(const alife_simulator*, number, boolean)
	function objects(const alife_simulator*)
	function register(alife_simulator*, cse_abstract*)
	function release(alife_simulator*, cse_abstract*)
	function release(alife_simulator*, cse_abstract*, boolean)
	function remove_all_restrictions(number, const enum RestrictionSpace::ERestrictorTypes&)
	function remove_in_restriction(alife_simulator*, cse_alife_monster_abstract*, number)
	function remove_out_restriction(alife_simulator*, cse_alife_monster_abstract*, number)
	function set_interactive(number, boolean)
	function set_objects_per_update(alife_simulator*, number)
	function set_process_time(number)
	function set_switch_distance(number)
	function set_switch_offline(number, boolean)
	function set_switch_online(number, boolean)
	function spawn_id(alife_simulator*, number)
	function story_object(const alife_simulator*, number)
	function switch_distance() const
	function teleport_object(number, number, number, const vector&)
	function valid_object_id(const alife_simulator*, number)

class anim
	const attack = 7
	const capture_prepare = 1
	const danger = 0
	const eat = 4
	const free = 1
	const lie_idle = 3
	const look_around = 8
	const panic = 2
	const rest = 6
	const sit_idle = 2
	const sleep = 5
	const stand_idle = 0
	const turn = 9

	anim()
	anim(enum MonsterSpace::EMentalState)
	anim(string)
	anim(enum MonsterSpace::EScriptMonsterAnimAction, number)
	anim(string, boolean)

	function anim(string)
	function completed()
	function type(enum MonsterSpace::EMentalState)

class award_data
	property m_count
	property m_last_reward_date

class award_pair_t
	property first
	property second

class best_scores_pair_t
	property first
	property second

class CActor (CGameObject)
	CActor()

	property CurrentHeight
	property m_fClimbFactor
	property m_fCrouchFactor
	property m_fJumpSpeed
	property m_fRun_StrafeFactor
	property m_fRunBackFactor
	property m_fRunFactor
	property m_fSprintFactor
	property m_fWalk_StrafeFactor
	property m_fWalkAccel
	property m_fWalkBackFactor

	function _construct()
	function attach_Vehicle(holder*)
	function CameraHeight()
	function CanRun()
	function CanSprint()
	function conditions() const
	function detach_Vehicle()
	function get_additional_weight() const
	function get_state() const
	function get_state_wishful() const
	function GetDefaultActionForObject()
	function getEnabled() const
	function GetFireDispersion() const
	function GetNightVisionStatus()
	function GetProtection_ArtefactsOnBelt(enum ALife::EHitType)
	function getVisible() const
	function GetWeaponAccuracy() const
	function is_jump()
	function IsZoomAimingMode() const
	function MaxCarryWeight() const
	function MaxWalkWeight() const
	function MoveActor(vector, vector)
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function ObjectWeLookingAt()
	function RepackAmmo()
	function set_state(number)
	function set_state_wishful(number)
	function SetNightVisionAllowed(boolean)
	function use(CGameObject*)
	function use_HolderEx(holder*, boolean)
	function Visual() const

class CActorCondition (CEntityCondition)
	const eBoostBleedingRestore = 3
	const eBoostBurnImmunity = 8
	const eBoostChemicalBurnImmunity = 12
	const eBoostChemicalBurnProtection = 7
	const eBoostExplImmunity = 13
	const eBoostFireWoundImmunity = 15
	const eBoostHpRestore = 0
	const eBoostMaxWeight = 4
	const eBoostPowerRestore = 1
	const eBoostRadiationImmunity = 10
	const eBoostRadiationProtection = 5
	const eBoostRadiationRestore = 2
	const eBoostShockImmunity = 9
	const eBoostStrikeImmunity = 14
	const eBoostTelepaticImmunity = 11
	const eBoostTelepaticProtection = 6
	const eBoostWoundImmunity = 16
	const eCantWalkWeight = 128
	const eCantWalkWeightReached = 256
	const eCriticalBleedingSpeed = 4
	const eCriticalPowerReached = 1
	const eCriticalRadiationReached = 16
	const eCriticalSatietyReached = 8
	const ePhyHealthMinReached = 64
	const eWeaponJammedReached = 32

	property m_condition_flags
	property m_fAccelK
	property m_fJumpPower
	property m_fJumpWeightPower
	property m_fOverweightJumpK
	property m_fOverweightWalkK
	property m_fSprintK
	property m_fStandPower
	property m_fWalkWeightPower
	property m_MaxWalkWeight

	function ApplyBooster(CActorCondition*, const SBooster&, string)
	function BleedingSpeed()
	function BoostBleedingRestore(number)
	function BoostBurnImmunity(number)
	function BoostChemicalBurnImmunity(number)
	function BoostChemicalBurnProtection(number)
	function BoosterForEach(CActorCondition*, const function<boolean>&)
	function BoostExplImmunity(number)
	function BoostFireWoundImmunity(number)
	function BoostHpRestore(number)
	function BoostMaxWeight(number)
	function BoostPowerRestore(number)
	function BoostRadiationImmunity(number)
	function BoostRadiationProtection(number)
	function BoostRadiationRestore(number)
	function BoostShockImmunity(number)
	function BoostStrikeImmunity(number)
	function BoostTelepaticImmunity(number)
	function BoostTelepaticProtection(number)
	function BoostWoundImmunity(number)
	function ChangeAlcohol(number)
	function ChangeBleeding(number)
	function ChangeEntityMorale(number)
	function ChangeHealth(number)
	function ChangePower(number)
	function ChangePsyHealth(number)
	function ChangeRadiation(number)
	function ChangeSatiety(number)
	function GetEntityMorale() const
	function GetHealthLost() const
	function GetMaxPower() const
	function GetPower() const
	function GetPsyHealth() const
	function GetRadiation() const
	function GetSatiety() const
	function GetSatiety()
	function GetWhoHitLastTimeID()
	function IsCantSprint() const
	function IsCantWalk() const
	function IsCantWalkWeight()
	function IsLimping() const
	function SatietyCritical()
	function SetMaxPower(number)
	function V_Satiety()
	function V_SatietyHealth()
	function V_SatietyPower()

class CAdvancedDetector (CGameObject)
	CAdvancedDetector()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CAI_Bloodsucker (CGameObject)
	CAI_Bloodsucker()

	function _construct()
	function force_visibility_state(number)
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CAI_Boar (CGameObject)
	CAI_Boar()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CAI_Dog (CGameObject)
	CAI_Dog()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CAI_Flesh (CGameObject)
	CAI_Flesh()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CAI_PseudoDog (CGameObject)
	CAI_PseudoDog()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CAI_Stalker (CGameObject)
	CAI_Stalker()

	function _construct()
	function conditions() const
	function getEnabled() const
	function getVisible() const
	function GetWeaponAccuracy() const
	function inside_anomaly()
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CAI_Trader (CGameObject)
	CAI_Trader()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CALifeHumanBrain (CALifeMonsterBrain)
	function can_choose_alife_tasks(boolean)
	function movement(const CALifeMonsterBrain*)
	function update()

class CALifeMonsterBrain
	function can_choose_alife_tasks(boolean)
	function movement(const CALifeMonsterBrain*)
	function update()

class CALifeMonsterDetailPathManager
	function actual() const
	function completed() const
	function failed() const
	function speed() const
	function speed(const number&)
	function target(const CALifeSmartTerrainTask*)
	function target(const number&)
	function target(const number&, const number&, const vector&)

class CALifeMonsterMovementManager
	function actual() const
	function completed() const
	function detail(const CALifeMonsterMovementManager*)
	function path_type() const
	function path_type(const enum MovementManager::EPathType&)
	function patrol(const CALifeMonsterMovementManager*)

class CALifeMonsterPatrolPathManager
	function actual() const
	function completed() const
	function path(string)
	function route_type() const
	function route_type(const enum PatrolPathManager::EPatrolRouteType&)
	function start_type() const
	function start_type(const enum PatrolPathManager::EPatrolStartType&)
	function start_vertex_index(const number&)
	function target_game_vertex_id() const
	function target_level_vertex_id() const
	function target_position(CALifeMonsterPatrolPathManager*)
	function use_randomness() const
	function use_randomness(const boolean&)

class CALifeSmartTerrainTask
	CALifeSmartTerrainTask(string)
	CALifeSmartTerrainTask(number, number)
	CALifeSmartTerrainTask(string, number)

	function game_vertex_id() const
	function level_vertex_id() const
	function position() const

class callback
	const action_animation = 21
	const action_movement = 18
	const action_object = 24
	const action_particle = 23
	const action_removed = 20
	const action_sound = 22
	const action_watch = 19
	const actor_before_death = 48
	const actor_sleep = 25
	const article_info = 12
	const death = 8
	const helicopter_on_hit = 27
	const helicopter_on_point = 26
	const hit = 16
	const hud_animation_end = 36
	const inventory_info = 11
	const inventory_pda = 10
	const item_to_belt = 40
	const item_to_ruck = 42
	const item_to_slot = 41
	const key_hold = 39
	const key_press = 37
	const key_release = 38
	const level_border_enter = 7
	const level_border_exit = 6
	const map_location_added = 14
	const on_attach_vehicle = 49
	const on_detach_vehicle = 50
	const on_item_drop = 29
	const on_item_take = 28
	const on_use_vehicle = 51
	const patrol_path_in_point = 9
	const script_animation = 30
	const sound = 17
	const take_item_from_box = 34
	const task_state = 13
	const trade_perform_operation = 3
	const trade_sell_buy_item = 2
	const trade_start = 0
	const trade_stop = 1
	const trader_global_anim_request = 31
	const trader_head_anim_request = 32
	const trader_sound_end = 33
	const use_object = 15
	const weapon_fired = 46
	const weapon_jammed = 45
	const weapon_magazine_empty = 47
	const weapon_no_ammo = 35
	const weapon_zoom_in = 43
	const weapon_zoom_out = 44
	const zone_enter = 4
	const zone_exit = 5

class CAmebaZone (CGameObject)
	CAmebaZone()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CAntirad (CGameObject)
	CAntirad()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CArtefact (CGameObject)
	CArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CBastArtefact (CArtefact)
	CBastArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CBlackDrops (CArtefact)
	CBlackDrops()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CBlackGraviArtefact (CArtefact)
	CBlackGraviArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CBlend
class CBottleItem (CGameObject)
	CBottleItem()

	function _construct()
	function BreakToPieces()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CBurer (CGameObject)
	CBurer()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CCar (CGameObject, holder)
	const eWpnActivate = 3
	const eWpnAutoFire = 5
	const eWpnDesiredDir = 1
	const eWpnDesiredPos = 2
	const eWpnFire = 4
	const eWpnToDefaultDir = 6

	CCar()

	function _construct()
	function Action(number, number)
	function CanHit()
	function CarExplode()
	function ChangefFuel(number)
	function ChangefHealth(number)
	function CurrentVel()
	function engaged()
	function ExplodeTime()
	function FireDirDiff()
	function getEnabled() const
	function GetfFuel()
	function GetfFuelConsumption()
	function GetfFuelTank()
	function GetfHealth() const
	function GetRPM()
	function getVisible() const
	function HandBreak()
	function HasWeapon()
	function IsActiveEngine()
	function IsObjectVisible(game_object*)
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function PlayDamageParticles()
	function ReleaseHandBreak()
	function SetEnterLocked(boolean)
	function SetExitLocked(boolean)
	function SetExplodeTime(number)
	function SetfFuel(number)
	function SetfFuelConsumption(number)
	function SetfFuelTank(number)
	function SetfHealth(number)
	function SetParam(number, vector)
	function SetRPM(number)
	function StartEngine()
	function StopDamageParticles()
	function StopEngine()
	function use(CGameObject*)
	function Visual() const

class CCartridge
	const cfCanBeUnlimited = 4
	const cfExplosive = 8
	const cfMagneticBeam = 16
	const cfRicochet = 2
	const cfTracer = 1

	CCartridge()

	property bullet_material_idx
	property m_4to1_tracer
	property m_flags
	property m_LocalAmmoType
	property param_s

	function GetInventoryName()
	function Weight() const

class CCat (CGameObject)
	CCat()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CChimera (CGameObject)
	CChimera()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CConsole
	function execute(string)
	function execute_deferred(CConsole*, string)
	function execute_script(string)
	function get_bool(CConsole*, string)
	function get_float(CConsole*, string)
	function get_integer(CConsole*, string)
	function get_string(string) const
	function get_token(string) const
	function hide()
	function show()

class CController (CGameObject)
	CController()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CCustomOutfit (CGameObject)
	CCustomOutfit()

	property bIsHelmetAvaliable
	property m_additional_weight
	property m_additional_weight2
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerLoss
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function BonePassBullet(number)
	function get_artefact_count() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CCustomZone (CGameObject)
	CCustomZone()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CDestroyablePhysicsObject (CGameObject)
	CDestroyablePhysicsObject()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CDialogHolder
	CDialogHolder()

	function AddDialogToRender(CUIWindow*)
	function RemoveDialogToRender(CUIWindow*)
	function SetMainInputReceiver(CUIDialogWnd*, boolean)
	function TopInputReceiver()

class CDummyArtefact (CArtefact)
	CDummyArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class ce_script_zone (DLL_Pure)
	ce_script_zone()

	function _construct()

class ce_smart_zone (DLL_Pure)
	ce_smart_zone()

	function _construct()

class CEatableItem (CInventoryItem)
	property m_bRemoveAfterUse
	property m_fWeightEmpty
	property m_fWeightFull

	function ActivateItem()
	function CanDelete() const
	function CanStack() const
	function CanTake() const
	function Cost() const
	function DeactivateItem()
	function Empty() const
	function GetCondition() const
	function GetDropManual() const
	function GetIconName() const
	function GetMaxUses()
	function GetRemainingUses() const
	function HandDependence() const
	function has_any_upgrades()
	function install_upgrade(string)
	function IsQuestItem() const
	function IsUsingCondition() const
	function ItemDescription()
	function NameItem()
	function NameShort()
	function pre_install_upgrade()
	function SetCondition(number)
	function SetDropManual(number)
	function SetRemainingUses(number)
	function SetWeight(number)
	function Useful() const
	function verify_upgrade(string)
	function Weight() const

class cef_storage
	function evaluate(cef_storage*, string, cse_alife_object*)
	function evaluate(cef_storage*, string, game_object*)
	function evaluate(cef_storage*, string, cse_alife_object*, cse_alife_object*)
	function evaluate(cef_storage*, string, game_object*, game_object*)
	function evaluate(cef_storage*, string, cse_alife_object*, cse_alife_object*, cse_alife_object*)
	function evaluate(cef_storage*, string, game_object*, game_object*, game_object*)
	function evaluate(cef_storage*, string, cse_alife_object*, cse_alife_object*, cse_alife_object*, cse_alife_object*)
	function evaluate(cef_storage*, string, game_object*, game_object*, game_object*, game_object*)

class CElectricBall (CArtefact)
	CElectricBall()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CEliteDetector (CGameObject)
	CEliteDetector()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CEntityCondition
	const eBoostBleedingRestore = 3
	const eBoostBurnImmunity = 8
	const eBoostChemicalBurnImmunity = 12
	const eBoostChemicalBurnProtection = 7
	const eBoostExplImmunity = 13
	const eBoostFireWoundImmunity = 15
	const eBoostHpRestore = 0
	const eBoostMaxWeight = 4
	const eBoostPowerRestore = 1
	const eBoostRadiationImmunity = 10
	const eBoostRadiationProtection = 5
	const eBoostRadiationRestore = 2
	const eBoostShockImmunity = 9
	const eBoostStrikeImmunity = 14
	const eBoostTelepaticImmunity = 11
	const eBoostTelepaticProtection = 6
	const eBoostWoundImmunity = 16

	function BleedingSpeed()
	function ChangeAlcohol(number)
	function ChangeBleeding(number)
	function ChangeEntityMorale(number)
	function ChangeHealth(number)
	function ChangePower(number)
	function ChangePsyHealth(number)
	function ChangeRadiation(number)
	function ChangeSatiety(number)
	function GetEntityMorale() const
	function GetHealthLost() const
	function GetMaxPower() const
	function GetPower() const
	function GetPsyHealth() const
	function GetRadiation() const
	function GetSatiety() const
	function GetWhoHitLastTimeID()
	function IsLimping() const
	function SetMaxPower(number)

class CExplosiveItem (CGameObject)
	CExplosiveItem()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CF1 (CGameObject)
	CF1()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CFadedBall (CArtefact)
	CFadedBall()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CFoodItem (CGameObject)
	CFoodItem()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CFracture (CGameObject)
	CFracture()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CGalantineArtefact (CArtefact)
	CGalantineArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CGameFont
	const alCenter = 2
	const alLeft = 0
	const alRight = 1

class CGameGraph
	function accessible(const CGameGraph*, const number&)
	function accessible(const CGameGraph*, const number&, boolean)
	function distance(number, number) const
	function levels(const CGameGraph*)
	function valid_vertex_id(number) const
	function vertex(number) const
	function vertex_id(const GameGraph__CVertex*) const

class CGameObject (DLL_Pure, ISheduled, ICollidable, IRenderable)
	CGameObject()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CGameTask
	CGameTask()

	function add_complete_func(string)
	function add_complete_info(string)
	function add_fail_func(string)
	function add_fail_info(string)
	function add_on_complete_func(string)
	function add_on_complete_info(string)
	function add_on_fail_func(string)
	function add_on_fail_info(string)
	function change_map_location(string, number)
	function create_map_location(boolean)
	function get_icon_name()
	function get_id()
	function get_map_location()
	function get_map_object_id()
	function get_priority()
	function get_title()
	function remove_map_locations(boolean)
	function set_description(string)
	function set_icon_name(string)
	function set_id(string)
	function set_map_hint(string)
	function set_map_location(string)
	function set_map_object_id(number)
	function set_priority(number)
	function set_title(string)
	function set_type(number)

class CGraviArtefact (CArtefact)
	CGraviArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CGrenadeLauncher (CGameObject)
	CGrenadeLauncher()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CHairsZone (CGameObject)
	CHairsZone()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CHelicopter (CGameObject)
	const eAlive = 0
	const eBodyByPath = 0
	const eBodyToPoint = 1
	const eDead = 1
	const eEnemyEntity = 2
	const eEnemyNone = 0
	const eEnemyPoint = 1
	const eMovLanding = 4
	const eMovNone = 0
	const eMovPatrolPath = 2
	const eMovRoundPath = 3
	const eMovTakeOff = 5
	const eMovToPoint = 1

	CHelicopter()

	property m_dead
	property m_exploded
	property m_flame_started
	property m_light_started
	property m_max_mgun_dist
	property m_max_rocket_dist
	property m_min_mgun_dist
	property m_min_rocket_dist
	property m_syncronize_rocket
	property m_time_between_rocket_attack
	property m_use_mgun_on_attack
	property m_use_rocket_on_attack

	function _construct()
	function AlreadyOnPoint()
	function ClearEnemy()
	function Die()
	function Explode()
	function GetBodyState()
	function GetCameraType()
	function GetCurrVelocity()
	function GetCurrVelocityVec()
	function GetDestPosition()
	function GetDistanceToDestPosition()
	function getEnabled() const
	function GetEnemy()
	function GetfHealth() const
	function GetHuntState()
	function GetMaxVelocity()
	function GetMovementState()
	function GetOnPointRangeDist()
	function GetPathAltitude(vector&, number)
	function GetPosition()
	function GetRealAltitude()
	function GetSafeAltitude()
	function GetSpeedInDestPoint(number)
	function GetState()
	function getVisible() const
	function GoPatrolByPatrolPath(string, number)
	function GoPatrolByRoundPath(vector, number, boolean)
	function isVisible(game_object*)
	function LookAtPoint(vector, boolean)
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SetBarrelDirTolerance(number)
	function SetCameraType(number)
	function SetDestPosition(vector*)
	function SetEnemy(game_object*)
	function SetEnemy(vector*)
	function SetfHealth(number)
	function SetFireTrailLength(number)
	function SetLinearAcc(number, number)
	function SetMaxVelocity(number)
	function SetOnPointRangeDist(number)
	function SetSpeedInDestPoint(number)
	function StartFlame()
	function TurnEngineSound(boolean)
	function TurnLighting(boolean)
	function use(CGameObject*)
	function UseFireTrail()
	function UseFireTrail(boolean)
	function Visual() const

class CHelmet (CGameObject)
	CHelmet()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CHudItem
	function animation_slot()
	function HudAnimationExist(string)

class CInventory
	function Activate(number, boolean)
	function BeltWidth() const
	function CalcTotalWeight()
	function GetActiveSlot() const
	function GetMaxWeight() const
	function InBelt(const CInventoryItem*) const
	function InRuck(const CInventoryItem*) const
	function InSlot(const CInventoryItem*) const
	function ItemFromSlot(number) const
	function SetActiveSlot(number)
	function SetMaxWeight(number)
	function TotalWeight() const

class CInventoryBox (CGameObject)
	CInventoryBox()

	function _construct()
	function can_take() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function set_can_take(boolean)
	function set_closed(boolean, string)
	function use(CGameObject*)
	function Visual() const

class CInventoryItem
	function ActivateItem()
	function CanStack() const
	function CanTake() const
	function Cost() const
	function DeactivateItem()
	function GetCondition() const
	function GetDropManual() const
	function GetIconName() const
	function HandDependence() const
	function has_any_upgrades()
	function install_upgrade(string)
	function IsQuestItem() const
	function IsUsingCondition() const
	function ItemDescription()
	function NameItem()
	function NameShort()
	function pre_install_upgrade()
	function SetCondition(number)
	function SetDropManual(number)
	function SetWeight(number)
	function Useful() const
	function verify_upgrade(string)
	function Weight() const

class CInventoryOwner
	function buy_supplies(class CInifile&, string)
	function deadbody_can_take(boolean)
	function deadbody_can_take_status() const
	function deadbody_closed(boolean)
	function deadbody_closed_status() const
	function DisableInvUpgrade()
	function DisableTalk()
	function DisableTrade()
	function EnableInvUpgrade()
	function EnableTalk()
	function EnableTrade()
	function get_money() const
	function GetTalkPartner()
	function IconName() const
	function inventory()
	function IsInvUpgradeEnabled()
	function IsTalkEnabled()
	function IsTalking()
	function IsTradeEnabled()
	function OfferTalk(CInventoryOwner*)
	function sell_useless_items()
	function StartTalk(CInventoryOwner*, boolean)
	function StopTalk()
	function trade_section() const

class CLevelChanger (CGameObject)
	CLevelChanger()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class client_spawn_manager
	function add(number, number, const function<void>&)
	function add(number, number, const function<void>&, object)
	function remove(number, number)

class ClientID
	ClientID()

	operator ==(ClientID&, ClientID)

	function set(number)
	function value() const

class clsid
	const actor = 93
	const ameba_zone = 210
	const art_bast_artefact = 0
	const art_black_drops = 1
	const art_cta = 3
	const art_dummy = 4
	const art_electric_ball = 5
	const art_faded_ball = 6
	const art_galantine = 7
	const art_gravi = 8
	const art_gravi_black = 2
	const art_mercury_ball = 9
	const art_needles = 10
	const art_rusty_hair = 11
	const art_thorn = 12
	const art_zuda = 13
	const artefact = 42
	const artefact_s = 106
	const bloodsucker = 14
	const bloodsucker_s = 113
	const boar = 15
	const boar_s = 114
	const burer = 16
	const burer_s = 115
	const car = 53
	const car_s = 107
	const cat = 17
	const cat_s = 116
	const chimera = 29
	const chimera_s = 117
	const controller = 18
	const controller_s = 118
	const crow = 19
	const destrphys_s = 96
	const detector_advanced_s = 54
	const detector_elite_s = 55
	const detector_scientific_s = 56
	const detector_simple_s = 57
	const device_detector_advanced = 58
	const device_detector_elite = 59
	const device_detector_scientific = 62
	const device_detector_simple = 63
	const device_flare = 60
	const device_pda = 61
	const device_torch = 64
	const device_torch_s = 149
	const dog_black = 20
	const dog_red = 23
	const dog_s = 121
	const equ_backpack = 66
	const equ_helmet_s = 68
	const equ_stalker = 65
	const equ_stalker_s = 69
	const flesh = 24
	const flesh_group = 25
	const flesh_s = 122
	const fracture = 26
	const fracture_s = 124
	const game = 74
	const game_cl_artefact_hunt = 46
	const game_cl_capture_the_artefact = 47
	const game_cl_deathmatch = 48
	const game_cl_single = 49
	const game_cl_team_deathmatch = 50
	const game_sv_artefact_hunt = 136
	const game_sv_capture_the_artefact = 137
	const game_sv_deathmatch = 138
	const game_sv_single = 139
	const game_sv_team_deathmatch = 140
	const game_ui_artefact_hunt = 150
	const game_ui_capture_the_artefact = 151
	const game_ui_deathmatch = 152
	const game_ui_single = 153
	const game_ui_team_deathmatch = 154
	const gigant_s = 123
	const graph_point = 28
	const hanging_lamp = 97
	const helicopter = 51
	const helmet = 67
	const hlamp_s = 132
	const hud_manager = 78
	const inventory_box = 99
	const inventory_box_s = 144
	const karlik_s = 125
	const level = 73
	const level_changer = 88
	const level_changer_s = 89
	const main_menu = 90
	const mp_players_bag = 91
	const nogravity_zone = 217
	const obj_antirad = 79
	const obj_attachable = 80
	const obj_bandage = 81
	const obj_bolt = 82
	const obj_bottle = 83
	const obj_breakable = 94
	const obj_climable = 95
	const obj_document = 84
	const obj_explosive = 85
	const obj_explosive_s = 142
	const obj_food = 86
	const obj_food_s = 143
	const obj_holder_ent = 98
	const obj_medkit = 87
	const obj_pda_s = 147
	const obj_phskeleton = 104
	const obj_phys_destroyable = 103
	const obj_physic = 100
	const online_offline_group = 92
	const phantom = 30
	const poltergeist = 31
	const poltergeist_s = 126
	const projector = 102
	const pseudo_gigant = 27
	const pseudodog_s = 127
	const psy_dog = 22
	const psy_dog_phantom = 21
	const psy_dog_phantom_s = 119
	const psy_dog_s = 120
	const rat = 32
	const rat_s = 128
	const script_actor = 141
	const script_heli = 52
	const script_object = 108
	const script_phys = 101
	const script_restr = 134
	const script_stalker = 35
	const script_trader = 37
	const script_zone = 105
	const smart_cover = 109
	const smart_terrain = 110
	const smart_zone = 111
	const smartcover_s = 112
	const snork = 33
	const snork_s = 129
	const space_restrictor = 133
	const spectator = 135
	const stalker = 34
	const team_base_zone = 220
	const torrid_zone = 221
	const trader = 36
	const tushkano = 38
	const tushkano_s = 130
	const wpn_ak74 = 176
	const wpn_ak74_s = 155
	const wpn_ammo = 40
	const wpn_ammo_m209 = 43
	const wpn_ammo_m209_s = 145
	const wpn_ammo_og7b = 44
	const wpn_ammo_og7b_s = 146
	const wpn_ammo_s = 41
	const wpn_ammo_vog25 = 45
	const wpn_ammo_vog25_s = 148
	const wpn_auto_shotgun_s = 156
	const wpn_binocular = 177
	const wpn_binocular_s = 157
	const wpn_bm16 = 178
	const wpn_bm16_s = 158
	const wpn_fn2000 = 179
	const wpn_fort = 180
	const wpn_grenade_f1 = 70
	const wpn_grenade_f1_s = 71
	const wpn_grenade_fake = 72
	const wpn_grenade_launcher = 181
	const wpn_grenade_launcher_s = 159
	const wpn_grenade_rgd5 = 75
	const wpn_grenade_rgd5_s = 76
	const wpn_grenade_rpg7 = 77
	const wpn_groza = 182
	const wpn_groza_s = 160
	const wpn_hpsa = 183
	const wpn_hpsa_s = 161
	const wpn_knife = 184
	const wpn_knife_s = 162
	const wpn_lr300 = 185
	const wpn_lr300_s = 163
	const wpn_pm = 186
	const wpn_pm_s = 164
	const wpn_rg6 = 187
	const wpn_rg6_s = 165
	const wpn_rpg7 = 188
	const wpn_rpg7_s = 166
	const wpn_scope = 189
	const wpn_scope_s = 167
	const wpn_shotgun = 190
	const wpn_shotgun_s = 168
	const wpn_silencer = 191
	const wpn_silencer_s = 169
	const wpn_stat_mgun = 192
	const wpn_svd = 193
	const wpn_svd_s = 170
	const wpn_svu = 194
	const wpn_svu_s = 171
	const wpn_usp45 = 195
	const wpn_usp45_s = 172
	const wpn_val = 196
	const wpn_val_s = 173
	const wpn_vintorez = 197
	const wpn_vintorez_s = 174
	const wpn_walther = 198
	const wpn_walther_s = 175
	const wpn_wmagaz = 199
	const wpn_wmaggl = 200
	const zombie = 39
	const zombie_s = 131
	const zone = 222
	const zone_acid_fog = 209
	const zone_ameba_s = 201
	const zone_bfuzz = 211
	const zone_bfuzz_s = 202
	const zone_campfire = 212
	const zone_dead = 213
	const zone_galant_s = 203
	const zone_galantine = 214
	const zone_mbald_s = 204
	const zone_mincer = 216
	const zone_mincer_s = 205
	const zone_mosquito_bald = 215
	const zone_nograv_s = 206
	const zone_radio_s = 207
	const zone_radioactive = 218
	const zone_rusty_hair = 219
	const zone_torrid_s = 208

class CMainMenu
	function CancelDownload()
	function GetAccountMngr()
	function GetCDKey()
	function GetDemoInfo(string)
	function GetGSVer()
	function GetLoginMngr()
	function GetPatchProgress()
	function GetPlayerName()
	function GetProfileStore()
	function ValidateCDKey()

class CMapLocation
	function Collidable() const
	function DisablePointer()
	function DisableSpot()
	function EnablePointer()
	function EnableSpot()
	function GetHint()
	function GetLastPosition()
	function GetLevelName()
	function GetOwnerTaskID()
	function GetPosition()
	function GetType() const
	function HighlightSpot(boolean, const fcolor&)
	function HintEnabled()
	function IsUserDefined() const
	function ObjectID()
	function PointerEnabled()
	function SetHint(const class shared_str&)
	function SetUserDefinedFlag(number)
	function SpotEnabled()
	function SpotSize()

class CMapManager
	CMapManager()

	function AddMapLocation(string, number)
	function AddUserLocation(string, string, vector)
	function AllLocationsForEach(const function<boolean>&)
	function DisableAllPointers()
	function GetMapLocation(string, number)
	function HasMapLocation(string, number)
	function MapLocationsForEach(string, number, const function<boolean>&)
	function RemoveMapLocation(CMapLocation*)
	function RemoveMapLocation(string, number)
	function RemoveMapLocationByObjectID(number)

class CMedkit (CGameObject)
	CMedkit()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CMercuryBall (CArtefact)
	CMercuryBall()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CMincer (CGameObject)
	CMincer()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CMosquitoBald (CGameObject)
	CMosquitoBald()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CNoGravityZone (CGameObject)
	CNoGravityZone()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class color
	color()
	color(number, number, number)

	property b
	property g
	property r

	function set(number, number, number)

class color_animator
	color_animator(string)

	function calculate(number)
	function length()
	function load(string)

class cond
	const act_end = 128
	const anim_end = 4
	const look_end = 2
	const move_end = 1
	const object_end = 32
	const sound_end = 8
	const time_end = 64

	cond()
	cond(number)
	cond(number, double)

class connect_error_cb
	connect_error_cb()
	connect_error_cb(object, function<void>)

	function bind(object, function<void>)
	function clear()

class COptionsManager
	COptionsManager()

	function NeedSystemRestart()
	function NeedVidRestart()
	function OptionsPostAccept()
	function SaveBackupValues(string)
	function SaveValues(string)
	function SendMessage2Group(string, string)
	function SetCurrentValues(string)
	function UndoGroup(string)

class cover_point
	function is_smart_cover(const cover_point*)
	function level_vertex_id() const
	function position() const

class CPda (CGameObject)
	CPda()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CPhrase
	function GetPhraseScript()

class CPhraseDialog
	function AddPhrase(string, string, string, number)

class CPhraseScript
	function AddAction(string)
	function AddDisableInfo(string)
	function AddDontHasInfo(string)
	function AddGiveInfo(string)
	function AddHasInfo(string)
	function AddPrecondition(string)
	function SetScriptText(string)

class CPhysicObject (CGameObject)
	CPhysicObject()

	function _construct()
	function anim_time_get()
	function anim_time_set(number)
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function play_bones_sound()
	function run_anim_back()
	function run_anim_forward()
	function set_door_ignore_dynamics()
	function stop_anim()
	function stop_bones_sound()
	function unset_door_ignore_dynamics()
	function use(CGameObject*)
	function Visual() const

class CPhysicsShellHolder (CGameObject)
	CPhysicsShellHolder()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CPoltergeist (CGameObject)
	CPoltergeist()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CPseudoGigant (CGameObject)
	CPseudoGigant()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CPsyDog (CGameObject)
	CPsyDog()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CPsyDogPhantom (CGameObject)
	CPsyDogPhantom()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class cpure_server_object (ipure_server_object)
class CRadioactiveZone (CGameObject)
	CRadioactiveZone()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CRGD5 (CGameObject)
	CRGD5()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CRustyHairArtefact (CArtefact)
	CRustyHairArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CSavedGameWrapper
	CSavedGameWrapper(string)

	function actor_health() const
	function game_time(const CSavedGameWrapper*)
	function level_id() const
	function level_name() const

class CScientificDetector (CGameObject)
	CScientificDetector()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CScope (CGameObject)
	CScope()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CScriptXmlInit
	CScriptXmlInit()

	function Init3tButton(string, CUIWindow*)
	function InitAnimStatic(string, CUIWindow*)
	function InitCDkey(string, CUIWindow*)
	function InitCheck(string, CUIWindow*)
	function InitComboBox(string, CUIWindow*)
	function InitEditBox(string, CUIWindow*)
	function InitFrame(string, CUIWindow*)
	function InitFrameLine(string, CUIWindow*)
	function InitHint(string, CUIWindow*)
	function InitKeyBinding(string, CUIWindow*)
	function InitListBox(string, CUIWindow*)
	function InitMapInfo(string, CUIWindow*)
	function InitMapList(string, CUIWindow*)
	function InitMMShniaga(string, CUIWindow*)
	function InitMPPlayerName(string, CUIWindow*)
	function InitProgressBar(string, CUIWindow*)
	function InitScrollView(string, CUIWindow*)
	function InitServerList(string, CUIWindow*)
	function InitSleepStatic(string, CUIWindow*)
	function InitSpinFlt(string, CUIWindow*)
	function InitSpinNum(string, CUIWindow*)
	function InitSpinText(string, CUIWindow*)
	function InitStatic(string, CUIWindow*)
	function InitTab(string, CUIWindow*)
	function InitTextWnd(string, CUIWindow*)
	function InitTrackBar(string, CUIWindow*)
	function InitWindow(string, number, CUIWindow*)
	function NodeExists(string, number)
	function ParseFile(string)

class cse_abstract (cpure_server_object)
	property angle
	property id
	property parent_id
	property position
	property script_version

	function clsid() const
	function name(const cse_abstract*)
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)

class CSE_AbstractVisual (cse_visual, cse_abstract)
	CSE_AbstractVisual(string)

	property angle
	property id
	property parent_id
	property position
	property script_version

	function clsid() const
	function getStartupAnimation()
	function init()
	function name(const cse_abstract*)
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)

class cse_alife_car (cse_alife_dynamic_object_visual, cse_ph_skeleton)
	cse_alife_car(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_creature_abstract (cse_alife_dynamic_object_visual)
	cse_alife_creature_abstract(string)

	property angle
	property group
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function g_group()
	function g_squad()
	function g_team()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_creature_actor (cse_alife_creature_abstract, cse_alife_trader_abstract, cse_ph_skeleton)
	cse_alife_creature_actor(string)

	property angle
	property group
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function character_icon(cse_alife_trader_abstract*)
	function character_name(cse_alife_trader_abstract*)
	function clsid() const
	function community() const
	function g_group()
	function g_squad()
	function g_team()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function profile_name(cse_alife_trader_abstract*)
	function rank()
	function reputation()
	function section_name(const cse_abstract*)
	function set_character_name(cse_alife_trader_abstract*, string)
	function set_profile_name(cse_alife_trader_abstract*, string)
	function set_rank(number)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_creature_crow (cse_alife_creature_abstract)
	cse_alife_creature_crow(string)

	property angle
	property group
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function g_group()
	function g_squad()
	function g_team()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_creature_phantom (cse_alife_creature_abstract)
	cse_alife_creature_phantom(string)

	property angle
	property group
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function g_group()
	function g_squad()
	function g_team()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_dynamic_object (cse_alife_object)
	cse_alife_dynamic_object(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_dynamic_object_visual (cse_alife_dynamic_object, cse_visual)
	cse_alife_dynamic_object_visual(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_graph_point (cse_abstract)
	cse_alife_graph_point(string)

	property angle
	property id
	property parent_id
	property position
	property script_version

	function clsid() const
	function init()
	function name(const cse_abstract*)
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)

class cse_alife_group_abstract
class cse_alife_helicopter (cse_alife_dynamic_object_visual, cse_motion, cse_ph_skeleton)
	cse_alife_helicopter(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_human_abstract (cse_alife_trader_abstract, cse_alife_monster_abstract)
	cse_alife_human_abstract(string)

	property angle
	property group
	property group_id
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_smart_terrain_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function brain(cse_alife_human_abstract*)
	function brain(cse_alife_monster_abstract*)
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function character_icon(cse_alife_trader_abstract*)
	function character_name(cse_alife_trader_abstract*)
	function clear_smart_terrain(cse_alife_monster_abstract*)
	function clsid() const
	function community() const
	function current_level_travel_speed(cse_alife_monster_abstract*)
	function current_level_travel_speed(cse_alife_monster_abstract*, number)
	function force_set_goodwill(cse_alife_monster_abstract*, number, number)
	function g_group()
	function g_squad()
	function g_team()
	function has_detector()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function kill()
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function profile_name(cse_alife_trader_abstract*)
	function rank()
	function reputation()
	function section_name(const cse_abstract*)
	function set_character_name(cse_alife_trader_abstract*, string)
	function set_profile_name(cse_alife_trader_abstract*, string)
	function set_rank(number)
	function smart_terrain_id(cse_alife_monster_abstract*)
	function smart_terrain_task_activate(cse_alife_monster_abstract*)
	function smart_terrain_task_deactivate(cse_alife_monster_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function travel_speed(cse_alife_monster_abstract*)
	function travel_speed(cse_alife_monster_abstract*, number)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_human_stalker (cse_alife_human_abstract, cse_ph_skeleton)
	cse_alife_human_stalker(string)

	property angle
	property group
	property group_id
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_smart_terrain_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function brain(cse_alife_human_abstract*)
	function brain(cse_alife_monster_abstract*)
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function character_icon(cse_alife_trader_abstract*)
	function character_name(cse_alife_trader_abstract*)
	function clear_smart_terrain(cse_alife_monster_abstract*)
	function clsid() const
	function community() const
	function current_level_travel_speed(cse_alife_monster_abstract*)
	function current_level_travel_speed(cse_alife_monster_abstract*, number)
	function force_set_goodwill(cse_alife_monster_abstract*, number, number)
	function g_group()
	function g_squad()
	function g_team()
	function has_detector()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function kill()
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function profile_name(cse_alife_trader_abstract*)
	function rank()
	function reputation()
	function section_name(const cse_abstract*)
	function set_character_name(cse_alife_trader_abstract*, string)
	function set_profile_name(cse_alife_trader_abstract*, string)
	function set_rank(number)
	function smart_terrain_id(cse_alife_monster_abstract*)
	function smart_terrain_task_activate(cse_alife_monster_abstract*)
	function smart_terrain_task_deactivate(cse_alife_monster_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function travel_speed(cse_alife_monster_abstract*)
	function travel_speed(cse_alife_monster_abstract*, number)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_inventory_box (cse_alife_dynamic_object_visual)
	cse_alife_inventory_box(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_inventory_item
	property condition

	function add_upgrade(cse_alife_inventory_item*, string)
	function has_upgrade(cse_alife_inventory_item*, string)

class cse_alife_item (cse_alife_dynamic_object_visual, cse_alife_inventory_item)
	cse_alife_item(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_ammo (cse_alife_item)
	cse_alife_item_ammo(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_artefact (cse_alife_item)
	cse_alife_item_artefact(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_bolt (cse_alife_item)
	cse_alife_item_bolt(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_custom_outfit (cse_alife_item)
	cse_alife_item_custom_outfit(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_detector (cse_alife_item)
	cse_alife_item_detector(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_document (cse_alife_item)
	cse_alife_item_document(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_explosive (cse_alife_item)
	cse_alife_item_explosive(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_grenade (cse_alife_item)
	cse_alife_item_grenade(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_helmet (cse_alife_item)
	cse_alife_item_helmet(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_pda (cse_alife_item)
	cse_alife_item_pda(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_torch (cse_alife_item)
	cse_alife_item_torch(string)

	property angle
	property condition
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_weapon (cse_alife_item)
	const eAddonAttachable = 2
	const eAddonDisabled = 0
	const eAddonPermanent = 1
	const eWeaponAddonGrenadeLauncher = 2
	const eWeaponAddonScope = 1
	const eWeaponAddonSilencer = 4

	cse_alife_item_weapon(string)

	property angle
	property condition
	property grenade_launcher_status
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property scope_status
	property script_version
	property silencer_status

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clone_addons(cse_alife_item_weapon*)
	function clsid() const
	function get_addon_flags()
	function get_ammo_elapsed()
	function get_ammo_elapsed2()
	function get_ammo_magsize()
	function get_ammo_type()
	function get_ammo_type2()
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function set_ammo_elapsed(number)
	function set_ammo_elapsed2(number)
	function set_ammo_type(number)
	function set_ammo_type2(number)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_weapon_auto_shotgun (cse_alife_item_weapon)
	const eAddonAttachable = 2
	const eAddonDisabled = 0
	const eAddonPermanent = 1
	const eWeaponAddonGrenadeLauncher = 2
	const eWeaponAddonScope = 1
	const eWeaponAddonSilencer = 4

	cse_alife_item_weapon_auto_shotgun(string)

	property angle
	property condition
	property grenade_launcher_status
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property scope_status
	property script_version
	property silencer_status

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clone_addons(cse_alife_item_weapon*)
	function clsid() const
	function get_addon_flags()
	function get_ammo_elapsed()
	function get_ammo_elapsed2()
	function get_ammo_magsize()
	function get_ammo_type()
	function get_ammo_type2()
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function set_ammo_elapsed(number)
	function set_ammo_elapsed2(number)
	function set_ammo_type(number)
	function set_ammo_type2(number)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_weapon_magazined (cse_alife_item_weapon)
	const eAddonAttachable = 2
	const eAddonDisabled = 0
	const eAddonPermanent = 1
	const eWeaponAddonGrenadeLauncher = 2
	const eWeaponAddonScope = 1
	const eWeaponAddonSilencer = 4

	cse_alife_item_weapon_magazined(string)

	property angle
	property condition
	property grenade_launcher_status
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property scope_status
	property script_version
	property silencer_status

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clone_addons(cse_alife_item_weapon*)
	function clsid() const
	function get_addon_flags()
	function get_ammo_elapsed()
	function get_ammo_elapsed2()
	function get_ammo_magsize()
	function get_ammo_type()
	function get_ammo_type2()
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function set_ammo_elapsed(number)
	function set_ammo_elapsed2(number)
	function set_ammo_type(number)
	function set_ammo_type2(number)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_weapon_magazined_w_gl (cse_alife_item_weapon_magazined)
	const eAddonAttachable = 2
	const eAddonDisabled = 0
	const eAddonPermanent = 1
	const eWeaponAddonGrenadeLauncher = 2
	const eWeaponAddonScope = 1
	const eWeaponAddonSilencer = 4

	cse_alife_item_weapon_magazined_w_gl(string)

	property angle
	property condition
	property grenade_launcher_status
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property scope_status
	property script_version
	property silencer_status

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clone_addons(cse_alife_item_weapon*)
	function clsid() const
	function get_addon_flags()
	function get_ammo_elapsed()
	function get_ammo_elapsed2()
	function get_ammo_magsize()
	function get_ammo_type()
	function get_ammo_type2()
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function set_ammo_elapsed(number)
	function set_ammo_elapsed2(number)
	function set_ammo_type(number)
	function set_ammo_type2(number)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_item_weapon_shotgun (cse_alife_item_weapon)
	const eAddonAttachable = 2
	const eAddonDisabled = 0
	const eAddonPermanent = 1
	const eWeaponAddonGrenadeLauncher = 2
	const eWeaponAddonScope = 1
	const eWeaponAddonSilencer = 4

	cse_alife_item_weapon_shotgun(string)

	property angle
	property condition
	property grenade_launcher_status
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property scope_status
	property script_version
	property silencer_status

	function add_upgrade(cse_alife_inventory_item*, string)
	function bfUseful()
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clone_addons(cse_alife_item_weapon*)
	function clsid() const
	function get_addon_flags()
	function get_ammo_elapsed()
	function get_ammo_elapsed2()
	function get_ammo_magsize()
	function get_ammo_type()
	function get_ammo_type2()
	function has_upgrade(cse_alife_inventory_item*, string)
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function set_ammo_elapsed(number)
	function set_ammo_elapsed2(number)
	function set_ammo_type(number)
	function set_ammo_type2(number)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_level_changer (cse_alife_space_restrictor)
	cse_alife_level_changer(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_monster_abstract (cse_alife_creature_abstract, cse_alife_schedulable)
	cse_alife_monster_abstract(string)

	property angle
	property group
	property group_id
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_smart_terrain_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function brain(cse_alife_monster_abstract*)
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clear_smart_terrain(cse_alife_monster_abstract*)
	function clsid() const
	function current_level_travel_speed(cse_alife_monster_abstract*)
	function current_level_travel_speed(cse_alife_monster_abstract*, number)
	function force_set_goodwill(cse_alife_monster_abstract*, number, number)
	function g_group()
	function g_squad()
	function g_team()
	function has_detector()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function kill()
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function rank()
	function section_name(const cse_abstract*)
	function smart_terrain_id(cse_alife_monster_abstract*)
	function smart_terrain_task_activate(cse_alife_monster_abstract*)
	function smart_terrain_task_deactivate(cse_alife_monster_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function travel_speed(cse_alife_monster_abstract*)
	function travel_speed(cse_alife_monster_abstract*, number)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_monster_base (cse_alife_monster_abstract, cse_ph_skeleton)
	cse_alife_monster_base(string)

	property angle
	property group
	property group_id
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_smart_terrain_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function brain(cse_alife_monster_abstract*)
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clear_smart_terrain(cse_alife_monster_abstract*)
	function clsid() const
	function current_level_travel_speed(cse_alife_monster_abstract*)
	function current_level_travel_speed(cse_alife_monster_abstract*, number)
	function force_set_goodwill(cse_alife_monster_abstract*, number, number)
	function g_group()
	function g_squad()
	function g_team()
	function has_detector()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function kill()
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function rank()
	function section_name(const cse_abstract*)
	function smart_terrain_id(cse_alife_monster_abstract*)
	function smart_terrain_task_activate(cse_alife_monster_abstract*)
	function smart_terrain_task_deactivate(cse_alife_monster_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function travel_speed(cse_alife_monster_abstract*)
	function travel_speed(cse_alife_monster_abstract*, number)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_monster_rat (cse_alife_monster_abstract, cse_alife_inventory_item)
	cse_alife_monster_rat(string)

	property angle
	property condition
	property group
	property group_id
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_smart_terrain_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function add_upgrade(cse_alife_inventory_item*, string)
	function alive() const
	function brain(cse_alife_monster_abstract*)
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clear_smart_terrain(cse_alife_monster_abstract*)
	function clsid() const
	function current_level_travel_speed(cse_alife_monster_abstract*)
	function current_level_travel_speed(cse_alife_monster_abstract*, number)
	function force_set_goodwill(cse_alife_monster_abstract*, number, number)
	function g_group()
	function g_squad()
	function g_team()
	function has_detector()
	function has_upgrade(cse_alife_inventory_item*, string)
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function kill()
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function rank()
	function section_name(const cse_abstract*)
	function smart_terrain_id(cse_alife_monster_abstract*)
	function smart_terrain_task_activate(cse_alife_monster_abstract*)
	function smart_terrain_task_deactivate(cse_alife_monster_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function travel_speed(cse_alife_monster_abstract*)
	function travel_speed(cse_alife_monster_abstract*, number)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_monster_zombie (cse_alife_monster_abstract)
	cse_alife_monster_zombie(string)

	property angle
	property group
	property group_id
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_smart_terrain_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function brain(cse_alife_monster_abstract*)
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clear_smart_terrain(cse_alife_monster_abstract*)
	function clsid() const
	function current_level_travel_speed(cse_alife_monster_abstract*)
	function current_level_travel_speed(cse_alife_monster_abstract*, number)
	function force_set_goodwill(cse_alife_monster_abstract*, number, number)
	function g_group()
	function g_squad()
	function g_team()
	function has_detector()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function kill()
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function rank()
	function section_name(const cse_abstract*)
	function smart_terrain_id(cse_alife_monster_abstract*)
	function smart_terrain_task_activate(cse_alife_monster_abstract*)
	function smart_terrain_task_deactivate(cse_alife_monster_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function travel_speed(cse_alife_monster_abstract*)
	function travel_speed(cse_alife_monster_abstract*, number)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_mounted_weapon (cse_alife_dynamic_object_visual)
	cse_alife_mounted_weapon(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_object (cse_abstract)
	cse_alife_object(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_object_breakable (cse_alife_dynamic_object_visual)
	cse_alife_object_breakable(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_object_climable (cse_shape, cse_abstract)
	cse_alife_object_climable(string)

	property angle
	property id
	property parent_id
	property position
	property script_version

	function clsid() const
	function init()
	function name(const cse_abstract*)
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)

class cse_alife_object_hanging_lamp (cse_alife_dynamic_object_visual, cse_ph_skeleton)
	cse_alife_object_hanging_lamp(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_object_physic (cse_alife_dynamic_object_visual, cse_ph_skeleton)
	cse_alife_object_physic(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function set_yaw(cse_alife_object_physic*, number)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_object_projector (cse_alife_dynamic_object_visual)
	cse_alife_object_projector(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_online_offline_group (cse_alife_dynamic_object, cse_alife_schedulable)
	cse_alife_online_offline_group(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function add_location_type(string)
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clear_location_types()
	function clsid() const
	function commander_id()
	function force_change_position(vector)
	function get_current_task()
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function npc_count() const
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function register_member(number)
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function squad_members() const
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function unregister_member(number)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_ph_skeleton_object (cse_alife_dynamic_object_visual, cse_ph_skeleton)
	cse_alife_ph_skeleton_object(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_psydog_phantom (cse_alife_monster_base)
	cse_alife_psydog_phantom(string)

	property angle
	property group
	property group_id
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_smart_terrain_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version
	property squad
	property team

	function alive() const
	function brain(cse_alife_monster_abstract*)
	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clear_smart_terrain(cse_alife_monster_abstract*)
	function clsid() const
	function current_level_travel_speed(cse_alife_monster_abstract*)
	function current_level_travel_speed(cse_alife_monster_abstract*, number)
	function force_set_goodwill(cse_alife_monster_abstract*, number, number)
	function g_group()
	function g_squad()
	function g_team()
	function has_detector()
	function health() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function kill()
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function o_torso(cse_alife_creature_abstract*)
	function on_before_register()
	function on_death(cse_abstract*)
	function on_register()
	function on_spawn()
	function on_unregister()
	function rank()
	function section_name(const cse_abstract*)
	function smart_terrain_id(cse_alife_monster_abstract*)
	function smart_terrain_task_activate(cse_alife_monster_abstract*)
	function smart_terrain_task_deactivate(cse_alife_monster_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function travel_speed(cse_alife_monster_abstract*)
	function travel_speed(cse_alife_monster_abstract*, number)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_schedulable (ipure_schedulable_object)
class cse_alife_smart_zone (cse_alife_space_restrictor, cse_alife_schedulable)
	cse_alife_smart_zone(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function detect_probability()
	function enabled(cse_alife_monster_abstract*) const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function register_npc(cse_alife_monster_abstract*)
	function section_name(const cse_abstract*)
	function smart_touch(cse_alife_monster_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function suitable(cse_alife_monster_abstract*) const
	function switch_offline()
	function switch_online()
	function task(cse_alife_monster_abstract*)
	function unregister_npc(cse_alife_monster_abstract*)
	function update()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_space_restrictor (cse_alife_dynamic_object, cse_shape)
	cse_alife_space_restrictor(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_team_base_zone (cse_alife_space_restrictor)
	cse_alife_team_base_zone(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_trader (cse_alife_dynamic_object_visual, cse_alife_trader_abstract)
	cse_alife_trader(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function character_icon(cse_alife_trader_abstract*)
	function character_name(cse_alife_trader_abstract*)
	function clsid() const
	function community() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function profile_name(cse_alife_trader_abstract*)
	function rank()
	function reputation()
	function section_name(const cse_abstract*)
	function set_character_name(cse_alife_trader_abstract*, string)
	function set_profile_name(cse_alife_trader_abstract*, string)
	function set_rank(number)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_alife_trader_abstract
	function character_icon(cse_alife_trader_abstract*)
	function character_name(cse_alife_trader_abstract*)
	function community() const
	function profile_name(cse_alife_trader_abstract*)
	function rank()
	function reputation()
	function set_character_name(cse_alife_trader_abstract*, string)
	function set_profile_name(cse_alife_trader_abstract*, string)
	function set_rank(number)

class cse_anomalous_zone (cse_custom_zone)
	cse_anomalous_zone(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_custom_zone (cse_alife_dynamic_object, cse_shape)
	cse_custom_zone(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_motion
class cse_ph_skeleton
class cse_shape
class cse_smart_cover (cse_alife_dynamic_object)
	cse_smart_cover(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function description() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function set_available_loopholes(object)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_spectator (cse_abstract)
	cse_spectator(string)

	property angle
	property id
	property parent_id
	property position
	property script_version

	function clsid() const
	function init()
	function name(const cse_abstract*)
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)

class cse_temporary (cse_abstract)
	cse_temporary(string)

	property angle
	property id
	property parent_id
	property position
	property script_version

	function clsid() const
	function init()
	function name(const cse_abstract*)
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)

class cse_torrid_zone (cse_custom_zone, cse_motion)
	cse_torrid_zone(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class cse_visual
class cse_zone_visual (cse_anomalous_zone, cse_visual)
	cse_zone_visual(string)

	property angle
	property id
	property m_game_vertex_id
	property m_level_vertex_id
	property m_story_id
	property online
	property parent_id
	property position
	property script_version

	function can_save() const
	function can_switch_offline() const
	function can_switch_offline(boolean)
	function can_switch_online() const
	function can_switch_online(boolean)
	function clsid() const
	function init()
	function interactive() const
	function keep_saved_data_anyway() const
	function move_offline() const
	function move_offline(boolean)
	function name(const cse_abstract*)
	function on_before_register()
	function on_register()
	function on_spawn()
	function on_unregister()
	function section_name(const cse_abstract*)
	function spawn_ini(cse_abstract*)
	function STATE_Read(net_packet&, number)
	function STATE_Write(net_packet&)
	function switch_offline()
	function switch_online()
	function UPDATE_Read(net_packet&)
	function UPDATE_Write(net_packet&)
	function use_ai_locations(boolean)
	function used_ai_locations() const
	function visible_for_map() const
	function visible_for_map(boolean)

class CServerList (CUIWindow)
	const ece_unique_nick_expired = 2
	const ece_unique_nick_not_registred = 1

	CServerList()

	function AttachChild(CUIWindow*)
	function ConnectToSelected()
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function NetRadioChanged(boolean)
	function RefreshList(boolean)
	function RefreshQuick()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetConnectionErrCb(connect_error_cb)
	function SetFilters(SServerFilters&)
	function SetPlayerName(string)
	function SetPPMode()
	function SetSortFunc(string, boolean)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function ShowServerInfo()
	function WindowName()

class CSightParams
	const eSightTypeAnimationDirection = 11
	const eSightTypeCover = 5
	const eSightTypeCoverLookOver = 8
	const eSightTypeCurrentDirection = 0
	const eSightTypeDirection = 2
	const eSightTypeDummy = -1
	const eSightTypeFireObject = 9
	const eSightTypeFirePosition = 10
	const eSightTypeLookOver = 7
	const eSightTypeObject = 4
	const eSightTypePathDirection = 1
	const eSightTypePosition = 3
	const eSightTypeSearch = 6

	CSightParams()

	property m_object
	property m_sight_type
	property m_vector

class CSilencer (CGameObject)
	CSilencer()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CSimpleDetector (CGameObject)
	CSimpleDetector()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CSnork (CGameObject)
	CSnork()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CSpaceRestrictor (CGameObject)
	CSpaceRestrictor()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function Radius() const
	function use(CGameObject*)
	function Visual() const

class CThornArtefact (CArtefact)
	CThornArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class CTime
	const DateToDay = 0
	const DateToMonth = 1
	const DateToYear = 2
	const TimeToHours = 0
	const TimeToMilisecs = 3
	const TimeToMinutes = 1
	const TimeToSeconds = 2

	CTime()
	CTime(const CTime&)

	operator -(CTime&, CTime)
	operator +(CTime&, CTime)
	operator <(const CTime&, CTime)
	operator <=(const CTime&, CTime)
	operator ==(const CTime&, CTime)
	operator >(const CTime&, CTime)
	operator >=(const CTime&, CTime)

	function add(CTime*)
	function dateToString(number)
	function diffSec(CTime*)
	function get(number&, number&, number&, number&, number&, number&, number&)
	function set(number, number, number, number, number, number, number)
	function setHMS(number, number, number)
	function setHMSms(number, number, number, number)
	function sub(CTime*)
	function timeToString(number)

class CTorch (CGameObject)
	CTorch()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CTorridZone (CGameObject)
	CTorridZone()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CTushkano (CGameObject)
	CTushkano()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CUI3tButton (CUIButton)
	CUI3tButton()

	function AdjustHeightToText()
	function AdjustWidthToText()
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetStretchTexture()
	function GetTextureColor() const
	function GetTextureRect()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function InitTexture(string)
	function InitTextureEx(string, string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetStretchTexture(boolean)
	function SetTextureColor(number)
	function SetTextureRect(Frect*)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function TextControl()
	function WindowName()

class CUIActorMenu (CUIDialogWnd)
	CUIActorMenu()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function get_drag_item()
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetHolder()
	function GetInvBox(CUIActorMenu*)
	function GetMenuMode()
	function GetPartner(CUIActorMenu*)
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function HideDialog()
	function highlight_for_each_in_slot(const function<boolean>&, number, number)
	function highlight_section_in_slot(string, number, number)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function refresh_current_cell_item()
	function ResetPPMode()
	function SetActor(CUIActorMenu*, game_object*)
	function SetAutoDelete(boolean)
	function SetInvBox(CUIActorMenu*, game_object*)
	function SetMenuMode(enum EMenuMode)
	function SetPartner(CUIActorMenu*, game_object*)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function ShowDialog(boolean)
	function ToBelt(game_object*, boolean)
	function ToSlot(game_object*, boolean, number)
	function WindowName()

class CUIButton (CUIStatic)
	CUIButton()

	function AdjustHeightToText()
	function AdjustWidthToText()
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetStretchTexture()
	function GetTextureColor() const
	function GetTextureRect()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function InitTexture(string)
	function InitTextureEx(string, string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetStretchTexture(boolean)
	function SetTextureColor(number)
	function SetTextureRect(Frect*)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function TextControl()
	function WindowName()

class CUICheckButton (CUI3tButton)
	CUICheckButton()

	function AdjustHeightToText()
	function AdjustWidthToText()
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetCheck() const
	function GetHeight() const
	function GetStretchTexture()
	function GetTextureColor() const
	function GetTextureRect()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function InitTexture(string)
	function InitTextureEx(string, string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetCheck(boolean)
	function SetDependControl(CUIWindow*)
	function SetPPMode()
	function SetStretchTexture(boolean)
	function SetTextureColor(number)
	function SetTextureRect(Frect*)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function TextControl()
	function WindowName()

class CUIComboBox (CUIWindow)
	CUIComboBox()

	function AddItem(string, number)
	function AttachChild(CUIWindow*)
	function ClearList()
	function CurrentID()
	function DetachChild(CUIWindow*)
	function disable_id(number)
	function Enable(boolean)
	function enable_id(number)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetCurrentIdx()
	function GetHeight() const
	function GetText()
	function GetTextOf(number)
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetCurrentIdx(number)
	function SetCurrentOptValue()
	function SetListLength(number)
	function SetPPMode()
	function SetText(string)
	function SetVertScroll(boolean)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUICustomEdit (CUIWindow)
	function AttachChild(CUIWindow*)
	function CaptureFocus(boolean)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetText() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetNextFocusCapturer(CUICustomEdit*)
	function SetPPMode()
	function SetText(string)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUICustomSpin (CUIWindow)
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetText()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIDialogWnd (CUIWindow)
	CUIDialogWnd()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetHolder()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function HideDialog()
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function ShowDialog(boolean)
	function WindowName()

class CUIEditBox (CUICustomEdit)
	CUIEditBox()

	function AttachChild(CUIWindow*)
	function CaptureFocus(boolean)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetText() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function InitTexture(string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetNextFocusCapturer(CUICustomEdit*)
	function SetPPMode()
	function SetText(string)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIFrameLineWnd (CUIWindow)
	CUIFrameLineWnd()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetColor(number)
	function SetHeight(number)
	function SetPPMode()
	function SetWidth(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIFrameWindow (CUIWindow)
	CUIFrameWindow()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetColor(number)
	function SetHeight(number)
	function SetPPMode()
	function SetWidth(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIGameCustom
	function AddCustomStatic(string, boolean)
	function AddDialogToRender(CUIWindow*)
	function CurrentItemAtCell()
	function enable_fake_indicators(boolean)
	function GetCustomStatic(string)
	function hide_messages()
	function HideActorMenu()
	function HidePdaMenu()
	function RemoveCustomStatic(string)
	function RemoveDialogToRender(CUIWindow*)
	function SetMainInputReceiver(CUIDialogWnd*, boolean)
	function show_messages()
	function ShowActorMenu()
	function TopInputReceiver()
	function update_fake_indicators(number, number)
	function UpdateActorMenu()

class CUIHudStatesWnd (CUIWindow)
	CUIHudStatesWnd()

	property m_back
	property m_fire_mode
	property m_radia_damage
	property m_ui_grenade
	property m_ui_health_bar
	property m_ui_stamina_bar
	property m_ui_weapon_ammo_color_active
	property m_ui_weapon_ammo_color_inactive
	property m_ui_weapon_ap_ammo
	property m_ui_weapon_cur_ammo
	property m_ui_weapon_fmj_ammo
	property m_ui_weapon_icon
	property m_ui_weapon_third_ammo

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUILines
	function GetText()
	function SetElipsis(boolean)
	function SetFont(CGameFont*)
	function SetText(string)
	function SetTextColor(number)
	function SetTextST(string)

class CUIListBox (CUIScrollView)
	CUIListBox()

	function AddExistingItem(CUIListBoxItem*)
	function AddTextItem(string)
	function AddWindow(CUIWindow*, boolean)
	function AttachChild(CUIWindow*)
	function Clear()
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetCurrentScrollPos()
	function GetHeight() const
	function GetItem(number)
	function GetItemByIndex(number)
	function GetItemHeight()
	function GetMaxScrollPos()
	function GetMinScrollPos()
	function GetSelectedIndex()
	function GetSelectedItem()
	function GetSize()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function RemoveAll()
	function RemoveItem(CUIWindow*)
	function RemoveWindow(CUIWindow*)
	function ResetPPMode()
	function ScrollToBegin()
	function ScrollToEnd()
	function SetAutoDelete(boolean)
	function SetFixedScrollBar(boolean)
	function SetItemHeight(number)
	function SetPPMode()
	function SetScrollPos(number)
	function SetSelectedIndex(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function ShowSelectedItem(boolean)
	function WindowName()

class CUIListBoxItem (CUIFrameLineWnd)
	CUIListBoxItem(number)

	function AddIconField(number)
	function AddTextField(string, number)
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetTextItem()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetColor(number)
	function SetHeight(number)
	function SetPPMode()
	function SetTextColor(number)
	function SetWidth(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIListBoxItemMsgChain (CUIListBoxItem)
	CUIListBoxItemMsgChain(number)

	function AddIconField(number)
	function AddTextField(string, number)
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetTextItem()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetColor(number)
	function SetHeight(number)
	function SetPPMode()
	function SetTextColor(number)
	function SetWidth(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIMainIngameWnd (CUIWindow)
	CUIMainIngameWnd()

	property m_ind_bleeding
	property m_ind_boost_chem
	property m_ind_boost_health
	property m_ind_boost_power
	property m_ind_boost_psy
	property m_ind_boost_rad
	property m_ind_boost_radia
	property m_ind_boost_weight
	property m_ind_boost_wound
	property m_ind_helmet_broken
	property m_ind_outfit_broken
	property m_ind_overweight
	property m_ind_radiation
	property m_ind_starvation
	property m_ind_weapon_broken
	property m_ui_hud_states
	property UIMotionIcon
	property UIStaticDiskIO
	property UIStaticQuickHelp
	property UIZoneMap

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIMapInfo (CUIWindow)
	CUIMapInfo()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function Init(vector2, vector2)
	function InitMap(string, string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIMapList (CUIWindow)
	CUIMapList()

	function AttachChild(CUIWindow*)
	function ClearList()
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetCommandLine(string)
	function GetCurGameType()
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEmpty()
	function IsEnabled()
	function IsShown()
	function LoadMapList()
	function OnModeChange()
	function ResetPPMode()
	function SaveMapList()
	function SetAutoDelete(boolean)
	function SetMapInfo(CUIMapInfo*)
	function SetMapPic(CUIStatic*)
	function SetModeSelector(CUIWindow*)
	function SetPPMode()
	function SetServerParams(string)
	function SetWeatherSelector(CUIComboBox*)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function StartDedicatedServer()
	function WindowName()

class CUIMessageBox (CUIStatic)
	CUIMessageBox()

	function AdjustHeightToText()
	function AdjustWidthToText()
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetHost()
	function GetPassword()
	function GetStretchTexture()
	function GetTextureColor() const
	function GetTextureRect()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function InitMessageBox(string)
	function InitTexture(string)
	function InitTextureEx(string, string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetStretchTexture(boolean)
	function SetText(string)
	function SetTextureColor(number)
	function SetTextureRect(Frect*)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function TextControl()
	function WindowName()

class CUIMessageBoxEx (CUIDialogWnd)
	CUIMessageBoxEx()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetHolder()
	function GetHost()
	function GetPassword()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function HideDialog()
	function InitMessageBox(string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetText(string)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function ShowDialog(boolean)
	function WindowName()

class CUIMMShniaga (CUIWindow)
	const epi_main = 0
	const epi_new_game = 1
	const epi_new_network_game = 2

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPage(enum CUIMMShniaga::enum_page_id, string, string)
	function SetPPMode()
	function SetVisibleMagnifier(boolean)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function ShowPage(enum CUIMMShniaga::enum_page_id)
	function WindowName()

class CUIMotionIcon (CUIWindow)
	CUIMotionIcon()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIPdaWnd (CUIDialogWnd)
	CUIPdaWnd()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetActiveDialog()
	function GetActiveSection()
	function GetHeight() const
	function GetHolder()
	function GetTabControl()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function HideDialog()
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetActiveDialog(CUIWindow*)
	function SetActiveSubdialog(string)
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function ShowDialog(boolean)
	function WindowName()

class CUIProgressBar (CUIWindow)
	CUIProgressBar()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetProgressPos()
	function GetRange_max()
	function GetRange_min()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetProgressPos(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIPropertiesBox (CUIFrameWindow)
	CUIPropertiesBox()

	function AddItem(string)
	function AttachChild(CUIWindow*)
	function AutoUpdateSize()
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetSelectedItem()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function Hide()
	function InitPropertiesBox(vector2, vector2)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function RemoveAll()
	function RemoveItem(number)
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetColor(number)
	function SetHeight(number)
	function SetPPMode()
	function SetWidth(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function Show(number, number)
	function WindowName()

class CUIScriptWnd (CUIDialogWnd, DLL_Pure)
	CUIScriptWnd()

	function _construct()
	function AddCallback(string, number, const function<void>&, object)
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Dispatch(number, number)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetHolder()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function HideDialog()
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function Load(string)
	function OnKeyboard(number, enum EUIMessages)
	function Register(CUIWindow*, string)
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function ShowDialog(boolean)
	function Update()
	function WindowName()

class CUIScrollView (CUIWindow)
	CUIScrollView()

	function AddWindow(CUIWindow*, boolean)
	function AttachChild(CUIWindow*)
	function Clear()
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetCurrentScrollPos()
	function GetHeight() const
	function GetMaxScrollPos()
	function GetMinScrollPos()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function RemoveWindow(CUIWindow*)
	function ResetPPMode()
	function ScrollToBegin()
	function ScrollToEnd()
	function SetAutoDelete(boolean)
	function SetFixedScrollBar(boolean)
	function SetPPMode()
	function SetScrollPos(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUISleepStatic (CUIStatic)
	CUISleepStatic()

	function AdjustHeightToText()
	function AdjustWidthToText()
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetStretchTexture()
	function GetTextureColor() const
	function GetTextureRect()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function InitTexture(string)
	function InitTextureEx(string, string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetStretchTexture(boolean)
	function SetTextureColor(number)
	function SetTextureRect(Frect*)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function TextControl()
	function WindowName()

class CUISpinFlt (CUICustomSpin)
	CUISpinFlt()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetText()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUISpinNum (CUICustomSpin)
	CUISpinNum()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetText()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUISpinText (CUICustomSpin)
	CUISpinText()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetText()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIStatic (CUIWindow)
	CUIStatic()

	function AdjustHeightToText()
	function AdjustWidthToText()
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetStretchTexture()
	function GetTextureColor() const
	function GetTextureRect()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function InitTexture(string)
	function InitTextureEx(string, string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetStretchTexture(boolean)
	function SetTextureColor(number)
	function SetTextureRect(Frect*)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function TextControl()
	function WindowName()

class CUITabButton (CUIButton)
	CUITabButton()

	function AdjustHeightToText()
	function AdjustWidthToText()
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetStretchTexture()
	function GetTextureColor() const
	function GetTextureRect()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function InitTexture(string)
	function InitTextureEx(string, string)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetStretchTexture(boolean)
	function SetTextureColor(number)
	function SetTextureRect(Frect*)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function TextControl()
	function WindowName()

class CUITabControl (CUIWindow)
	CUITabControl()

	function AddItem(CUITabButton*)
	function AddItem(string, string, vector2, vector2)
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetActiveId()
	function GetButtonById(string)
	function GetHeight() const
	function GetTabsCount() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function RemoveAll()
	function ResetPPMode()
	function SetActiveTab(string)
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUITextWnd (CUIWindow)
	CUITextWnd()

	function AdjustHeightToText()
	function AdjustWidthToText()
	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetFont()
	function GetHeight() const
	function GetText()
	function GetTextColor()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetEllipsis(boolean)
	function SetFont(CGameFont*)
	function SetPPMode()
	function SetText(string)
	function SetTextAlignment(enum CGameFont::EAligment)
	function SetTextColor(number)
	function SetTextComplexMode(boolean)
	function SetTextOffset(number, number)
	function SetTextST(string)
	function SetVTextAlignment(enum EVTextAlignment)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUITrackBar (CUIWindow)
	CUITrackBar()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetCheck()
	function GetFValue()
	function GetHeight() const
	function GetIValue()
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetCheck(boolean)
	function SetCurrentValue()
	function SetOptFBounds(number, number)
	function SetOptIBounds(number, number)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIWindow
	CUIWindow()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetPPMode()
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class CUIZoneMap
	CUIZoneMap()

	property disabled
	property visible

	function Background()
	function MapFrame()

class CWeapon (CGameObject)
	CWeapon()

	function _construct()
	function can_kill() const
	function Cost() const
	function GetAddonsState() const
	function getEnabled() const
	function GetMagazine()
	function getVisible() const
	function GetZoomFactor() const
	function IsZoomed() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function ready_to_kill() const
	function SetAddonsState(number)
	function SetZoomFactor(number)
	function use(CGameObject*)
	function Visual() const
	function Weight() const

class CWeaponAK74 (CGameObject)
	CWeaponAK74()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponAmmo (CGameObject)
	CWeaponAmmo()

	property m_4to1_tracer
	property m_boxCurr
	property m_boxSize
	property m_tracer

	function _construct()
	function Cost() const
	function Get(CCartridge&)
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const
	function Weight() const

class CWeaponAutomaticShotgun (CGameObject)
	CWeaponAutomaticShotgun()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponBinoculars (CGameObject)
	CWeaponBinoculars()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponBM16 (CGameObject)
	CWeaponBM16()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponFN2000 (CGameObject)
	CWeaponFN2000()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponFORT (CGameObject)
	CWeaponFORT()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponGroza (CGameObject)
	CWeaponGroza()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponHPSA (CGameObject)
	CWeaponHPSA()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponKnife (CGameObject)
	CWeaponKnife()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponLR300 (CGameObject)
	CWeaponLR300()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponMagazined (CGameObject)
	CWeaponMagazined()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponMagazinedWGrenade (CGameObject)
	CWeaponMagazinedWGrenade()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponPM (CGameObject)
	CWeaponPM()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponRG6 (CGameObject)
	CWeaponRG6()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponRPG7 (CGameObject)
	CWeaponRPG7()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponShotgun (CGameObject)
	CWeaponShotgun()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponSVD (CGameObject)
	CWeaponSVD()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponSVU (CGameObject)
	CWeaponSVU()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponUSP45 (CGameObject)
	CWeaponUSP45()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponVal (CGameObject)
	CWeaponVal()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponVintorez (CGameObject)
	CWeaponVintorez()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CWeaponWalther (CGameObject)
	CWeaponWalther()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CZombie (CGameObject)
	CZombie()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class CZoneCampfire (CGameObject)
	CZoneCampfire()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function is_on()
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function turn_off()
	function turn_on()
	function use(CGameObject*)
	function Visual() const

class CZudaArtefact (CArtefact)
	CZudaArtefact()

	property m_bCanSpawnZone
	property m_fBleedingRestoreSpeed
	property m_fHealthRestoreSpeed
	property m_fPowerRestoreSpeed
	property m_fRadiationRestoreSpeed
	property m_fSatietyRestoreSpeed

	function _construct()
	function ActivateArtefact()
	function AdditionalInventoryWeight() const
	function CanBeActivated()
	function FollowByPath(string, number, vector)
	function GetAfRank() const
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function SwitchVisibility(boolean)
	function use(CGameObject*)
	function Visual() const

class danger_object
	const attack_sound = 1
	const attacked = 5
	const bullet_ricochet = 0
	const enemy_sound = 7
	const entity_attacked = 2
	const entity_corpse = 4
	const entity_death = 3
	const grenade = 6
	const hit = 2
	const sound = 1
	const visual = 0

	operator ==(const danger_object&, danger_object)

	function dependent_object(const danger_object*)
	function object(const danger_object*)
	function perceive_type() const
	function position(const danger_object*)
	function time() const
	function type() const

class demo_info
	function get_author_name() const
	function get_game_score() const
	function get_game_type() const
	function get_map_name() const
	function get_map_version() const
	function get_player(number) const
	function get_players_count() const

class demo_player_info
	function get_artefacts() const
	function get_deaths() const
	function get_frags() const
	function get_name() const
	function get_rank() const
	function get_spots() const
	function get_team() const

class DIK_keys
	const DIK_0 = 11
	const DIK_1 = 2
	const DIK_2 = 3
	const DIK_3 = 4
	const DIK_4 = 5
	const DIK_5 = 6
	const DIK_6 = 7
	const DIK_7 = 8
	const DIK_8 = 9
	const DIK_9 = 10
	const DIK_A = 30
	const DIK_ADD = 78
	const DIK_APOSTROPHE = 40
	const DIK_APPS = 221
	const DIK_AT = 145
	const DIK_AX = 150
	const DIK_B = 48
	const DIK_BACK = 14
	const DIK_BACKSLASH = 43
	const DIK_C = 46
	const DIK_CAPITAL = 58
	const DIK_CIRCUMFLEX = 144
	const DIK_COLON = 146
	const DIK_COMMA = 51
	const DIK_CONVERT = 121
	const DIK_D = 32
	const DIK_DECIMAL = 83
	const DIK_DELETE = 211
	const DIK_DIVIDE = 181
	const DIK_DOWN = 208
	const DIK_E = 18
	const DIK_END = 207
	const DIK_EQUALS = 13
	const DIK_ESCAPE = 1
	const DIK_F = 33
	const DIK_F1 = 59
	const DIK_F10 = 68
	const DIK_F11 = 87
	const DIK_F12 = 88
	const DIK_F13 = 100
	const DIK_F14 = 101
	const DIK_F15 = 102
	const DIK_F2 = 60
	const DIK_F3 = 61
	const DIK_F4 = 62
	const DIK_F5 = 63
	const DIK_F6 = 64
	const DIK_F7 = 65
	const DIK_F8 = 66
	const DIK_F9 = 67
	const DIK_G = 34
	const DIK_GRAVE = 41
	const DIK_H = 35
	const DIK_HOME = 199
	const DIK_I = 23
	const DIK_INSERT = 210
	const DIK_J = 36
	const DIK_K = 37
	const DIK_KANA = 112
	const DIK_KANJI = 148
	const DIK_L = 38
	const DIK_LBRACKET = 26
	const DIK_LCONTROL = 29
	const DIK_LEFT = 203
	const DIK_LMENU = 56
	const DIK_LSHIFT = 42
	const DIK_LWIN = 219
	const DIK_M = 50
	const DIK_MINUS = 12
	const DIK_MULTIPLY = 55
	const DIK_N = 49
	const DIK_NEXT = 209
	const DIK_NOCONVERT = 123
	const DIK_NUMLOCK = 69
	const DIK_NUMPAD0 = 82
	const DIK_NUMPAD1 = 79
	const DIK_NUMPAD2 = 80
	const DIK_NUMPAD3 = 81
	const DIK_NUMPAD4 = 75
	const DIK_NUMPAD5 = 76
	const DIK_NUMPAD6 = 77
	const DIK_NUMPAD7 = 71
	const DIK_NUMPAD8 = 72
	const DIK_NUMPAD9 = 73
	const DIK_NUMPADCOMMA = 179
	const DIK_NUMPADENTER = 156
	const DIK_NUMPADEQUALS = 141
	const DIK_O = 24
	const DIK_P = 25
	const DIK_PAUSE = 197
	const DIK_PERIOD = 52
	const DIK_PRIOR = 201
	const DIK_Q = 16
	const DIK_R = 19
	const DIK_RBRACKET = 27
	const DIK_RCONTROL = 157
	const DIK_RETURN = 28
	const DIK_RIGHT = 205
	const DIK_RMENU = 184
	const DIK_RSHIFT = 54
	const DIK_RWIN = 220
	const DIK_S = 31
	const DIK_SCROLL = 70
	const DIK_SEMICOLON = 39
	const DIK_SLASH = 53
	const DIK_SPACE = 57
	const DIK_STOP = 149
	const DIK_SUBTRACT = 74
	const DIK_SYSRQ = 183
	const DIK_T = 20
	const DIK_TAB = 15
	const DIK_U = 22
	const DIK_UNDERLINE = 147
	const DIK_UNLABELED = 151
	const DIK_UP = 200
	const DIK_V = 47
	const DIK_W = 17
	const DIK_X = 45
	const DIK_Y = 21
	const DIK_YEN = 125
	const DIK_Z = 44
	const MOUSE_1 = 337
	const MOUSE_2 = 338
	const MOUSE_3 = 339
	const MOUSE_4 = 340
	const MOUSE_5 = 341
	const MOUSE_6 = 342
	const MOUSE_7 = 343
	const MOUSE_8 = 344

class DLL_Pure
	DLL_Pure()

	function _construct()

class duality
	duality()
	duality(number, number)

	property h
	property v

	function set(number, number)

class EDDListType
	const iActorBag = 2
	const iActorBelt = 3
	const iActorSlot = 1
	const iActorTrade = 4
	const iDeadBodyBag = 7
	const iInvalid = 0
	const iPartnerTrade = 6
	const iPartnerTradeBag = 5
	const iQuickSlot = 8
	const iTrashSlot = 9

class effector
	effector(number, number)

	function finish(effector*)
	function process(effector_params*)
	function start(effector*)

class effector_params
	effector_params()

	property blur
	property color_add
	property color_base
	property color_gray
	property dual
	property gray
	property noise

	function assign(effector_params*, effector_params*)

class entity_action
	entity_action()
	entity_action(const entity_action*)

	function all()
	function anim() const
	function completed()
	function look() const
	function move() const
	function object() const
	function particle() const
	function set_action(act&)
	function set_action(anim&)
	function set_action(cond&)
	function set_action(look&)
	function set_action(move&)
	function set_action(object&)
	function set_action(particle&)
	function set_action(sound&)
	function sound() const
	function time()

class entity_memory_object (memory_object)
	property last_level_time
	property level_time
	property object_info
	property self_info

	function object(const entity_memory_object&)

class explosive
	function explode()

class Fbox
	Fbox()

	property max
	property min

class fcolor
	fcolor()

	property a
	property b
	property g
	property r

	function set(const fcolor&)
	function set(number)
	function set(number, number, number, number)

class flags16
	flags16()

	function and(number)
	function and(const flags16&, number)
	function assign(const flags16&)
	function assign(number)
	function equal(flags16*, const flags16&)
	function equal(flags16*, const flags16&, number)
	function get() const
	function invert()
	function invert(const flags16&)
	function invert(number)
	function is(flags16*, number)
	function is_any(flags16*, number)
	function one(flags16*)
	function or(number)
	function or(const flags16&, number)
	function set(flags16*, number, boolean)
	function test(flags16*, number)
	function zero()

class flags32
	flags32()

	function and(number)
	function and(const flags32&, number)
	function assign(const flags32&)
	function assign(number)
	function equal(flags32*, const flags32&)
	function equal(flags32*, const flags32&, number)
	function get() const
	function invert()
	function invert(const flags32&)
	function invert(number)
	function is(flags32*, number)
	function is_any(flags32*, number)
	function one()
	function or(number)
	function or(const flags32&, number)
	function set(flags32*, number, boolean)
	function test(flags32*, number)
	function zero()

class flags8
	flags8()

	function and(number)
	function and(const flags8&, number)
	function assign(const flags8&)
	function assign(number)
	function equal(flags8*, const flags8&)
	function equal(flags8*, const flags8&, number)
	function get() const
	function invert()
	function invert(const flags8&)
	function invert(number)
	function is(flags8*, number)
	function is_any(flags8*, number)
	function one(flags8*)
	function or(number)
	function or(const flags8&, number)
	function set(flags8*, number, boolean)
	function test(flags8*, number)
	function zero()

class found_email_cb
	found_email_cb()
	found_email_cb(object, function<void>)

	function bind(object, function<void>)
	function clear()

class Frect
	Frect()

	property lt
	property rb
	property x1
	property x2
	property y1
	property y2

	function set(number, number, number, number)

class FS
	const FS_ClampExt = 4
	const FS_ListFiles = 1
	const FS_ListFolders = 2
	const FS_RootOnly = 8
	const FS_sort_by_modif_down = 5
	const FS_sort_by_modif_up = 4
	const FS_sort_by_name_down = 1
	const FS_sort_by_name_up = 0
	const FS_sort_by_size_down = 3
	const FS_sort_by_size_up = 2

	function append_path(string, string, string, number)
	function dir_delete(FS*, string, number)
	function dir_delete(FS*, string, string, number)
	function exist(string)
	function exist(string, string)
	function file_copy(string, string)
	function file_delete(string)
	function file_delete(string, string)
	function file_length(string)
	function file_list_open(FS*, string, number)
	function file_list_open(FS*, string, string, number)
	function file_list_open_ex(FS*, string, number, string)
	function file_rename(string, string, boolean)
	function get_file_age(string)
	function get_file_age_str(FS*, string)
	function get_path(string)
	function path_exist(string)
	function r_close(reader*&)
	function r_open(string)
	function r_open(string, string)
	function rescan_path(FS*, string)
	function update_path(FS*, string, string)
	function w_close(class IWriter*&)
	function w_open(string)
	function w_open(string, string)

class fs_file
	property modif
	property name
	property ptr
	property size_compressed
	property size_real
	property vfs

class FS_file_list
	function Free()
	function GetAt(number)
	function Size()

class FS_file_list_ex
	function GetAt(number)
	function Size()
	function Sort(number)

class FS_item
	function Modif()
	function ModifDigitOnly()
	function NameFull()
	function NameShort()
	function Size()

class game_difficulty
	const master = 3
	const novice = 0
	const stalker = 1
	const veteran = 2

class game_memory_object (memory_object)
	property last_level_time
	property level_time
	property object_info
	property self_info

	function object(const game_memory_object&)

class game_object
	const action_type_count = 6
	const alifeMovementTypeMask = 0
	const alifeMovementTypeRandom = 1
	const animation = 2
	const dialog_pda_msg = 0
	const dummy = -1
	const enemy = 2
	const friend = 0
	const game_path = 0
	const info_pda_msg = 1
	const level_path = 1
	const movement = 0
	const neutral = 1
	const no_path = 3
	const no_pda_msg = 2
	const object = 5
	const particle = 4
	const patrol_path = 2
	const relation_attack = 1
	const relation_fight_help_human = 2
	const relation_fight_help_monster = 4
	const relation_kill = 0
	const sound = 3
	const watch = 1

	property bleeding
	property health
	property morale
	property power
	property psy_health
	property radiation
	property satiety

	function accessible(const vector&)
	function accessible(number)
	function accessible_nearest(const vector&, vector&)
	function accuracy() const
	function action() const
	function action_by_index(number)
	function action_count() const
	function activate_slot(number)
	function active_detector() const
	function active_item()
	function active_slot()
	function active_sound_count()
	function active_sound_count(boolean)
	function active_zone_contact(number)
	function actor_look_at_point(vector)
	function add_animation(string, boolean, boolean)
	function add_animation(string, boolean, vector, vector, boolean)
	function add_combat_sound(string, number, enum ESoundTypes, number, number, number, string)
	function add_restrictions(string, string)
	function add_sound(string, number, enum ESoundTypes, number, number, number)
	function add_sound(string, number, enum ESoundTypes, number, number, number, string)
	function aim_bone_id() const
	function aim_bone_id(string)
	function aim_time(game_object*)
	function aim_time(game_object*, number)
	function alive() const
	function allow_break_talk_dialog(boolean)
	function allow_sprint(boolean)
	function ammo_box_size()
	function ammo_get_count()
	function ammo_set_count(number)
	function animation_count() const
	function animation_slot() const
	function apply_loophole_direction_distance() const
	function apply_loophole_direction_distance(number)
	function attach_vehicle(game_object*, boolean)
	function attachable_item_enabled() const
	function attachable_item_load_attach(string)
	function base_in_restrictions()
	function base_out_restrictions()
	function belt_count() const
	function berserk()
	function best_cover(const vector&, const vector&, number, number, number)
	function best_danger()
	function best_enemy()
	function best_item()
	function best_weapon()
	function bind_object(object_binder*)
	function binded_object()
	function body_state() const
	function bone_position(string) const
	function bone_visible(string)
	function burer_get_force_gravi_attack()
	function burer_set_force_gravi_attack(boolean)
	function buy_condition(ini_file*, string)
	function buy_condition(number, number)
	function buy_item_condition_factor(number)
	function buy_supplies(ini_file*, string)
	function can_script_capture() const
	function can_select_weapon() const
	function can_select_weapon(boolean)
	function can_throw_grenades() const
	function can_throw_grenades(boolean)
	function cast_Actor()
	function cast_Ammo()
	function cast_Antirad()
	function cast_Artefact()
	function cast_BottleItem()
	function cast_Car()
	function cast_CustomOutfit()
	function cast_CustomZone()
	function cast_EatableItem()
	function cast_EntityAlive()
	function cast_Explosive()
	function cast_FoodItem()
	function cast_GameObject()
	function cast_GrenadeLauncher()
	function cast_Heli()
	function cast_HolderCustom()
	function cast_HudItem()
	function cast_InventoryBox()
	function cast_InventoryItem()
	function cast_InventoryOwner()
	function cast_Medkit()
	function cast_Monster()
	function cast_PhysicsShellHolder()
	function cast_Scope()
	function cast_ScriptZone()
	function cast_Silencer()
	function cast_SpaceRestrictor()
	function cast_Stalker()
	function cast_Torch()
	function cast_Trader()
	function cast_Weapon()
	function cast_WeaponMagazined()
	function cast_WeaponMagazinedWGrenade()
	function center()
	function change_character_rank(number)
	function change_character_reputation(number)
	function change_goodwill(number, game_object*)
	function change_team(number, number, number)
	function character_community()
	function character_icon()
	function character_name()
	function character_rank()
	function character_reputation()
	function clear_animations()
	function clear_override_animation()
	function clsid() const
	function command(const entity_action*, boolean)
	function community_goodwill(string)
	function condition() const
	function cost() const
	function critically_wounded()
	function deadbody_can_take(boolean)
	function deadbody_can_take_status()
	function deadbody_closed(boolean)
	function deadbody_closed_status()
	function death_sound_enabled() const
	function death_sound_enabled(boolean)
	function death_time() const
	function debug_planner(const action_planner*)
	function detach_vehicle(boolean)
	function detail_path_type() const
	function direction() const
	function disable_anomaly()
	function disable_hit_marks() const
	function disable_hit_marks(boolean)
	function disable_info_portion(string)
	function disable_inv_upgrade()
	function disable_show_hide_sounds(boolean)
	function disable_talk()
	function disable_trade()
	function dont_has_info(string)
	function drop_item(game_object*)
	function drop_item_and_teleport(game_object*, vector)
	function eat(game_object*)
	function enable_anomaly()
	function enable_attachable_item(boolean)
	function enable_inv_upgrade()
	function enable_level_changer(boolean)
	function enable_memory_object(game_object*, boolean)
	function enable_night_vision(boolean)
	function enable_talk()
	function enable_torch(boolean)
	function enable_trade()
	function enable_vision(boolean)
	function explode(number)
	function external_sound_start(string)
	function external_sound_stop()
	function extrapolate_length() const
	function extrapolate_length(number)
	function fake_death_fall_down()
	function fake_death_stand_up()
	function find_best_cover(vector)
	function force_set_goodwill(number, game_object*)
	function force_set_position(vector, boolean)
	function force_stand_sleep_animation(number)
	function force_visibility_state(number)
	function fov() const
	function game_vertex_id() const
	function general_goodwill(game_object*)
	function get_actor_jump_speed() const
	function get_actor_max_walk_weight() const
	function get_actor_max_weight() const
	function get_actor_relation_flags() const
	function get_actor_run_coef() const
	function get_actor_runback_coef() const
	function get_actor_sprint_koef() const
	function get_additional_max_walk_weight() const
	function get_additional_max_weight() const
	function get_ammo_count_for_type(number)
	function get_ammo_in_magazine()
	function get_ammo_total() const
	function get_ammo_type()
	function get_anomaly_power()
	function get_artefact()
	function get_artefact_bleeding()
	function get_artefact_health()
	function get_artefact_power()
	function get_artefact_radiation()
	function get_artefact_satiety()
	function get_attached_vehicle()
	function get_bone_id(string) const
	function get_campfire()
	function get_car()
	function get_corpse() const
	function get_current_direction()
	function get_current_holder()
	function get_current_outfit() const
	function get_current_outfit_protection(number)
	function get_current_point_index()
	function get_dest_game_vertex_id()
	function get_dest_level_vertex_id()
	function get_dest_smart_cover()
	function get_dest_smart_cover_name()
	function get_enemy() const
	function get_enemy_strength() const
	function get_force_anti_aim()
	function get_hanging_lamp()
	function get_helicopter()
	function get_holder_class()
	function get_luminocity()
	function get_luminocity_hemi()
	function get_main_weapon_type()
	function get_max_uses()
	function get_monster_hit_info()
	function get_movement_speed() const
	function get_physics_object()
	function get_physics_shell() const
	function get_remaining_uses()
	function get_restrictor_type()
	function get_script() const
	function get_script_name() const
	function get_smart_cover_description() const
	function get_sound_info()
	function get_spatial_type()
	function get_start_dialog()
	function get_state()
	function get_task(string, boolean)
	function get_task_state(string)
	function get_total_weight() const
	function get_visibility_state()
	function get_visual_name() const
	function get_weapon_substate()
	function get_weapon_type()
	function give_game_news(string, string, string, number, number)
	function give_game_news(string, string, string, number, number, number)
	function give_info_portion(string)
	function give_money(number)
	function give_talk_message(string, string, string)
	function give_talk_message2(string, string, string, string)
	function give_task(CGameTask*, number, boolean, number)
	function goodwill(game_object*)
	function group() const
	function group_throw_time_interval() const
	function group_throw_time_interval(number)
	function has_ammo_type(number)
	function has_info(string)
	function has_upgrade(string)
	function head_orientation() const
	function hide_weapon()
	function hit(hit*)
	function id() const
	function idle_max_time() const
	function idle_max_time(number)
	function idle_min_time() const
	function idle_min_time(number)
	function ignore_monster_threshold() const
	function ignore_monster_threshold(number)
	function in_current_loophole_fov(vector) const
	function in_current_loophole_range(vector) const
	function in_loophole_fov(string, string, vector) const
	function in_loophole_range(string, string, vector) const
	function in_restrictions()
	function in_smart_cover() const
	function inactualize_game_path()
	function inactualize_level_path()
	function inactualize_patrol_path()
	function info_add(string)
	function info_clear()
	function inside(const vector&) const
	function inside(const vector&, number) const
	function install_upgrade(string)
	function inv_box_can_take(boolean)
	function inv_box_can_take_status()
	function inv_box_closed(boolean, string)
	function inv_box_closed_status()
	function inventory_for_each(const function<boolean>&)
	function invulnerable() const
	function invulnerable(boolean)
	function is_active_task(CGameTask*)
	function is_body_turning() const
	function is_door_blocked_by_npc() const
	function is_door_locked_for_npc() const
	function is_inv_box_empty()
	function is_inv_upgrade_enabled()
	function is_level_changer_enabled()
	function is_on_belt(game_object*) const
	function is_talk_enabled()
	function is_talking()
	function is_there_items_to_pickup() const
	function is_trade_enabled()
	function is_weapon_going_to_be_strapped(const game_object*) const
	function item_in_slot(number) const
	function item_on_belt(number) const
	function iterate_feel_touch(const function<boolean>&)
	function iterate_installed_upgrades(const function<boolean>&)
	function iterate_inventory(function<boolean>, object)
	function iterate_inventory_box(function<boolean>, object)
	function jump(const vector&, number)
	function kill(game_object*, boolean)
	function level_vertex_id() const
	function location_on_path(number, vector*)
	function lock_door_for_npc()
	function lookout_max_time() const
	function lookout_max_time(number)
	function lookout_min_time() const
	function lookout_min_time(number)
	function make_item_active(game_object*)
	function make_object_visible_somewhen(game_object*)
	function mark_item_dropped(game_object*, boolean)
	function marked_dropped(game_object*)
	function mass() const
	function max_health() const
	function max_ignore_monster_distance() const
	function max_ignore_monster_distance(const number&)
	function memory_hit_objects() const
	function memory_position(const game_object&)
	function memory_sound_objects() const
	function memory_time(const game_object&)
	function memory_visible_objects() const
	function mental_state() const
	function money()
	function motivation_action_manager(game_object*)
	function movement_enabled()
	function movement_enabled(boolean)
	function movement_target_reached()
	function movement_type() const
	function name() const
	function night_vision_allowed(boolean)
	function night_vision_enabled() const
	function not_yet_visible_objects() const
	function object(number)
	function object(string)
	function object_count() const
	function on_door_is_closed()
	function on_door_is_open()
	function out_restrictions()
	function parent() const
	function path_completed() const
	function path_type() const
	function patrol()
	function patrol_path_make_inactual()
	function phantom_set_enemy(game_object*)
	function play_cycle(string)
	function play_cycle(string, boolean)
	function play_hud_motion(string, boolean, number)
	function play_sound(number)
	function play_sound(number, number)
	function play_sound(number, number, number)
	function play_sound(number, number, number, number)
	function play_sound(number, number, number, number, number)
	function play_sound(number, number, number, number, number, number)
	function poltergeist_get_actor_ignore()
	function poltergeist_set_actor_ignore(boolean)
	function position() const
	function profile_name()
	function range() const
	function rank()
	function register_door_for_npc()
	function register_in_combat()
	function relation(game_object*)
	function release_stand_sleep_animation()
	function remove_all_restrictions()
	function remove_danger(const danger_object&)
	function remove_home()
	function remove_memory_hit_object(const hit_memory_object&)
	function remove_memory_sound_object(const sound_memory_object&)
	function remove_memory_visible_object(const visible_memory_object&)
	function remove_restrictions(string, string)
	function remove_sound(number)
	function reset_action_queue()
	function reset_bone_protections(string, string)
	function restore_default_start_dialog()
	function restore_ignore_monster_threshold()
	function restore_max_ignore_monster_distance()
	function restore_sound_threshold()
	function restore_weapon()
	function run_talk_dialog(game_object*, boolean)
	function safe_cover(const vector&, number, number)
	function script(boolean, string)
	function section() const
	function see(const game_object*)
	function see(string)
	function sell_condition(ini_file*, string)
	function sell_condition(number, number)
	function set_active_task(CGameTask*)
	function set_actor_direction(number)
	function set_actor_jump_speed(number)
	function set_actor_max_walk_weight(number)
	function set_actor_max_weight(number)
	function set_actor_position(vector, boolean)
	function set_actor_relation_flags(flags32)
	function set_actor_run_coef(number)
	function set_actor_runback_coef(number)
	function set_actor_sprint_koef(number)
	function set_additional_max_walk_weight(number)
	function set_additional_max_weight(number)
	function set_alien_control(boolean)
	function set_ammo_elapsed(number)
	function set_ammo_type(number)
	function set_anomaly_power(number)
	function set_artefact_bleeding(number)
	function set_artefact_health(number)
	function set_artefact_power(number)
	function set_artefact_radiation(number)
	function set_artefact_satiety(number)
	function set_body_state(enum MonsterSpace::EBodyState)
	function set_bone_visible(string, boolean, boolean)
	function set_callback(enum GameObject::ECallbackType)
	function set_callback(enum GameObject::ECallbackType, const function<void>&)
	function set_callback(enum GameObject::ECallbackType, const function<void>&, object)
	function set_capture_anim(game_object*, string, const vector&, number)
	function set_character_community(string, number, number)
	function set_character_icon(string)
	function set_character_rank(number)
	function set_character_reputation(number)
	function set_collision_off(boolean)
	function set_community_goodwill(string, number)
	function set_condition(number)
	function set_const_force(const vector&, number, number)
	function set_custom_panic_threshold(number)
	function set_default_panic_threshold()
	function set_desired_direction()
	function set_desired_direction(const vector*)
	function set_desired_position()
	function set_desired_position(const vector*)
	function set_dest_game_vertex_id(number)
	function set_dest_level_vertex_id(number)
	function set_dest_loophole()
	function set_dest_loophole(string)
	function set_dest_smart_cover()
	function set_dest_smart_cover(string)
	function set_detail_path_type(enum DetailPathManager::EDetailPathType)
	function set_enemy(game_object*)
	function set_enemy_callback()
	function set_enemy_callback(const function<boolean>&)
	function set_enemy_callback(const function<boolean>&, object)
	function set_fastcall(const function<boolean>&, object)
	function set_force_anti_aim(boolean)
	function set_fov(number)
	function set_goodwill(number, game_object*)
	function set_health_ex(number)
	function set_home(number, number, number, boolean, number)
	function set_home(string, number, number, boolean, number)
	function set_invisible(boolean)
	function set_item(enum MonsterSpace::EObjectAction)
	function set_item(enum MonsterSpace::EObjectAction, game_object*)
	function set_item(enum MonsterSpace::EObjectAction, game_object*, number)
	function set_item(enum MonsterSpace::EObjectAction, game_object*, number, number)
	function set_level_changer_invitation(string)
	function set_main_weapon_type(number)
	function set_manual_invisibility(boolean)
	function set_mental_state(enum MonsterSpace::EMentalState)
	function set_movement_selection_type(enum ESelectionType)
	function set_movement_type(enum MonsterSpace::EMovementType)
	function set_nonscript_usable(boolean)
	function set_npc_position(vector)
	function set_override_animation(string)
	function set_path_type(enum MovementManager::EPathType)
	function set_patrol_extrapolate_callback()
	function set_patrol_extrapolate_callback(const function<boolean>&)
	function set_patrol_extrapolate_callback(const function<boolean>&, object)
	function set_patrol_path(string, enum PatrolPathManager::EPatrolStartType, enum PatrolPathManager::EPatrolRouteType, boolean)
	function set_previous_point(number)
	function set_queue_size(number)
	function set_range(number)
	function set_relation(enum ALife::ERelationType, game_object*)
	function set_remaining_uses(number)
	function set_restrictor_type(number)
	function set_sight(game_object*)
	function set_sight(enum SightManager::ESightType, vector*)
	function set_sight(game_object*, boolean)
	function set_sight(enum SightManager::ESightType, boolean, boolean)
	function set_sight(enum SightManager::ESightType, vector&, boolean)
	function set_sight(enum SightManager::ESightType, vector*, number)
	function set_sight(game_object*, boolean, boolean)
	function set_sight(game_object*, boolean, boolean, boolean)
	function set_smart_cover_target()
	function set_smart_cover_target(game_object*)
	function set_smart_cover_target(vector)
	function set_smart_cover_target_default(boolean)
	function set_smart_cover_target_fire()
	function set_smart_cover_target_fire_no_lookout()
	function set_smart_cover_target_idle()
	function set_smart_cover_target_lookout()
	function set_smart_cover_target_selector()
	function set_smart_cover_target_selector(function<void>)
	function set_smart_cover_target_selector(function<void>, object)
	function set_sound_mask(number)
	function set_sound_threshold(number)
	function set_spatial_type(number)
	function set_start_dialog(string)
	function set_start_point(number)
	function set_sympathy(number)
	function set_task_state(enum ETaskState, string)
	function set_tip_text(string)
	function set_tip_text_default()
	function set_trader_global_anim(string)
	function set_trader_head_anim(string)
	function set_trader_sound(string, string)
	function set_vis_state(number)
	function set_visual_memory_enabled(boolean)
	function set_visual_name(string, boolean)
	function set_weapon_type(number)
	function set_weight(number)
	function show_condition(ini_file*, string)
	function sight_params()
	function skip_transfer_enemy(boolean)
	function sniper_fire_mode() const
	function sniper_fire_mode(boolean)
	function sniper_update_rate() const
	function sniper_update_rate(boolean)
	function sound_prefix() const
	function sound_prefix(string)
	function sound_voice_prefix() const
	function spawn_ini() const
	function special_danger_move()
	function special_danger_move(boolean)
	function squad() const
	function start_particles(string, string)
	function start_trade(game_object*)
	function start_upgrade(game_object*)
	function stop_particles(string, string)
	function stop_talk()
	function story_id() const
	function suitable_smart_cover(game_object*)
	function switch_state(number)
	function switch_to_talk()
	function switch_to_trade()
	function switch_to_upgrade()
	function sympathy()
	function take_items_enabled() const
	function take_items_enabled(boolean)
	function target_body_state() const
	function target_mental_state() const
	function target_movement_type() const
	function team() const
	function torch_enabled() const
	function transfer_item(game_object*, game_object*)
	function transfer_money(number, game_object*)
	function unload_magazine()
	function unlock_door_for_npc()
	function unregister_door_for_npc()
	function unregister_in_combat()
	function use(game_object*)
	function use_smart_covers_only() const
	function use_smart_covers_only(boolean)
	function vertex_in_direction(number, vector, number) const
	function visibility_threshold() const
	function vision_enabled() const
	function weapon_addon_attach(game_object*)
	function weapon_addon_detach(string, boolean)
	function weapon_get_ammo_section(number)
	function weapon_get_scope()
	function weapon_grenadelauncher_status()
	function weapon_in_grenade_mode()
	function weapon_is_grenadelauncher()
	function weapon_is_scope()
	function weapon_is_silencer()
	function weapon_scope_status()
	function weapon_set_scope(number)
	function weapon_silencer_status()
	function weapon_strapped() const
	function weapon_unstrapped() const
	function weight() const
	function who_hit_name()
	function who_hit_section_name()
	function wounded() const
	function wounded(boolean)

class GAME_TYPE
	const eGameIDArtefactHunt = 8
	const eGameIDCaptureTheArtefact = 16
	const eGameIDDeathmatch = 2
	const eGameIDTeamDeathmatch = 4
	const GAME_UNKNOWN = -1

class GameGraph__CVertex
	function game_point(const GameGraph__CVertex*)
	function level_id() const
	function level_point(const GameGraph__CVertex*)
	function level_vertex_id() const
	function mask(const GameGraph__CVertex*)

class GameGraph__LEVEL_MAP__value_type
	property id
	property level

class hanging_lamp (CGameObject)
	hanging_lamp()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function turn_off()
	function turn_on()
	function use(CGameObject*)
	function Visual() const

class hit
	const burn = 0
	const chemical_burn = 2
	const dummy = 11
	const explosion = 8
	const fire_wound = 6
	const light_burn = 10
	const radiation = 3
	const shock = 1
	const strike = 7
	const telepatic = 4
	const wound = 5

	hit()
	hit(const hit*)

	property direction
	property draftsman
	property impulse
	property power
	property type

	function bone(string)

class hit_memory_object (entity_memory_object)
	property amount
	property bone_index
	property direction
	property last_level_time
	property level_time
	property object_info
	property self_info

	function object(const entity_memory_object&)

class holder
	function Action(number, number)
	function engaged()
	function SetEnterLocked(boolean)
	function SetExitLocked(boolean)
	function SetParam(number, vector)

class ICollidable
	ICollidable()

class IKinematicsAnimated
	function PlayCycle(IKinematicsAnimated*, string)

class ini_file
	ini_file(string)
	ini_file(string, number, number, number, string)

	function line_count(string)
	function line_exist(string, string)
	function r_bool(string, string)
	function r_clsid(string, string)
	function r_float(string, string)
	function r_line(ini_file*, string, number, string&, string&)
	function r_s32(string, string)
	function r_string(string, string)
	function r_string_wq(string, string)
	function r_token(string, string, const token_list&)
	function r_u32(string, string)
	function r_vector(string, string)
	function remove_line(string, string)
	function save_as(string)
	function save_at_end(boolean)
	function section_count()
	function section_exist(string)
	function section_for_each(ini_file*, function<boolean>)
	function set_override_names(boolean)
	function set_readonly(boolean)
	function w_bool(string, string, boolean, string)
	function w_color(string, string, number, string)
	function w_fcolor(string, string, const fcolor&, string)
	function w_float(string, string, number, string)
	function w_fvector2(string, string, const vector2&, string)
	function w_fvector3(string, string, const vector&, string)
	function w_fvector4(string, string, const struct _vector4<number>&, string)
	function w_s16(string, string, number, string)
	function w_s32(string, string, number, string)
	function w_s64(string, string, __int64, string)
	function w_s8(string, string, signed char, string)
	function w_string(string, string, string, string)
	function w_u16(string, string, number, string)
	function w_u32(string, string, number, string)
	function w_u64(string, string, unsigned __int64, string)
	function w_u8(string, string, number, string)

class ipure_alife_load_object
class ipure_alife_load_save_object (ipure_alife_load_object, ipure_alife_save_object)
class ipure_alife_save_object
class ipure_schedulable_object
class ipure_server_object (ipure_alife_load_save_object)
class IRender_Visual
	function dcast_PKinematicsAnimated()

class IRenderable
class ISheduled
class key_bindings
	const kACCEL = 6
	const kARTEFACT = 28
	const kBACK = 9
	const kBUY = 48
	const kCAM_1 = 14
	const kCAM_2 = 15
	const kCAM_3 = 16
	const kCAM_AUTOAIM = 82
	const kCAM_ZOOM_IN = 17
	const kCAM_ZOOM_OUT = 18
	const kCHAT = 42
	const kCONSOLE = 46
	const kCROUCH = 5
	const kCUSTOM1 = 67
	const kCUSTOM10 = 76
	const kCUSTOM11 = 77
	const kCUSTOM12 = 78
	const kCUSTOM13 = 79
	const kCUSTOM14 = 80
	const kCUSTOM15 = 81
	const kCUSTOM2 = 68
	const kCUSTOM3 = 69
	const kCUSTOM4 = 70
	const kCUSTOM5 = 71
	const kCUSTOM6 = 72
	const kCUSTOM7 = 73
	const kCUSTOM8 = 74
	const kCUSTOM9 = 75
	const kDOWN = 3
	const kDROP = 39
	const kFWD = 8
	const kINVENTORY = 47
	const kJUMP = 4
	const kL_LOOKOUT = 12
	const kL_STRAFE = 10
	const kLEFT = 0
	const kNIGHT_VISION = 20
	const kPDA_TAB1 = 83
	const kPDA_TAB2 = 84
	const kPDA_TAB3 = 85
	const kPDA_TAB4 = 86
	const kPDA_TAB5 = 87
	const kPDA_TAB6 = 88
	const kQUICK_LOAD = 66
	const kQUICK_SAVE = 65
	const kQUIT = 45
	const kR_LOOKOUT = 13
	const kR_STRAFE = 11
	const kRIGHT = 1
	const kSCORES = 41
	const kSCREENSHOT = 44
	const kSKIN = 49
	const kTEAM = 50
	const kTORCH = 19
	const kUP = 2
	const kUSE = 40
	const kWPN_1 = 22
	const kWPN_2 = 23
	const kWPN_3 = 24
	const kWPN_4 = 25
	const kWPN_5 = 26
	const kWPN_6 = 27
	const kWPN_FIRE = 30
	const kWPN_FUNC = 35
	const kWPN_NEXT = 29
	const kWPN_RELOAD = 34
	const kWPN_ZOOM = 31

class login_manager
	function forgot_password(string)
	function get_current_profile() const
	function get_email_from_registry()
	function get_nick_from_registry()
	function get_password_from_registry()
	function get_remember_me_from_registry()
	function login(string, string, string, login_operation_cb)
	function login_offline(string, login_operation_cb)
	function logout()
	function save_email_to_registry(string)
	function save_nick_to_registry(string)
	function save_password_to_registry(string)
	function save_remember_me_to_registry(boolean)
	function set_unique_nick(string, login_operation_cb)
	function stop_login()
	function stop_setting_unique_nick()

class login_operation_cb
	login_operation_cb()
	login_operation_cb(object, function<void>)

	function bind(object, function<void>)
	function clear()

class look
	const cur_dir = 0
	const danger = 5
	const direction = 2
	const fire_point = 10
	const path_dir = 1
	const point = 3
	const search = 6

	look()
	look(enum SightManager::ESightType)
	look(enum SightManager::ESightType, game_object*)
	look(enum SightManager::ESightType, vector&)
	look(const vector&, number, number)
	look(enum SightManager::ESightType, game_object*, string)
	look(game_object*, number, number)

	function bone(string)
	function completed()
	function direct(const vector&)
	function object(game_object*)
	function type(enum SightManager::ESightType)

class matrix
	matrix()

	property _14_
	property _24_
	property _34_
	property _44_
	property c
	property i
	property j
	property k

	function div(number)
	function div(const matrix&, number)
	function getHPB(matrix*, number*, number*, number*)
	function identity()
	function mk_xform(const struct _quaternion<number>&, const vector&)
	function mul(number)
	function mul(const matrix&, const matrix&)
	function mul(const matrix&, number)
	function set(const matrix&)
	function set(const vector&, const vector&, const vector&, const vector&)
	function setHPB(number, number, number)
	function setXYZ(number, number, number)
	function setXYZi(number, number, number)

class MEMBERS__value_type
	property id
	property object

class memory_info (visible_memory_object)
	property hit_info
	property last_level_time
	property level_time
	property object_info
	property self_info
	property sound_info
	property visual_info

	function object(const game_memory_object&)

class memory_object
	property last_level_time
	property level_time

class MonsterHitInfo
	property direction
	property time
	property who

class MonsterSpace
	const head_anim_angry = 1
	const head_anim_glad = 2
	const head_anim_kind = 3
	const head_anim_normal = 0
	const sound_script = 128

class move
	const back = 4
	const criteria = 2
	const crouch = 0
	const curve = 0
	const curve_criteria = 2
	const default = 0
	const dodge = 1
	const down = 64
	const drag = 3
	const force = 1
	const fwd = 2
	const handbrake = 128
	const jump = 4
	const left = 8
	const line = 0
	const none = 1
	const off = 512
	const on = 256
	const right = 16
	const run = 1
	const run_fwd = 2
	const run_with_leader = 7
	const stand = 2
	const standing = 1
	const steal = 5
	const up = 32
	const walk = 0
	const walk_bkwd = 1
	const walk_fwd = 0
	const walk_with_leader = 6

	move()
	move(enum CScriptMovementAction::EInputKeys)
	move(enum CScriptMovementAction::EInputKeys, number)
	move(enum MonsterSpace::EScriptMonsterMoveAction, game_object*)
	move(enum MonsterSpace::EScriptMonsterMoveAction, patrol*)
	move(enum MonsterSpace::EScriptMonsterMoveAction, vector*)
	move(vector*, number)
	move(enum MonsterSpace::EScriptMonsterMoveAction, game_object*, number)
	move(enum MonsterSpace::EScriptMonsterMoveAction, number, vector*)
	move(enum MonsterSpace::EScriptMonsterMoveAction, patrol*, number)
	move(enum MonsterSpace::EScriptMonsterMoveAction, vector*, number)
	move(enum MonsterSpace::EBodyState, enum MonsterSpace::EMovementType, enum DetailPathManager::EDetailPathType, game_object*)
	move(enum MonsterSpace::EBodyState, enum MonsterSpace::EMovementType, enum DetailPathManager::EDetailPathType, patrol*)
	move(enum MonsterSpace::EBodyState, enum MonsterSpace::EMovementType, enum DetailPathManager::EDetailPathType, vector*)
	move(enum MonsterSpace::EScriptMonsterMoveAction, game_object*, number, enum MonsterSpace::EScriptMonsterSpeedParam)
	move(enum MonsterSpace::EScriptMonsterMoveAction, number, vector*, number)
	move(enum MonsterSpace::EScriptMonsterMoveAction, patrol*, number, enum MonsterSpace::EScriptMonsterSpeedParam)
	move(enum MonsterSpace::EScriptMonsterMoveAction, vector*, number, enum MonsterSpace::EScriptMonsterSpeedParam)
	move(enum MonsterSpace::EBodyState, enum MonsterSpace::EMovementType, enum DetailPathManager::EDetailPathType, game_object*, number)
	move(enum MonsterSpace::EBodyState, enum MonsterSpace::EMovementType, enum DetailPathManager::EDetailPathType, patrol*, number)
	move(enum MonsterSpace::EBodyState, enum MonsterSpace::EMovementType, enum DetailPathManager::EDetailPathType, vector*, number)

	function body(enum MonsterSpace::EBodyState)
	function completed()
	function input(enum CScriptMovementAction::EInputKeys)
	function move(enum MonsterSpace::EMovementType)
	function object(game_object*)
	function path(enum DetailPathManager::EDetailPathType)
	function patrol(const class CPatrolPath*, const class shared_str&)
	function position(const vector&)

class net_packet
	net_packet()

	function r_advance(number)
	function r_angle16(number&)
	function r_angle8(number&)
	function r_begin(net_packet*, number)
	function r_bool(net_packet*)
	function r_clientID(net_packet*)
	function r_dir(vector&)
	function r_elapsed()
	function r_eof(net_packet*)
	function r_float()
	function r_float(number&)
	function r_float_q16(number&, number, number)
	function r_float_q8(number&, number, number)
	function r_matrix(matrix&)
	function r_s16()
	function r_s16(number&)
	function r_s32()
	function r_s32(number&)
	function r_s64()
	function r_s64(__int64&)
	function r_s8()
	function r_s8(signed char&)
	function r_sdir(vector&)
	function r_seek(number)
	function r_stringZ(net_packet*)
	function r_tell()
	function r_u16()
	function r_u16(number&)
	function r_u32()
	function r_u32(number&)
	function r_u64()
	function r_u64(unsigned __int64&)
	function r_u8()
	function r_u8(number&)
	function r_vec3(vector&)
	function w_angle16(number)
	function w_angle8(number)
	function w_begin(number)
	function w_bool(net_packet*, boolean)
	function w_chunk_close16(number)
	function w_chunk_close8(number)
	function w_chunk_open16(number&)
	function w_chunk_open8(number&)
	function w_clientID(ClientID&)
	function w_dir(const vector&)
	function w_float(number)
	function w_float_q16(number, number, number)
	function w_float_q8(number, number, number)
	function w_matrix(matrix&)
	function w_s16(number)
	function w_s32(number)
	function w_s64(__int64)
	function w_sdir(const vector&)
	function w_stringZ(string)
	function w_tell()
	function w_u16(number)
	function w_u32(number)
	function w_u64(unsigned __int64)
	function w_u8(number)
	function w_vec3(const vector&)

class noise
	noise()
	noise(number, number, number)

	property fps
	property grain
	property intensity

	function set(number, number, number)

class not_yet_visible_object
	property value

	function object(const not_yet_visible_object&)

class object
	const activate = 16
	const aim1 = 4
	const aim2 = 5
	const deactivate = 17
	const drop = 11
	const dummy = -1
	const fire1 = 6
	const fire2 = 8
	const hide = 22
	const idle = 9
	const reload = 2
	const reload1 = 2
	const reload2 = 3
	const show = 21
	const strap = 10
	const switch1 = 0
	const switch2 = 1
	const take = 23
	const turn_off = 20
	const turn_on = 19
	const use = 18

	object()
	object(enum MonsterSpace::EObjectAction)
	object(game_object*, enum MonsterSpace::EObjectAction)
	object(string, enum MonsterSpace::EObjectAction)
	object(game_object*, enum MonsterSpace::EObjectAction, number)

	function action(enum MonsterSpace::EObjectAction)
	function completed()
	function object(game_object*)
	function object(string)

class object_binder
	object_binder(game_object*)

	property object

	function load(reader*)
	function net_destroy()
	function net_export(net_packet*)
	function net_import(net_packet*)
	function net_Relcase(game_object*)
	function net_save_relevant()
	function net_spawn(cse_alife_object*)
	function reinit()
	function reload(string)
	function save(net_packet*)
	function update(number)

class object_factory
	function register(string, string, string)
	function register(string, string, string, string)

class object_params
	property level_vertex
	property position

class particle
	particle()
	particle(string, const particle_params&)
	particle(string, string)
	particle(string, const particle_params&, boolean)
	particle(string, string, const particle_params&)
	particle(string, string, const particle_params&, boolean)

	function completed()
	function set_angles(const vector&)
	function set_bone(string)
	function set_particle(string, boolean)
	function set_position(const vector&)
	function set_velocity(const vector&)

class particle_params
	particle_params()
	particle_params(const vector&)
	particle_params(const vector&, const vector&)
	particle_params(const vector&, const vector&, const vector&)

class particles_object
	particles_object(string)

	function last_position() const
	function load_path(string)
	function looped() const
	function move_to(const vector&, const vector&)
	function pause_path(boolean)
	function play()
	function play_at_pos(const vector&)
	function playing() const
	function set_direction(const vector&)
	function set_orientation(number, number, number)
	function start_path(boolean)
	function stop()
	function stop_deffered()
	function stop_path()

class Patch_Dawnload_Progress
	function GetFlieName()
	function GetInProgress()
	function GetProgress()
	function GetStatus()

class patrol
	const continue = 1
	const custom = 3
	const dummy = -1
	const nearest = 2
	const next = 4
	const start = 0
	const stop = 0

	patrol(string)
	patrol(string, enum PatrolPathManager::EPatrolStartType)
	patrol(string, enum PatrolPathManager::EPatrolStartType, enum PatrolPathManager::EPatrolRouteType)
	patrol(string, enum PatrolPathManager::EPatrolStartType, enum PatrolPathManager::EPatrolRouteType, boolean)
	patrol(string, enum PatrolPathManager::EPatrolStartType, enum PatrolPathManager::EPatrolRouteType, boolean, number)

	function count() const
	function flag(number, number) const
	function flags(number) const
	function game_vertex_id(number) const
	function get_nearest(const vector&) const
	function index(string) const
	function level_vertex_id(number) const
	function name(number) const
	function point(const patrol*, number)
	function terminal(number) const

class physics_element
	function apply_force(number, number, number)
	function fix()
	function get_angular_vel(vector&) const
	function get_density()
	function get_linear_vel(vector&) const
	function get_mass()
	function get_volume()
	function global_transform(physics_element*)
	function is_breakable()
	function is_fixed()
	function release_fixed()

class physics_joint
	function get_anchor(vector&)
	function get_axes_number()
	function get_axis_angle(number)
	function get_axis_dir(number, vector&)
	function get_bone_id()
	function get_first_element()
	function get_limits(number&, number&, number)
	function get_max_force_and_velocity(number&, number&, number)
	function get_stcond_element()
	function is_breakable()
	function set_anchor_global(number, number, number)
	function set_anchor_vs_first_element(number, number, number)
	function set_anchor_vs_second_element(number, number, number)
	function set_axis_dir_global(number, number, number, number)
	function set_axis_dir_vs_first_element(number, number, number, number)
	function set_axis_dir_vs_second_element(number, number, number, number)
	function set_axis_spring_dumping_factors(number, number, number)
	function set_joint_spring_dumping_factors(number, number)
	function set_limits(number, number, number)
	function set_max_force_and_velocity(number, number, number)

class physics_shell
	function apply_force(number, number, number)
	function block_breaking()
	function get_angular_vel(vector&) const
	function get_element_by_bone_id(number)
	function get_element_by_bone_name(string)
	function get_element_by_order(number)
	function get_elements_number()
	function get_joint_by_bone_id(number)
	function get_joint_by_bone_name(string)
	function get_joint_by_order(number)
	function get_joints_number()
	function get_linear_vel(vector&) const
	function is_breakable()
	function is_breaking_blocked()
	function unblock_breaking()

class physics_world
	function add_call(class CPHCondition*, class CPHAction*)
	function gravity()
	function set_gravity(number)

class planner_action (action_planner, action_base)
	planner_action()
	planner_action(game_object*)
	planner_action(game_object*, string)

	property object
	property storage

	function action(const number&)
	function actual(const action_planner*)
	function add_action(const number&, action_base*)
	function add_effect(const world_property&)
	function add_evaluator(const number&, property_evaluator*)
	function add_precondition(const world_property&)
	function clear()
	function current_action()
	function current_action_id() const
	function evaluator(const number&)
	function execute()
	function finalize()
	function initialize()
	function initialized() const
	function remove_action(const number&)
	function remove_effect(const number&)
	function remove_evaluator(const number&)
	function remove_precondition(const number&)
	function set_goal_world_state(action_planner*, world_state*)
	function set_weight(const number&)
	function setup(game_object*)
	function setup(game_object*, property_storage*)
	function show(string)
	function update()
	function weight(const world_state&, const world_state&) const

class profile
	function online() const
	function unique_nick() const

class profile_store
	const at_award_massacre = 0
	const at_awards_count = 30
	const bst_backstabs_in_row = 2
	const bst_bleed_kills_in_row = 2
	const bst_explosive_kills_in_row = 3
	const bst_eye_kills_in_row = 4
	const bst_head_shots_in_row = 3
	const bst_kills_in_row = 0
	const bst_kinife_kills_in_row = 1
	const bst_score_types_count = 7

	function get_awards()
	function get_best_scores()
	function load_current_profile(store_operation_cb, store_operation_cb)
	function stop_loading()

class profile_timer
	profile_timer()
	profile_timer(profile_timer&)

	operator +(const profile_timer&, profile_timer)
	operator <(const profile_timer&, profile_timer)

	function __tostring(profile_timer&)
	function start()
	function stop()
	function time() const

class property_evaluator
	property_evaluator()
	property_evaluator(game_object*)
	property_evaluator(game_object*, string)

	property object
	property storage

	function evaluate()
	function setup(game_object*, property_storage*)

class property_evaluator_const (property_evaluator)
	property_evaluator_const(boolean)

	property object
	property storage

	function evaluate()
	function setup(game_object*, property_storage*)

class property_storage
	property_storage()

	function property(const number&) const
	function set_property(const number&, const boolean&)

class ray_pick
	ray_pick()
	ray_pick(vector&, vector&, number, enum collide::rq_target, game_object*)

	function get_distance()
	function get_element()
	function get_object()
	function get_result()
	function query()
	function set_direction(vector&)
	function set_flags(enum collide::rq_target)
	function set_ignore_object(game_object*)
	function set_position(vector&)
	function set_range(number)

class reader
	function r_advance(number)
	function r_angle16()
	function r_angle8()
	function r_bool(reader*)
	function r_dir(vector&)
	function r_elapsed() const
	function r_eof(reader*)
	function r_float()
	function r_float(number&)
	function r_float_q16(number, number)
	function r_float_q8(number, number)
	function r_s16()
	function r_s16(number&)
	function r_s32()
	function r_s32(number&)
	function r_s64()
	function r_s64(__int64&)
	function r_s8()
	function r_s8(signed char&)
	function r_sdir(vector&)
	function r_seek(number)
	function r_stringZ(reader*)
	function r_tell() const
	function r_u16()
	function r_u16(number&)
	function r_u32()
	function r_u32(number&)
	function r_u64()
	function r_u64(unsigned __int64&)
	function r_u8()
	function r_u8(number&)
	function r_vec3(reader*, vector*)

class render_device
	property aspect_ratio
	property cam_dir
	property cam_pos
	property cam_right
	property cam_top
	property f_time_delta
	property fov
	property frame
	property height
	property precache_frame
	property time_delta
	property width

	function is_paused(render_device*)
	function pause(render_device*, boolean)
	function time_global(const render_device*)

class rotation
	property pitch
	property yaw

class rq_result
	rq_result()

	property element
	property object
	property range

class rq_target
	const rqtBoth = 3
	const rqtDyn = 13
	const rqtNone = 0
	const rqtObject = 1
	const rqtObstacle = 8
	const rqtShape = 4
	const rqtStatic = 2

class rtoken_list
	rtoken_list()

	function add(string)
	function clear()
	function count()
	function get(number)
	function remove(number)

class SBooster
	SBooster()

	property fBoostTime
	property fBoostValue
	property m_type

class SCartridgeParam
	SCartridgeParam()

	property buckShot
	property impair
	property k_cam_dispersion
	property u8ColorID

	function Init()

class smart_cover_object (CGameObject)
	smart_cover_object()

	function _construct()
	function getEnabled() const
	function getVisible() const
	function net_Export(net_packet&)
	function net_Import(net_packet&)
	function net_Spawn(cse_abstract*)
	function use(CGameObject*)
	function Visual() const

class snd_type
	const ambient = 128
	const anomaly = 268435456
	const anomaly_idle = 268437504
	const attack = 8192
	const bullet_hit = 524288
	const die = 131072
	const drop = 33554432
	const eat = 4096
	const empty = 1048576
	const hide = 16777216
	const idle = 2048
	const injure = 65536
	const item = 1073741824
	const item_drop = 1107296256
	const item_hide = 1090519040
	const item_pick_up = 1140850688
	const item_take = 1082130432
	const item_use = 1077936128
	const monster = 536870912
	const monster_attack = 536879104
	const monster_die = 537001984
	const monster_eat = 536875008
	const monster_injure = 536936448
	const monster_step = 536903680
	const monster_talk = 536887296
	const no_sound = 0
	const object_break = 1024
	const object_collide = 512
	const object_explode = 256
	const pick_up = 67108864
	const reload = 262144
	const shoot = 2097152
	const step = 32768
	const take = 8388608
	const talk = 16384
	const use = 4194304
	const weapon = -2147483648
	const weapon_bullet_hit = -2146959360
	const weapon_empty = -2146435072
	const weapon_reload = -2147221504
	const weapon_shoot = -2145386496
	const world = 134217728
	const world_ambient = 134217856
	const world_object_break = 134218752
	const world_object_collide = 134218240
	const world_object_explode = 134217984

class sound
	const attack = 3
	const attack_hit = 4
	const die = 7
	const eat = 2
	const idle = 1
	const panic = 11
	const steal = 10
	const take_damage = 5
	const threaten = 9

	sound()
	sound(enum MonsterSound::EType)
	sound(enum MonsterSound::EType, number)
	sound(sound_object*, vector*)
	sound(string, string)
	sound(string, vector*)
	sound(sound_object*, string, const vector&)
	sound(sound_object*, vector*, const vector&)
	sound(string, string, const vector&)
	sound(string, string, enum MonsterSpace::EMonsterHeadAnimType)
	sound(string, vector*, const vector&)
	sound(sound_object*, string, const vector&, const vector&)
	sound(sound_object*, vector*, const vector&, boolean)
	sound(string, string, const vector&, const vector&)
	sound(string, vector*, const vector&, boolean)
	sound(sound_object*, string, const vector&, const vector&, boolean)
	sound(string, string, const vector&, const vector&, boolean)

	function completed()
	function set_angles(const vector&)
	function set_bone(string)
	function set_position(const vector&)
	function set_sound(const sound_object&)
	function set_sound(string)
	function set_sound_type(enum ESoundTypes)

class sound_memory_object (game_memory_object)
	property last_level_time
	property level_time
	property object_info
	property power
	property self_info

	function object(const game_memory_object&)
	function type() const

class sound_object
	const looped = 1
	const s2d = 2
	const s3d = 0

	sound_object(string)
	sound_object(string, enum ESoundTypes)

	property frequency
	property max_distance
	property min_distance
	property volume

	function attach_tail(string)
	function get_position() const
	function length()
	function play(game_object*)
	function play(game_object*, number)
	function play(game_object*, number, number)
	function play_at_pos(game_object*, const vector&)
	function play_at_pos(game_object*, const vector&, number)
	function play_at_pos(game_object*, const vector&, number, number)
	function play_no_feedback(game_object*, number, number, vector, number)
	function playing() const
	function set_position(const vector&)
	function stop()
	function stop_deffered()

class sound_params
	property frequency
	property max_distance
	property min_distance
	property position
	property volume

class SoundInfo
	property danger
	property position
	property power
	property time
	property who

class spawn_story_ids
	const INVALID_SPAWN_STORY_ID = -1

class SServerFilters
	SServerFilters()

	property empty
	property full
	property listen_servers
	property with_pass
	property without_ff
	property without_pass

class stalker_ids
	const action_accomplish_task = 7
	const action_aim_enemy = 16
	const action_alife_planner = 88
	const action_anomaly_planner = 90
	const action_combat_planner = 89
	const action_communicate_with_customer = 9
	const action_critically_wounded = 36
	const action_danger_by_sound_planner = 73
	const action_danger_grenade_look_around = 85
	const action_danger_grenade_planner = 72
	const action_danger_grenade_search = 86
	const action_danger_grenade_take_cover = 82
	const action_danger_grenade_take_cover_after_explosion = 84
	const action_danger_grenade_wait_for_explosion = 83
	const action_danger_in_direction_detour = 80
	const action_danger_in_direction_hold_position = 79
	const action_danger_in_direction_look_out = 78
	const action_danger_in_direction_planner = 71
	const action_danger_in_direction_search = 81
	const action_danger_in_direction_take_cover = 77
	const action_danger_planner = 91
	const action_danger_unknown_look_around = 75
	const action_danger_unknown_planner = 70
	const action_danger_unknown_search = 76
	const action_danger_unknown_take_cover = 74
	const action_dead = 0
	const action_death_planner = 87
	const action_detour_enemy = 25
	const action_dying = 1
	const action_find_ammo = 15
	const action_find_item_to_kill = 13
	const action_gather_items = 2
	const action_get_distance = 24
	const action_get_item_to_kill = 12
	const action_get_ready_to_kill = 17
	const action_hold_position = 23
	const action_in_smart_cover = 46
	const action_kill_enemy = 19
	const action_kill_enemy_if_not_visible = 29
	const action_kill_if_enemy_critically_wounded = 37
	const action_kill_if_player_on_the_path = 35
	const action_kill_wounded_enemy = 33
	const action_look_out = 22
	const action_make_item_killing = 14
	const action_no_alife = 3
	const action_post_combat_wait = 34
	const action_prepare_wounded_enemy = 32
	const action_reach_customer_location = 8
	const action_reach_task_location = 6
	const action_reach_wounded_enemy = 30
	const action_retreat_from_enemy = 20
	const action_script = 92
	const action_search_enemy = 26
	const action_smart_terrain_task = 4
	const action_solve_zone_puzzle = 5
	const action_sudden_attack = 28
	const action_take_cover = 21
	const detect_anomaly = 11
	const get_out_of_anomaly = 10
	const property_alife = 3
	const property_alive = 0
	const property_already_dead = 2
	const property_anomaly = 46
	const property_cover_actual = 42
	const property_cover_reached = 43
	const property_critically_wounded = 29
	const property_danger = 8
	const property_danger_by_sound = 41
	const property_danger_grenade = 40
	const property_danger_in_direction = 39
	const property_danger_unknown = 38
	const property_dead = 1
	const property_enemy = 7
	const property_enemy_critically_wounded = 30
	const property_enemy_detoured = 21
	const property_found_ammo = 12
	const property_found_item_to_kill = 10
	const property_grenade_exploded = 45
	const property_in_cover = 18
	const property_in_smart_cover = 50
	const property_inside_anomaly = 47
	const property_item_can_kill = 11
	const property_item_to_kill = 9
	const property_items = 6
	const property_looked_around = 44
	const property_looked_out = 19
	const property_panic = 17
	const property_position_holded = 20
	const property_pure_enemy = 23
	const property_puzzle_solved = 4
	const property_ready_to_detour = 14
	const property_ready_to_kill = 13
	const property_script = 74
	const property_see_enemy = 15
	const property_smart_terrain_task = 5
	const property_use_crouch_to_look_out = 24
	const property_use_suddenness = 22
	const sound_alarm = 4
	const sound_attack_allies_several_enemies = 7
	const sound_attack_allies_single_enemy = 6
	const sound_attack_no_allies = 5
	const sound_backup = 8
	const sound_detour = 9
	const sound_die = 0
	const sound_die_in_anomaly = 1
	const sound_enemy_critically_wounded = 24
	const sound_enemy_killed_or_wounded = -805289984
	const sound_enemy_lost_no_allies = 12
	const sound_enemy_lost_with_allies = 13
	const sound_friendly_grenade_alarm = 20
	const sound_grenade_alarm = 19
	const sound_humming = 3
	const sound_injuring = 2
	const sound_injuring_by_friend = 14
	const sound_kill_wounded = 23
	const sound_need_backup = 21
	const sound_panic_human = 15
	const sound_panic_monster = 16
	const sound_running_in_danger = 22
	const sound_script = 27
	const sound_search1_no_allies = 11
	const sound_search1_with_allies = 10
	const sound_tolls = 17
	const sound_wounded = 18

class StaticDrawableWrapper
	property m_endTime

	function wnd()

class store_operation_cb
	store_operation_cb()
	store_operation_cb(object, function<void>)

	function bind(object, function<void>)
	function clear()

class story_ids
	const brain_snork_growl = 923
	const esc_trader = 235
	const hosp_enter_katacomb = 231
	const hosp_level_changer_to_stancia_2 = 234
	const hosp_z3_csky_squad_help = 233
	const hosp_z3_hold_out = 232
	const Invalid = 65535
	const INVALID_STORY_ID = -1
	const kat_hosp_safe_restr_2 = 210
	const kat_hosp_safe_restr_3 = 211
	const kat_hosp_safe_restr_4 = 212
	const kat_hosp_safe_restr_5 = 213
	const kat_hosp_safe_restr_6 = 214
	const kat_hosp_z1_cs_commander = 204
	const kat_hosp_z1_cs_grenadier = 205
	const kat_hosp_z1_cs_solder_1 = 206
	const kat_hosp_z1_cs_solder_2 = 207
	const kat_hosp_z1_door_2 = 202
	const kat_hosp_z1_grenade = 208
	const kat_hosp_z1_sniper = 200
	const kat_hosp_z1_sniper_off_restr = 203
	const kat_hosp_z1_sniper_target = 201
	const kat_hosp_z2_destroyable_1 = 218
	const kat_hosp_z2_destroyable_10 = 227
	const kat_hosp_z2_destroyable_2 = 219
	const kat_hosp_z2_destroyable_3 = 220
	const kat_hosp_z2_destroyable_4 = 221
	const kat_hosp_z2_destroyable_5 = 222
	const kat_hosp_z2_destroyable_6 = 223
	const kat_hosp_z2_destroyable_7 = 224
	const kat_hosp_z2_destroyable_8 = 225
	const kat_hosp_z2_destroyable_9 = 226
	const kat_hosp_z2_door = 215
	const kat_hosp_z2_explode_1 = 228
	const kat_hosp_z2_explode_2 = 229
	const kat_hosp_z2_explode_3 = 230
	const kat_hosp_z2_helicopter = 209
	const kat_hosp_z2_level_exit_restr = 217
	const kat_hosp_z3_door = 216
	const test_01 = 65000
	const test_02 = 65001
	const test_03 = 65002
	const test_04 = 65003
	const test_05 = 65004
	const yan_ecolog_help = 900
	const yan_exit_to_rostok = 925
	const yan_general_ecolog = 902
	const yan_ghost = 918
	const yan_giant_growl = 924
	const yan_helmet = 920
	const yan_labx16_documents = 917
	const yan_labx16switcher_lamp1 = 908
	const yan_labx16switcher_lamp2 = 909
	const yan_labx16switcher_lamp3 = 910
	const yan_labx16switcher_primary_1_green = 911
	const yan_labx16switcher_primary_1_red = 912
	const yan_labx16switcher_primary_2_green = 913
	const yan_labx16switcher_primary_2_red = 914
	const yan_labx16switcher_primary_3_green = 915
	const yan_labx16switcher_primary_3_red = 916
	const yan_mapspot_from_vasilyev = 907
	const yan_scan_psi_radiation = 901
	const yan_secret_exit = 921
	const yan_semenov_help = 905
	const yan_specnaz_corpse = 922
	const yan_vasilyev = 903
	const yan_zombie_zone_mapspot = 906

class suggest_nicks_cb
	suggest_nicks_cb()
	suggest_nicks_cb(object, function<void>)

	function bind(object, function<void>)
	function clear()

class task
	const additional = 1
	const completed = 2
	const fail = 0
	const in_progress = 1
	const storyline = 0
	const task_dummy = 65535

class token
	token()

	property id
	property name

class token_list
	token_list()

	function add(string, number)
	function clear()
	function id(string)
	function name(number)
	function remove(string)

class ui_events
	const BUTTON_CLICKED = 17
	const BUTTON_DOWN = 18
	const CHECK_BUTTON_RESET = 21
	const CHECK_BUTTON_SET = 20
	const EDIT_TEXT_COMMIT = 76
	const LIST_ITEM_CLICKED = 35
	const LIST_ITEM_SELECT = 36
	const MAIN_MENU_RELOADED = 81
	const MESSAGE_BOX_CANCEL_CLICKED = 44
	const MESSAGE_BOX_COPY_CLICKED = 45
	const MESSAGE_BOX_NO_CLICKED = 43
	const MESSAGE_BOX_OK_CLICKED = 39
	const MESSAGE_BOX_QUIT_GAME_CLICKED = 42
	const MESSAGE_BOX_QUIT_WIN_CLICKED = 41
	const MESSAGE_BOX_YES_CLICKED = 40
	const PROPERTY_CLICKED = 38
	const RADIOBUTTON_SET = 22
	const SCROLLBAR_HSCROLL = 32
	const SCROLLBAR_VSCROLL = 31
	const SCROLLBOX_MOVE = 30
	const TAB_CHANGED = 19
	const TRACK_VALUE_CHANGED = 83
	const WINDOW_KEY_PRESSED = 10
	const WINDOW_KEY_RELEASED = 11
	const WINDOW_KEYBOARD_CAPTURE_LOST = 14
	const WINDOW_LBUTTON_DB_CLICK = 9
	const WINDOW_LBUTTON_DOWN = 0
	const WINDOW_LBUTTON_UP = 3
	const WINDOW_MOUSE_MOVE = 6
	const WINDOW_RBUTTON_DOWN = 1
	const WINDOW_RBUTTON_UP = 4

class UIHint (CUIWindow)
	UIHint()

	function AttachChild(CUIWindow*)
	function DetachChild(CUIWindow*)
	function Enable(boolean)
	function FocusReceiveTime() const
	function GetAbsoluteRect(Frect&)
	function GetHeight() const
	function GetHintText() const
	function GetWidth() const
	function GetWndPos(CUIWindow*)
	function IsAutoDelete()
	function IsCursorOverWindow() const
	function IsEnabled()
	function IsShown()
	function ResetPPMode()
	function SetAutoDelete(boolean)
	function SetHeight(number)
	function SetHintText(string)
	function SetPPMode()
	function SetWidth(number)
	function SetWindowName(string)
	function SetWndPos(vector2)
	function SetWndRect(Frect)
	function SetWndSize(vector2)
	function Show(boolean)
	function WindowName()

class vector
	vector()

	property x
	property y
	property z

	function abs(const vector&)
	function add(const vector&)
	function add(number)
	function add(const vector&, const vector&)
	function add(const vector&, number)
	function align()
	function average(const vector&)
	function average(const vector&, const vector&)
	function clamp(const vector&)
	function clamp(const vector&, const vector&)
	function crossproduct(const vector&, const vector&)
	function distance_to(const vector&) const
	function distance_to_sqr(const vector&) const
	function distance_to_xz(const vector&) const
	function div(const vector&)
	function div(number)
	function div(const vector&, const vector&)
	function div(const vector&, number)
	function dotproduct(const vector&) const
	function getH() const
	function getP() const
	function inertion(const vector&, number)
	function invert()
	function invert(const vector&)
	function lerp(const vector&, const vector&, number)
	function mad(const vector&, const vector&)
	function mad(const vector&, number)
	function mad(const vector&, const vector&, const vector&)
	function mad(const vector&, const vector&, number)
	function magnitude() const
	function max(const vector&)
	function max(const vector&, const vector&)
	function min(const vector&)
	function min(const vector&, const vector&)
	function mul(const vector&)
	function mul(number)
	function mul(const vector&, const vector&)
	function mul(const vector&, number)
	function normalize()
	function normalize(const vector&)
	function normalize_safe()
	function normalize_safe(const vector&)
	function reflect(const vector&, const vector&)
	function set(const vector&)
	function set(number, number, number)
	function set_length(number)
	function setHP(number, number)
	function similar(const vector&, number) const
	function slide(const vector&, const vector&)
	function sub(const vector&)
	function sub(number)
	function sub(const vector&, const vector&)
	function sub(const vector&, number)

class vector2
	vector2()

	property x
	property y

	function set(const vector2&)
	function set(number, number)

class visible_memory_object (game_memory_object)
	property last_level_time
	property level_time
	property object_info
	property self_info

	function object(const game_memory_object&)

class world_property
	world_property(number, boolean)

	operator <(const world_property&, world_property)
	operator ==(const world_property&, world_property)

	function condition() const
	function value() const

class world_state
	world_state()
	world_state(world_state)

	operator <(const world_state&, world_state)
	operator ==(const world_state&, world_state)

	function add_property(const world_property&)
	function clear()
	function includes(const world_state&) const
	function property(const number&) const
	function remove_property(const number&)

