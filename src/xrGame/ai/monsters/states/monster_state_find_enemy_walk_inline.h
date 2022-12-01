#pragma once

template <typename _Object>
void CStateMonsterFindEnemyWalkAround<_Object>::execute()
{
    this->object->set_action(ACT_STAND_IDLE);
    this->object->set_state_sound(MonsterSound::eMonsterSoundAggressive);
}
