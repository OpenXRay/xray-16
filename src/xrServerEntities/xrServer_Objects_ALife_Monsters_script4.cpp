////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Monsters_script4.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server monsters for ALife simulator, script export, the second part
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer_script_macroses.h"
#include "alife_human_brain.h"
#ifdef XRGAME_EXPORTS
#include "relation_registry.h"
#endif // #ifdef XRGAME_EXPORTS
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

CALifeMonsterBrain* monster_brain(CSE_ALifeMonsterAbstract* monster)
{
    THROW(monster);
    return (&monster->brain());
}

CALifeHumanBrain* human_brain(CSE_ALifeHumanAbstract* human)
{
    THROW(human);
    return (&human->brain());
}

void clear_smart_terrain(CSE_ALifeMonsterAbstract* monster)
{
    THROW(monster);
    monster->m_smart_terrain_id = 0xffff;
}

void smart_terrain_task_activate(CSE_ALifeMonsterAbstract* monster)
{
    THROW(monster);
    monster->m_task_reached = true;
}

void smart_terrain_task_deactivate(CSE_ALifeMonsterAbstract* monster)
{
    THROW(monster);
    monster->m_task_reached = false;
}

ALife::_OBJECT_ID smart_terrain_id(CSE_ALifeMonsterAbstract* monster)
{
    THROW(monster);
    return (monster->m_smart_terrain_id);
}
#ifdef XRGAME_EXPORTS
float travel_speed(CSE_ALifeMonsterAbstract* self) { return (self->m_fGoingSpeed); }
void travel_speed2(CSE_ALifeMonsterAbstract* self, float travel_speed) { self->m_fGoingSpeed = travel_speed; }
float current_level_travel_speed(CSE_ALifeMonsterAbstract* self) { return (self->m_fCurrentLevelGoingSpeed); }
void current_level_travel_speed2(CSE_ALifeMonsterAbstract* self, float travel_speed)
{
    self->m_fCurrentLevelGoingSpeed = travel_speed;
}

void ForceSetGoodwill(CSE_ALifeMonsterAbstract* self, int goodwill, ALife::_OBJECT_ID pWhoToSet)
{
    RELATION_REGISTRY().ForceSetGoodwill(self->ID, pWhoToSet, goodwill);
}
#endif // #ifdef XRGAME_EXPORTS

static void CSE_ALifeMonsterAbstract_Export(lua_State* luaState)
{
    module(luaState)[luabind_class_monster2(
        CSE_ALifeMonsterAbstract, "cse_alife_monster_abstract", CSE_ALifeCreatureAbstract, CSE_ALifeSchedulable)
                         .def("smart_terrain_id", &smart_terrain_id)
                         .def_readonly("group_id", &CSE_ALifeMonsterAbstract::m_group_id)
                         .def_readwrite("m_smart_terrain_id", &CSE_ALifeMonsterAbstract::m_smart_terrain_id)
                         .def("clear_smart_terrain", &clear_smart_terrain)
                         .def("brain", &monster_brain)
                         .def("rank", &CSE_ALifeMonsterAbstract::Rank)
                         .def("smart_terrain_task_activate", &smart_terrain_task_activate)
                         .def("smart_terrain_task_deactivate", &smart_terrain_task_deactivate)
#ifdef XRGAME_EXPORTS
                         .def("travel_speed", &travel_speed)
                         .def("travel_speed", &travel_speed2)
                         .def("current_level_travel_speed", &current_level_travel_speed)
                         .def("current_level_travel_speed", &current_level_travel_speed2)
                         .def("kill", &CSE_ALifeMonsterAbstract::kill)
                         .def("has_detector", &CSE_ALifeMonsterAbstract::has_detector)
                         .def("force_set_goodwill", &ForceSetGoodwill)
#endif
    ];
}
SCRIPT_EXPORT_FUNC(
    CSE_ALifeMonsterAbstract, (CSE_ALifeCreatureAbstract, CSE_ALifeSchedulable), CSE_ALifeMonsterAbstract_Export);

static void CSE_ALifeHumanAbstract_Export(lua_State* luaState)
{
    module(luaState)[luabind_class_monster2(
        CSE_ALifeHumanAbstract, "cse_alife_human_abstract", CSE_ALifeTraderAbstract, CSE_ALifeMonsterAbstract)
                         .def("brain", &human_brain)
#ifdef XRGAME_EXPORTS
                         .def("rank", &CSE_ALifeTraderAbstract::Rank)
                         .def("set_rank", &CSE_ALifeTraderAbstract::SetRank)
#endif
    ];
}

SCRIPT_EXPORT_FUNC(
    CSE_ALifeHumanAbstract, (CSE_ALifeTraderAbstract, CSE_ALifeMonsterAbstract), CSE_ALifeHumanAbstract_Export);

SCRIPT_EXPORT(CSE_ALifePsyDogPhantom, (CSE_ALifeMonsterBase), {
    module(luaState)[luabind_class_monster1(CSE_ALifePsyDogPhantom, "cse_alife_psydog_phantom", CSE_ALifeMonsterBase)];
});
