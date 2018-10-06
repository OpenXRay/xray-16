#include "pch_script.h"
#include "trader_animation.h"
#include "ai_trader.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "game_object_space.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Startup
/////////////////////////////////////////////////////////////////////////////////////////

void CTraderAnimation::reinit()
{
    m_motion_head.invalidate();
    m_motion_global.invalidate();
    m_sound = 0;
    m_external_sound = 0;

    m_anim_global = 0;
    m_anim_head = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Animation Callbacks
/////////////////////////////////////////////////////////////////////////////////////////
void CTraderAnimation::global_callback(CBlend* B)
{
    CTraderAnimation* trader = (CTraderAnimation*)B->CallbackParam;
    trader->m_motion_global.invalidate();
}

void CTraderAnimation::head_callback(CBlend* B)
{
    CTraderAnimation* trader = (CTraderAnimation*)B->CallbackParam;
    trader->m_motion_head.invalidate();
}

/////////////////////////////////////////////////////////////////////////////////////////
// Animation management
/////////////////////////////////////////////////////////////////////////////////////////
void CTraderAnimation::set_animation(LPCSTR anim)
{
    m_anim_global = anim;

    IKinematicsAnimated* kinematics_animated = smart_cast<IKinematicsAnimated*>(m_trader->Visual());
    m_motion_global = kinematics_animated->ID_Cycle(m_anim_global);
    kinematics_animated->PlayCycle(m_motion_global, TRUE, global_callback, this);
}

void CTraderAnimation::set_head_animation(LPCSTR anim)
{
    m_anim_head = anim;

    // назначить анимацию головы
    IKinematicsAnimated* kinematics_animated = smart_cast<IKinematicsAnimated*>(m_trader->Visual());
    m_motion_head = kinematics_animated->ID_Cycle(m_anim_head);
    kinematics_animated->PlayCycle(m_motion_head, TRUE, head_callback, this);
}

//////////////////////////////////////////////////////////////////////////
// Sound management
//////////////////////////////////////////////////////////////////////////
void CTraderAnimation::set_sound(LPCSTR sound, LPCSTR anim)
{
    if (m_sound)
        remove_sound();

    set_head_animation(anim);

    m_sound = new ref_sound();
    m_sound->create(sound, st_Effect, SOUND_TYPE_WORLD);
    //m_sound->play(NULL, sm_2D);
    m_sound->play(m_trader);
}

void CTraderAnimation::remove_sound()
{
    VERIFY(m_sound);

    if (m_sound->_feedback())
        m_sound->stop();

    m_sound->destroy();
    xr_delete(m_sound);
}

//////////////////////////////////////////////////////////////////////////
// Update
//////////////////////////////////////////////////////////////////////////
void CTraderAnimation::update_frame()
{
    if (m_sound)
    {
        if (m_sound->_feedback())
            m_sound->set_position(m_trader->Position());
        else
        {
            m_trader->callback(GameObject::eTraderSoundEnd)();
            remove_sound();
        }
    }

    if (!m_motion_global)
    {
        m_trader->callback(GameObject::eTraderGlobalAnimationRequest)();
        if (m_anim_global)
            m_motion_head.invalidate();
    }

    // назначить анимацию головы
    if (!m_motion_head)
    {
        if (m_sound && m_sound->_feedback())
        {
            m_trader->callback(GameObject::eTraderHeadAnimationRequest)();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// External sound support
//////////////////////////////////////////////////////////////////////////
void CTraderAnimation::external_sound_start(LPCSTR phrase)
{
    if (m_sound)
        remove_sound();

    m_sound = new ref_sound();
    m_sound->create(phrase, st_Effect, SOUND_TYPE_WORLD);
    m_sound->play(NULL, sm_2D);

    m_motion_head.invalidate();
}

void CTraderAnimation::external_sound_stop()
{
    if (m_sound)
        remove_sound();
}
//////////////////////////////////////////////////////////////////////////
