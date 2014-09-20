////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat_inline.h
//	Created 	: 23.04.2002
//  Modified 	: 26.11.2002
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Rat" (inline functions)
////////////////////////////////////////////////////////////////////////////

#pragma once

IC void CAI_Rat::vfChangeGoal()
{
	Fvector vP;
	vP.set(m_tSpawnPosition.x,m_tSpawnPosition.y,m_tSpawnPosition.z);
	m_tGoalDir.x = vP.x+m_tVarGoal.x*::Random.randF(-0.5f,0.5f); 
	m_tGoalDir.y = vP.y+m_tVarGoal.y*::Random.randF(-0.5f,0.5f);
	m_tGoalDir.z = vP.z+m_tVarGoal.z*::Random.randF(-0.5f,0.5f);
}

IC bool CAI_Rat::bfCheckIfGoalChanged(bool bForceChangeGoal)
{
	if (m_fGoalChangeTime<=0){
		m_fGoalChangeTime += m_fGoalChangeDelta+m_fGoalChangeDelta*::Random.randF(-0.5f,0.5f);
		if (bForceChangeGoal)
			vfChangeGoal();
		return(true);
	}
	return(false);
};

IC void CAI_Rat::vfChooseNewSpeed()
{
	int iRandom = ::Random.randI(0,2);
	switch (iRandom) {
				case 0 : {
					m_fSpeed = m_fMaxSpeed;
					break;
						 }
				case 1 : {
					m_fSpeed = m_fMinSpeed;
					break;
						 }
	}
	m_fSafeSpeed = m_fSpeed;
};

IC void CAI_Rat::vfUpdateTime(float fTimeDelta)
{
	m_fGoalChangeTime -= fTimeDelta > .1f ? .1f : fTimeDelta;
};		

IC	bool CAI_Rat::use_model_pitch	() const
{
	return			(!!g_Alive());
}
