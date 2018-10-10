#include "StdAfx.h"
#include "anim_triple.h"
#include "control_manager.h"

constexpr pcstr dbg_states[] = { "eStatePrepare", "eStateExecute", "eStateFinalize", "eStateNone" };

void CAnimationTriple::reset_data() { m_data.capture_type = 0; }
void CAnimationTriple::on_capture()
{
    m_current_state = eStateNone;

    m_man->capture(this, ControlCom::eControlAnimation);
    m_man->subscribe(this, ControlCom::eventAnimationEnd);
}

void CAnimationTriple::on_release()
{
    m_man->release(this, ControlCom::eControlAnimation);
    m_man->unsubscribe(this, ControlCom::eventAnimationEnd);

    if ((m_data.capture_type & ControlCom::eCapturePath) == ControlCom::eCapturePath)
        m_man->release(this, ControlCom::eControlPath);

    if ((m_data.capture_type & ControlCom::eCaptureMovement) == ControlCom::eCaptureMovement)
        m_man->release(this, ControlCom::eControlMovement);

    if ((m_data.capture_type & ControlCom::eCaptureDir) == ControlCom::eCaptureDir)
        m_man->release(this, ControlCom::eControlDir);
}

bool CAnimationTriple::check_start_conditions()
{
    if (is_active())
        return false;
    if (m_man->is_captured(ControlCom::eControlAnimation))
        return false;

    return true;
}

void CAnimationTriple::activate()
{
    if ((m_data.capture_type & ControlCom::eCapturePath) == ControlCom::eCapturePath)
    {
        m_man->capture(this, ControlCom::eControlPath);
        m_man->path_stop(this);
    }

    if ((m_data.capture_type & ControlCom::eCaptureMovement) == ControlCom::eCaptureMovement)
    {
        m_man->capture(this, ControlCom::eControlMovement);
        m_man->move_stop(this);
    }

    if ((m_data.capture_type & ControlCom::eCaptureDir) == ControlCom::eCaptureDir)
    {
        m_man->capture(this, ControlCom::eControlDir);
        m_man->dir_stop(this);
    }

    m_current_state = m_data.skip_prepare ? eStateExecute : eStatePrepare;
    m_previous_state = m_data.skip_prepare ? eStatePrepare : eStateNone;
    select_next_state();
}

void CAnimationTriple::on_event(ControlCom::EEventType, ControlCom::IEventData*) { select_next_state(); }
void CAnimationTriple::pointbreak()
{
    m_current_state = eStateFinalize;
    select_next_state();
}

void CAnimationTriple::select_next_state()
{
    if (m_current_state == eStateNone)
    {
        STripleAnimEventData event(m_current_state);
        m_man->notify(ControlCom::eventTAChange, &event);
        return;
    }

    if ((m_current_state == eStateExecute) && m_data.execute_once && (m_previous_state == eStateExecute))
        return;

    play_selected();

    // raise event
    if ((m_current_state != eStateExecute) ||
        ((m_current_state == eStateExecute) && (m_previous_state != eStateExecute)))
    {
        STripleAnimEventData event(m_current_state);
        m_man->notify(ControlCom::eventTAChange, &event);
    }

    m_previous_state = m_current_state;
    if (m_current_state != eStateExecute)
        m_current_state = EStateAnimTriple(m_current_state + 1);
}

void CAnimationTriple::play_selected()
{
    // start new animation
    SControlAnimationData* ctrl_data = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
    VERIFY(ctrl_data);

    ctrl_data->global.set_motion(m_data.pool[m_current_state]);
    ctrl_data->global.actual = false;
}
