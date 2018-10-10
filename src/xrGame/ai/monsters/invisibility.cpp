#include "StdAfx.h"
#include "invisibility.h"

void CInvisibility::reinit()
{
    m_time_start_blink = 0;
    m_cur_visibility = false;
    m_blink = false;
    m_time_last_blink = 0;

    set_manual_control(false);

    m_active = false;
    m_energy = 0.f;
}

void CInvisibility::reload(LPCSTR section)
{
    timeBlink = pSettings->r_u32(section, "Invisibility_BlinkTime");
    timeBlinkInterval = pSettings->r_u32(section, "Invisibility_BlinkMicroInterval");
    m_speed = pSettings->r_float(section, "Invisibility_EnergySpeed");
}

void CInvisibility::activate()
{
    if (m_active)
        return;
    start_blink();

    m_active = true;
    on_activate();
}

void CInvisibility::deactivate()
{
    if (!m_active)
        return;
    start_blink();

    m_active = false;
    on_deactivate();
}

void CInvisibility::start_blink()
{
    m_blink = true;
    m_time_start_blink = Device.dwTimeGlobal;
    m_time_last_blink = 0;
}

void CInvisibility::stop_blink()
{
    m_blink = false;
    m_cur_visibility = !active();

    on_change_visibility(m_cur_visibility);
}

void CInvisibility::update_blink()
{
    if (!m_blink)
        return;

    u32 cur_time = Device.dwTimeGlobal;

    // check for whole blink time
    if (m_time_start_blink + timeBlink < cur_time)
    {
        stop_blink();
        return;
    }

    // check for current blink interval time
    if (m_time_last_blink + timeBlinkInterval < cur_time)
    {
        // blink
        m_time_last_blink = cur_time;
        m_cur_visibility = !m_cur_visibility;

        on_change_visibility(m_cur_visibility);
    }
}

void CInvisibility::frame_update()
{
    update_blink();

    if (!m_manual)
    {
        if (m_active)
            m_energy -= m_speed * Device.fTimeDelta;
        else
            m_energy += m_speed * Device.fTimeDelta;
        clamp(m_energy, 0.f, 1.f);
    }
}

void CInvisibility::set_manual_control(bool b_man) { m_manual = b_man; }
void CInvisibility::manual_activate()
{
    if (m_manual)
        activate();
}

void CInvisibility::manual_deactivate()
{
    if (m_manual)
        deactivate();
}
