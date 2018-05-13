#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterRestWalkGraphAbstract CStateMonsterRestWalkGraph<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterRestWalkGraphAbstract::CStateMonsterRestWalkGraph(_Object* obj) : inherited(obj) {}
TEMPLATE_SPECIALIZATION
CStateMonsterRestWalkGraphAbstract::~CStateMonsterRestWalkGraph() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterRestWalkGraphAbstract::execute()
{
    this->object->path().detour_graph_points();
    this->object->set_action(ACT_WALK_FWD);
    this->object->set_state_sound(MonsterSound::eMonsterSoundIdle);
}
