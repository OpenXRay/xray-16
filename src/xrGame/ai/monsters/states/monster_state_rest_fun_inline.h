#pragma once
#include "xrPhysics/PhysicsShell.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterRestFunAbstract CStateMonsterRestFun<_Object>

#define IMPULSE_TO_CORPSE 15.f
#define MIN_DELAY 100
#define TIME_IN_STATE 8000

TEMPLATE_SPECIALIZATION
CStateMonsterRestFunAbstract::CStateMonsterRestFun(_Object* obj) : inherited(obj), time_last_hit(0) {}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestFunAbstract::initialize()
{
    inherited::initialize();

    time_last_hit = 0;
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestFunAbstract::execute()
{
    Fvector point;
    Fvector dir;

    dir.sub(this->object->CorpseMan.get_corpse_position(), this->object->Position());
    float dist = dir.magnitude();
    dir.normalize();
    point.mad(this->object->CorpseMan.get_corpse_position(), dir, 2.0f);

    this->object->set_action(ACT_RUN);
    this->object->path().set_target_point(point);
    this->object->path().set_rebuild_time(100 + u32(50.f * dist));
    this->object->path().set_use_covers(false);
    this->object->path().set_distance_to_end(0.5f);
    this->object->anim().accel_activate(eAT_Calm);
    this->object->anim().accel_set_braking(false);

    this->object->set_state_sound(MonsterSound::eMonsterSoundIdle);

    if ((dist < this->object->db().m_fDistToCorpse + 0.5f) && (time_last_hit + MIN_DELAY < Device.dwTimeGlobal))
    {
        CEntityAlive* corpse = const_cast<CEntityAlive*>(this->object->CorpseMan.get_corpse());
        CPhysicsShellHolder* target = smart_cast<CPhysicsShellHolder*>(corpse);

        if (target && target->m_pPhysicsShell)
        {
            Fvector dir;
            dir.add(Fvector().sub(target->Position(), this->object->Position()), this->object->Direction());

            float h, p;
            dir.getHP(h, p);
            dir.setHP(h, p + 5 * PI / 180);
            dir.normalize();

            // выполнить бросок
            for (u32 i = 0; i < target->m_pPhysicsShell->get_ElementsNumber(); i++)
            {
                target->m_pPhysicsShell->get_ElementByStoreOrder((u16)i)->applyImpulse(dir, IMPULSE_TO_CORPSE *
                        target->m_pPhysicsShell->getMass() / target->m_pPhysicsShell->Elements().size());
            }

            time_last_hit = Device.dwTimeGlobal;
        }
    }
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterRestFunAbstract::check_start_conditions()
{
    return ((this->object->CorpseMan.get_corpse() != 0) && this->object->Home->at_home(this->object->CorpseMan.get_corpse()->Position()));
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterRestFunAbstract::check_completion()
{
    if (!this->object->CorpseMan.get_corpse())
        return true;
    if (this->time_state_started + TIME_IN_STATE < Device.dwTimeGlobal)
        return true;
    return false;
}

#undef TIME_IN_STATE
#undef MIN_DELAY
#undef IMPULSE_TO_CORPSE
#undef CStateMonsterRestFunAbstract
#undef TEMPLATE_SPECIALIZATION
