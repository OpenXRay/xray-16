#pragma once

#include "monster_state_controlled_attack.h"
#include "monster_state_controlled_follow.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterControlledAbstract CStateMonsterControlled<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterControlledAbstract::CStateMonsterControlled(_Object* obj) : inherited(obj)
{
    this->add_state(eStateControlled_Attack, new CStateMonsterControlledAttack<_Object>(obj));
    this->add_state(eStateControlled_Follow, new CStateMonsterControlledFollow<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
void CStateMonsterControlledAbstract::execute()
{
    switch (this->object->get_data().m_task)
    {
    case eTaskFollow: this->select_state(eStateControlled_Follow); break;
    case eTaskAttack:
    {
        // проверить валидность данных атаки
        const CEntity* enemy = this->object->get_data().m_object;
        if (!enemy || enemy->getDestroy() || !enemy->g_Alive())
        {
            this->object->get_data().m_object = this->object->get_controller();
            this->select_state(eStateControlled_Follow);
        }
        else
            this->select_state(eStateControlled_Attack);
        break;
    }
    default: NODEFAULT;
    }

    this->get_state_current()->execute();

    this->prev_substate = this->current_substate;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterControlledAbstract
