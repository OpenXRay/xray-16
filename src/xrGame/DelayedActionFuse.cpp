#include "StdAfx.h"
#include "DelayedActionFuse.h"

CDelayedActionFuse::CDelayedActionFuse()
{
    m_dafflags.assign(0);
    m_fTime = 0.f;
    m_fSpeedChangeCondition = 0.f;
}

void CDelayedActionFuse::SetTimer(float current_condition)
{
    VERIFY(isInitialized() && !isActive());
    m_dafflags.set(flActive, TRUE);
    ChangeCondition(m_fSpeedChangeCondition - current_condition);
    VERIFY(!fis_zero(m_fTime) || m_dafflags.test(flNoConditionChange));
    if (!m_dafflags.test(flNoConditionChange))
        m_fSpeedChangeCondition /= m_fTime;
    // Msg("to_expl moment %f",m_fTime);
    m_fTime += Device.fTimeGlobal; //+current_condition/m_fSpeedChangeCondition;
    // Msg("expl moment %f",m_fTime);
    StartTimerEffects();
}
float CDelayedActionFuse::Time()
{
    VERIFY(isInitialized());
    if (!isActive())
        return m_fTime;
    else
        return m_fTime - Device.fTimeGlobal;
}
void CDelayedActionFuse::Initialize(float time, float critical_condition)
{
    if (isActive())
        return;

    VERIFY(time >= 0.f && critical_condition >= 0.f);
    if (!fis_zero(time))
    {
        m_fSpeedChangeCondition = critical_condition; // time;
        m_fTime = time;
    }
    else
    {
        m_fSpeedChangeCondition = 0.f;
        m_fTime = 0.f;
    }
    if (fis_zero(m_fSpeedChangeCondition))
        m_dafflags.set(flNoConditionChange, TRUE);
    m_dafflags.set(flInitialized, TRUE);
}
bool CDelayedActionFuse::Update(float current_condition)
{
    VERIFY(isActive());

    bool ret = false;
    float l_time_to_explosion = m_fTime - Device.fTimeGlobal;

    if (!m_dafflags.test(flNoConditionChange))
    {
        float delta_condition = m_fSpeedChangeCondition * l_time_to_explosion - current_condition;
        // float t=current_condition/m_fSpeedChangeCondition;
        // if(t<l_time_to_explosion) m_fTime;
        // VERIFY(delta_condition<=0.f);
        if (delta_condition > 0.f)
            delta_condition = 0.f; //.
        ChangeCondition(delta_condition);
        ret = current_condition + delta_condition <= 0.f;
    }
    else
    {
        ret = l_time_to_explosion <= 0.f;
    }

    if (ret)
    {
        m_dafflags.set(flActive, FALSE);
        m_dafflags.set(flInitialized, FALSE);
    }
    return ret;
}
