#include "StdAfx.h"
#include "burer_fast_gravi.h"
#include "burer.h"
#include "ai/monsters/control_animation_base.h"
#include "ai/monsters/control_direction_base.h"
#include "ai/monsters/control_movement_base.h"

bool CBurerFastGravi::check_start_conditions()
{
    if (is_active())
        return false;
    if (m_man->is_captured_pure())
        return false;
    if (!m_object->EnemyMan.get_enemy())
        return false;

    return true;
}

void CBurerFastGravi::activate()
{
    //	CBurer *burer = smart_cast<CBurer *>(m_object);
    m_man->subscribe(this, ControlCom::eventTAChange);
    m_object->dir().face_target(m_object->EnemyMan.get_enemy());
}

void CBurerFastGravi::deactivate() { m_man->unsubscribe(this, ControlCom::eventTAChange); }
void CBurerFastGravi::on_event(ControlCom::EEventType type, ControlCom::IEventData* data)
{
    if (type == ControlCom::eventTAChange)
    {
        STripleAnimEventData* event_data = (STripleAnimEventData*)data;
        if (event_data->m_current_state == eStateExecute)
        {
            process_hit();
            m_object->com_man().ta_pointbreak();
            m_man->deactivate(this);
        }
    }
}

void CBurerFastGravi::process_hit()
{
    m_object->HitEntity(m_object->EnemyMan.get_enemy(), 1.f, 100.f, m_object->Direction());
}
