#pragma once

class CVisualZone : public CCustomZone
{
    typedef CCustomZone inherited;
    MotionID m_idle_animation;
    MotionID m_attack_animation;
    u32 m_dwAttackAnimaionStart;
    u32 m_dwAttackAnimaionEnd;

public:
    CVisualZone();
    virtual ~CVisualZone();
    virtual bool net_Spawn(CSE_Abstract* DC);
    virtual void SwitchZoneState(EZoneState new_state);
    virtual void Load(const char* section);
    virtual void UpdateBlowout();

protected:
private:
};
