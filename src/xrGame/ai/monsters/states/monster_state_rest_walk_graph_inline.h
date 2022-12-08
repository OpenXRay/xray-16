#pragma once

template <typename _Object>
CStateMonsterRestWalkGraph<_Object>::CStateMonsterRestWalkGraph(_Object* obj) : inherited(obj) {}

template <typename _Object>
CStateMonsterRestWalkGraph<_Object>::~CStateMonsterRestWalkGraph() {}

template <typename _Object>
void CStateMonsterRestWalkGraph<_Object>::execute()
{
    this->object->path().detour_graph_points();
    this->object->set_action(ACT_WALK_FWD);
    this->object->set_state_sound(MonsterSound::eMonsterSoundIdle);
}
