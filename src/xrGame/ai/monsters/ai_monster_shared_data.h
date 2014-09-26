#pragma once 
#include "ai_monster_defs.h"

struct SMonsterSettings {
	
	// float speed factors

	float					m_fDistToCorpse;
	float					m_fDamagedThreshold;		// порог здоровья, ниже которого устанавливается флаг m_bDamaged

	// -------------------------------------------------------

	u32						m_dwIdleSndDelay;
	u32						m_dwEatSndDelay;
	u32						m_dwAttackSndDelay;

	u32						m_dwDistantIdleSndDelay;
	float					m_fDistantIdleSndRange;

	// -------------------------------------------------------

	u32						m_dwDayTimeBegin;
	u32						m_dwDayTimeEnd;
	float					satiety_threshold;
	
	// ----------------------------------------------------------- 

	float					m_fSoundThreshold;

	float					m_fEatFreq;
	float					m_fEatSlice;
	float					m_fEatSliceWeight;

	u8						m_legs_number;
	SAttackEffector			m_attack_effector;

	float					m_max_hear_dist;

	float					m_run_attack_path_dist;
	float					m_run_attack_start_dist;
};
