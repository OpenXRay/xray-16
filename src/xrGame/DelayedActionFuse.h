#pragma once

class CDelayedActionFuse
{
	enum{
		flActive			=			1<<0,
		flInitialized		=			1<<1,
		flNoConditionChange =			1<<2
	};
	Flags8		m_dafflags								;		
	float		m_fTime									;
	float		m_fSpeedChangeCondition					;

public:
		void	SetTimer				(float current_condition)										;
		void	Initialize				(float time,float critical_condition)							;
ICF		bool	CheckCondition			(float current_condition)										{if(isInitialized()&&!isActive()&&m_fSpeedChangeCondition>=current_condition){SetTimer(current_condition);return true;} else return false ;}
ICF		bool	isActive				()																{return !!m_dafflags.test(flActive);}
ICF		bool	isInitialized			()																{return !!m_dafflags.test(flInitialized);}
		bool	Update					(float current_condition)										;
		float	Time					()																;
protected:
				CDelayedActionFuse		()																;
virtual void	ChangeCondition			(float fDeltaCondition)											=0;
virtual void	StartTimerEffects		()																=0;
};