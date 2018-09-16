#pragma once
// XXX: Identify what parts of xrGame requires only "core", ui, ai, and finally script.
// Split up source (PCH-wise) accordingly.
// This will likely for a long time be WIP.

#include "Common/Common.hpp"

#include "xrEngine/stdafx.h" // XXX: This seems bad. PCH's are for internal (building) use.
#include "DPlay/dplay8.h"

// xrEngine src file count is ~1100.
// Comments following individual includes refers to number of times they are included in xrEngine as a whole.
//#include <assert.h> // ~440 - but it has no include guard! Perhaps that's intentional?
#include <math.h>
#include <queue> // ~360
#include "luabind/luabind.hpp" // luabind/*, almost 5000
#include "xrServerEntities/smart_cast.h" // a lot
#include "xrScriptEngine/script_space_forward.hpp" // ~765 // XXX: See to it this goes to pch_script
#include "xrScriptEngine/DebugMacros.hpp" // ~700 // XXX: See to it this goes to pch_script
#include "Common/LevelStructure.hpp" // ~730
#include "xrCommon/misc_math_types.h" // ~770
#include "xrEngine/ISheduled.h" // ~740
#include "xrCDB/ISpatial.h" // ~700
#include "xrCore/xrPool.h" // ~700
#include "xrEngine/ICollidable.h" // ~700
#include "xrEngine/IObjectPhysicsCollision.h" // ~700
#include "xrEngine/IRenderable.h" // ~700
#include "xrEngine/xr_object.h" // ~700
#include "xrEngine/PS_instance.h" // ~650
#include "xrPhysics/IPhysicsShellHolder.h" // ~640
#include "Level.h" // ~550
#include "Common/GUID.hpp" // ~530
#include "Common/object_broker.h" // ~500
#include "Common/object_cloner.h" // ~500
#include "Common/object_comparer.h" // ~500
#include "Common/object_destroyer.h" // ~500
#include "Common/object_loader.h" // ~500
#include "Common/object_saver.h" // ~500
#include "Include/xrRender/animation_blend.h" // ~500
#include "Include/xrRender/animation_motion.h" // ~500
#include "Include/xrRender/Kinematics.h" // ~360
#include "Include/xrRender/KinematicsAnimated.h" // ~500
#include "Include/xrRender/RenderVisual.h" // ~370
#include "Include/xrRender/UIRender.h" // ~450
#include "Include/xrRender/UIShader.h" // ~490
#include "xrCore/_plane2.h" // ~450
#include "xrAICore/AISpaceBase.hpp" // ~650
#include "xrAICore/Navigation/game_graph.h" // ~600
#include "xrServerEntities/xrServer_Objects.h" // ~500
#include "xrServerEntities/xrServer_Objects_ALife.h" // ~500
#include "xrServerEntities/xrServer_Objects_ALife_Items.h" // ~500
#include "xrGame/Entity.h" // ~490
#include "xrGame/damage_manager.h" // ~490
#include "xrGame/EntityCondition.h" // ~490
#include "xrGame/ui_defs.h" // ~450
#include "xrGame/entity_alive.h" // ~430
#include "xrCore/XML/XMLDocument.hpp" // ~400
#include "xrPhysics/xrPhysics.h" // ~400
#include "xrEngine/Feel_Sound.h" // ~400
#include "xrAICore/Navigation/graph_edge.h" // ~380
#include "xrAICore/Navigation/graph_abstract.h" // ~380
#include "loki/EmptyType.h" // ~380
#include "loki/NullType.h" // only ~50, so small it's OK.
#include "xrPhysics/PhysicsShell.h" // ~350
#include "xrServerEntities/ShapeData.h" // ~330
#include "xrScriptEngine/ScriptExporter.hpp" // ~330 // XXX: See to it this goes to pch_script
#include "xrServerEntities/specific_character.h" // ~330
#include "xrServerEntities/shared_data.h" // ~330
#include "xrServerEntities/xml_str_id_loader.h" // ~330
#include "xrServerEntities/character_info.h" // ~320
#include "xrServerEntities/ai_sounds.h" // ~320
#include "xrGame/attachment_owner.h" // ~320
#include "xrGame/step_manager.h" // ~370
#include "xrGame/physic_item.h" // ~330
#include "xrGame/pda_space.h" // ~330
#include "xrGame/PhraseDialogManager.h" // ~300
#include "xrGame/script_entity.h" // ~290
#include "xrEngine/Feel_Vision.h" // ~270
#include "xrGame/CustomMonster.h" // ~265
#include "xrGame/ui_base.h" // ~260
#include "xrPhysics/MathUtils.h" // ~260
#include "xrGame/WeaponAmmo.h" // ~250
#include "xrEngine/GameFont.h" // ~250
#include "xrGame/detail_path_manager.h" // ~120 + 120 for its inlines
#include "xrPhysics/MovementBoxDynamicActivate.h" // ~240
#include "xrPhysics/PHItemList.h"
#include "xrGame/ui/UIEditBox.h"
#include "xrGame/ui/UIWindow.h" // ~225
#include "xrGame/ui/UIMessages.h" // ~225  one single enum
#include "xrScriptEngine/Functor.hpp" // ~225 // XXX: See to it this goes to pch_script
#include "xrGame/fire_disp_controller.h" // ~220
#include "xrGame/Actor.h" // ~220
#include "xrEngine/CameraDefs.h" // ~210
#include "xrEngine/CameraManager.h" // ~190
#include "xrCore/PostProcess/PPInfo.hpp" // ~190
#include "xrScriptEngine/script_engine.hpp" // only ~200, VERY heavy! // XXX: See to it this goes to pch_script
//#include "xrGame/ui/UILine.h" // ~190
//#include "xrGame/ui/UILines.h" // ~190
#include "xrCore/_fbox2.h" // ~155
#include "xrCore/_vector3d_ext.h"
#include "xrCore/buffer_vector.h"
#include "xrCore/Crypto/xr_dsa_signer.h" // ~70, very heavy to compile
#include "xrCore/Crypto/xr_dsa_verifyer.h" // ~70, very heavy to compile
#include "xrServerEntities/restriction_space.h" // only ~110, so small it's worth it
#include "xrAICore/Components/condition_state.h" // only ~100, but it includes more
//#include "xrScriptEngine/script_engine.hpp" // ~210 // XXX: See to it this goes to pch_script
//#include "xrCore/dump_string.h" // ~260
//#include "xrCore/Math/Random32.hpp" // ~220
//#include "xrEngine/LightAnimLibrary.h" // ~200
#include "xrGame/UICursor.h"
#include "xrGame/UIDialogHolder.h" // ~135, somewhat heavy to compile
#include "xrGame/UIStaticItem.h" // ~190, and quite heavy to compile
#include "xrGame/ui/Restrictions.h"
#include "xrGame/ui/UI_IB_Static.h" // ~60, very heavy to compile
#include "xrGame/ui/UI3tButton.h" // ~60, very heavy to compile
#include "xrGame/ui/UIActorMenu.h"
#include "xrGame/ui/UIButton.h" // ~80, very heavy to compile
#include "xrGame/ui/UIBuyWndBase.h"
#include "xrGame/ui/UIBuyWndShared.h"
#include "xrGame/ui/UICellItem.h"
#include "xrGame/ui/UICustomEdit.h"
#include "xrGame/ui/UIDialogWnd.h" // ~100
#include "xrGame/ui/UIDragDropListEx.h"
#include "xrGame/ui/UIFrameWindow.h"
#include "xrGame/ui/UIHint.h"
#include "xrGame/ui/UIInventoryUtilities.h"
#include "xrGame/ui/UILanimController.h" // ~190
#include "xrGame/ui/UILine.h" // ~190
#include "xrGame/ui/UILines.h" // ~190
#include "xrGame/ui/UIListBox.h"
#include "xrGame/ui/UIOptionsItem.h" // ~60, somewhat heavy to compile
//#include "xrGame/ui/UIOptionsManager.h" // ~60, somewhat heavy to compile
#include "xrGame/ui/UIProgressBar.h" // ~23, very heavy to compile
#include "xrGame/ui/UIScrollView.h" // ~70, quite heavy to compile
#include "xrGame/ui/UIStatic.h" // ~190
#include "xrGame/ui/UISubLine.h" // ~190
#include "xrGame/ui/UIWndCallback.h" // 135, somewhat heavy to compile
#include "xrGame/ui/UIXmlInit.h" // ~105, somewhat heavy to compile
#include "xrGame/smart_cover.h" // ~30, VERY heavy to compile
#include "xrGame/team_hierarchy_holder.h" // ~35, but quite heavy to compile
#include "xrGame/Tracer.h" // ~60, somewhat heavy to compile
//#include "xrCore/_fbox2.h" // ~155
#include "xr_time.h" // only ~125, includes "alife_space.h" !
#include "xrServerEntities/alife_movement_manager_holder.h" // only ~120, small and self-contained
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h" // only ~120, very heavy to compile
#include "xrServerEntities/clsid_game.h" // only ~20, only macros
//#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h" // ~120
#include "xrPhysics/PHUpdateObject.h" // ~95 - includes PHItemList.h
//#include "xrPhysics/PHItemList.h" // ~110 - template class, so better include in pch
#include "xrGame/Weapon.h" // ~125, VERY heavy to compile
#include "xrGame/string_table.h" // ~100, somewhat heavy to compile
#include "xrGame/WeaponMagazined.h" // ~70, VERY heavy to compile
#include "visual_memory_manager.h" // only ~40, quite heavy to compile
#include "xrNetServer/NET_Messages.h" // only ~60, depends on dplay macros
#include "xrPhysics/DamageSource.h" // only ~65, very small
#include "xrPhysics/debug_output.h" // only ~60, quite heavy to compile
#include "xrServerEntities/alife_monster_brain.h" // only ~30, very heavy to compile
#include "xrServerEntities/object_factory.h" // only ~15, very heavy to compile
#include "xrServerEntities/xrServer_Objects_ALife_All.h" // only ~25, very heavy to compile
#include "xrServerEntities/xrServer_script_macroses.h" // only ~13, very heavy to compile
#include "xrEngine/CameraBase.h" // only ~40, pretty heavy to compile
#include "xrEngine/Effector.h" // ~80, very heavy to compile
#include "xrGame/CameraEffector.h" // ~75, very heavy to compile
#include "xrGame/alife_simulator.h" // ~80
#include "xrGame/alife_update_manager.h"
#include "xrGame/ammunition_groups.h"
#include "xrGame/animation_utils.h"
#include "xrGame/Artefact.h" // ~50
#include "xrGame/character_hit_animations.h"
#include "xrGame/character_shell_control.h" // ~65
#include "xrGame/CharacterPhysicsSupport.h" // ~65
#include "xrGame/danger_object.h"
#include "xrGame/Inventory.h" // ~120
#include "xrGame/kills_store.h"
#include "xrGame/MainMenu.h"
#include "xrGame/map_location.h"
#include "xrGame/map_location_defs.h"
#include "xrGame/member_order.h"
#include "xrGame/memory_manager.h"
#include "xrGame/Missile.h"
#include "xrGame/movement_manager_space.h"
#include "xrGame/object_manager.h"
#include "xrGame/PHDebug.h"
#include "xrGame/PHDestroyable.h"
#include "xrGame/PHDestroyableNotificate.h"
#include "xrGame/PHSkeleton.h"
#include "xrGame/property_evaluator.h"
#include "xrGame/property_evaluator_const.h"
#include "xrGame/quadtree.h"
#include "xrGame/Random.hpp" // ~150
#include "xrGame/script_callback_ex.h" // ~120, VERY heavy to compile
#include "xrGame/seniority_hierarchy_holder.h"
#include "xrGame/setup_manager.h"
#include "xrGame/sight_action.h"
#include "xrGame/sight_manager.h"
#include "xrGame/smart_cover.h"
#include "xrGame/smart_cover_animation_planner.h"
#include "xrGame/sound_player.h"
#include "xrGame/space_restrictor.h"
#include "xrGame/squad_hierarchy_holder.h"
#include "xrGame/stalker_animation_manager.h"
#include "xrGame/stalker_animation_pair.h"
#include "xrGame/stalker_animation_script.h"
#include "xrGame/stalker_planner.h"
#include "xrGame/static_obstacles_avoider.h" // ~60, VERY heavy to compile
#include "xrGame/stalker_movement_manager_obstacles.h" // ~60, INCREDIBLY heavy to compile
#include "xrGame/stalker_movement_manager_smart_cover.h" // ~60, INCREDIBLY heavy to compile
#include "xrGame/stalker_base_action.h" // ~30, INCREDIBLY heavy to compile
#include "xrGame/stalker_decision_space.h" // ~60, only enum's
#include "xrGame/Spectator.h" // ~60, INCREDIBLY heavy to compile
#include "xrGame/game_cl_mp.h" // ~60, INCREDIBLY heavy to compile
#include "xrGame/game_events_handler.h" // ~30, INCREDIBLY heavy to compile
#include "xrGame/steering_behaviour.h" // ~30, measurable compile time
#include "xrGame/UIGameCustom.h" // ~85, quite heavy to compile
#include "xrGame/UIGameMP.h"
#include "xrGame/UIGameSP.h"
#include "xrGame/wallmark_manager.h" // ~60, measurable compile time
#include "xrGame/WeaponCustomPistol.h" // ~30, INCREDIBLY heavy to compile
#include "xrGame/wrapper_abstract.h"
#include "xrGame/ai/monsters/ai_monster_utils.h" // ~130, quite heavy to compile
#include "xrGame/ai/monsters/control_animation.h"
#include "xrGame/ai/monsters/control_animation_base.h"
#include "xrGame/ai/monsters/control_com_defs.h"
#include "xrGame/ai/monsters/control_combase.h" // ~130
#include "xrGame/ai/monsters/control_direction.h" // ~130
#include "xrGame/ai/monsters/control_jump.h" // ~130
#include "xrGame/ai/monsters/control_manager.h" // ~130
#include "xrGame/ai/monsters/control_manager_custom.h" // ~130
#include "xrGame/ai/monsters/control_melee_jump.h" // ~130
#include "xrGame/ai/monsters/control_movement.h" // ~130
#include "xrGame/ai/monsters/control_path_builder.h" // ~130
#include "xrGame/ai/monsters/monster_corpse_manager.h" // ~130
#include "xrGame/ai/stalker/ai_stalker.h"
#include "Include/xrRender/WallMarkArray.h" // ~80
#include "xrAICore/Navigation/ai_object_location.h" // ~95, very heavy to compile
#include "xrAICore/Navigation/graph_engine.h" // ~80, VERY heavy to compile
#include "xrAICore/Navigation/PatrolPath/patrol_path.h" // ~80, VERY heavy to compile
#ifdef DEBUG
#include "Include/xrRender/DebugRender.h"
#endif
