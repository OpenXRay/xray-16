#pragma once

#include "Level.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object>

#define CStateMonsterLookActorAbstract CStateMonsterLookActor<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterLookActorAbstract::execute()
{
    this->object->set_action(ACT_STAND_IDLE);
    this->object->dir().face_target(Level().CurrentEntity()->Position(), 1200);
}

#define CStateMonsterTurnAwayFromActorAbstract CStateMonsterTurnAwayFromActor<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterTurnAwayFromActorAbstract::execute()
{
    Fvector point;
    Fvector dir;
    dir.sub(this->object->Position(), Level().CurrentEntity()->Position());
    dir.normalize();
    point.mad(this->object->Position(), dir, 2.f);

    this->object->set_action(ACT_STAND_IDLE);
    this->object->dir().face_target(point, 1200);
}

#define CStateMonstertTestIdleAbstract CStateMonstertTestIdle<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonstertTestIdleAbstract::execute() { this->object->set_action(ACT_STAND_IDLE); }

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterLookActorAbstract
#undef CStateMonsterTurnAwayFromActorAbstract
#undef CStateMonstertTestIdleAbstract
