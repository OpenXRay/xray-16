#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterControlledAttackAbstract CStateMonsterControlledAttack<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterControlledAttackAbstract::CStateMonsterControlledAttack(_Object* obj) : inherited(obj) {}
TEMPLATE_SPECIALIZATION
void CStateMonsterControlledAttackAbstract::initialize()
{
    inherited::initialize();
    this->object->EnemyMan.force_enemy(get_enemy());
}

TEMPLATE_SPECIALIZATION
void CStateMonsterControlledAttackAbstract::execute()
{
    this->object->EnemyMan.force_enemy(get_enemy());
    inherited::execute();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterControlledAttackAbstract::finalize()
{
    inherited::finalize();
    this->object->EnemyMan.unforce_enemy();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterControlledAttackAbstract::critical_finalize()
{
    inherited::critical_finalize();
    this->object->EnemyMan.unforce_enemy();
}

TEMPLATE_SPECIALIZATION
const CEntityAlive* CStateMonsterControlledAttackAbstract::get_enemy()
{
    CControlledEntityBase* entity = smart_cast<CControlledEntityBase*>(this->object);
    VERIFY(entity);
    return smart_cast<const CEntityAlive*>(entity->get_data().m_object);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterControlledAttackAbstract
