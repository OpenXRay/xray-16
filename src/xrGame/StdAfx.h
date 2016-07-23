#pragma once
// XXX: Identify what parts of xrGame requires only "core", ui, and finally script.
// Split up source (PCH-wise) accordingly.
// This will likely for a long time be WIP.

#include "Common/Common.hpp"

#include "xrEngine/stdafx.h" // XXX: This seems bad. PCH's are for internal (building) use.
#include "DPlay/dplay8.h"

// xrEngine src file count is ~1100.
// Comments following individual includes refers to number of times they are included in xrEngine as a whole.
#include <assert.h> // ~440 - but it has no include guard! Perhaps that's intentional?
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
#include "xrAICore/Navigation/graph_edge.h" // ~380
#include "xrAICore/Navigation/graph_abstract.h" // ~380
#include "xrPhysics/xrPhysics.h" // ~400
#include "loki/EmptyType.h" // ~380
#include "loki/NullType.h" // only ~50, but so small it's cool.
#include "xrPhysics/PhysicsShell.h" // ~350
#include "xrServerEntities/ShapeData.h" // ~330
#include "xrScriptEngine/ScriptExporter.hpp" // ~330 // XXX: See to it this goes to pch_script
#include "xrServerEntities/specific_character.h" // ~330
#include "xrServerEntities/shared_data.h" // ~330
#include "xrServerEntities/xml_str_id_loader.h" // ~330
#include "xrServerEntities/character_info.h" // ~320
#include "xrServerEntities/ai_sounds.h" // ~320
#include "xrCore/XML/XMLDocument.hpp" // ~400
#include "xrGame/step_manager.h" // ~370
#include "xrGame/physic_item.h" // ~330
#include "xrGame/script_entity.h" // ~290
#include "xrPhysics/MathUtils.h" // ~260
#include "xrGame/WeaponAmmo.h" // ~250
#include "xrPhysics/MovementBoxDynamicActivate.h" // ~240
#include "xrScriptEngine/functor.hpp" // ~225 // XXX: See to it this goes to pch_script
#include "xrScriptEngine/script_engine.hpp" // only ~200, but VERY heavy! // XXX: See to it this goes to pch_script
#include "xrServerEntities/restriction_space.h" // only ~110, but so small it's worth it
#include "xrAICore/Components/condition_state.h" // only ~100, but it includes more
//#include "xrScriptEngine/script_engine.hpp" // ~210 // XXX: See to it this goes to pch_script
//#include "xrCore/dump_string.h" // ~260
//#include "xrCore/Math/Random32.hpp" // ~220
//#include "xrEngine/LightAnimLibrary.h" // ~200
//#include "xrCore/_fbox2.h" // ~155
//#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h" // ~120
