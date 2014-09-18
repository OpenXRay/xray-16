#pragma once

class CVisualZone :
	public CCustomZone
{
	typedef				CCustomZone		inherited	;
	MotionID			m_idle_animation			;
	MotionID			m_attack_animation			;
	u32					m_dwAttackAnimaionStart		;
	u32					m_dwAttackAnimaionEnd		;
public:
	CVisualZone				()						;
	virtual			~CVisualZone					()						;
	virtual BOOL	net_Spawn						(CSE_Abstract* DC)				;
	virtual void	SwitchZoneState					(EZoneState new_state)	;
	virtual void	Load							(LPCSTR section)		;
	virtual void	UpdateBlowout					()						;
protected:
private:
};
