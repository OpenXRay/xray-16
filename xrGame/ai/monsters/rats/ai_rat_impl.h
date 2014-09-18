////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat_impl.h
//	Created 	: 23.04.2002
//  Modified 	: 26.11.2002
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Rat" (inline functions implementation)
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../../level.h"
#include "../../../seniority_hierarchy_holder.h"
#include "../../../team_hierarchy_holder.h"
#include "../../../squad_hierarchy_holder.h"
#include "../../../group_hierarchy_holder.h"
#include "../../../../xrServerEntities/ai_sounds.h"

IC void CAI_Rat::add_active_member	(bool bForceActive)
{
	CGroupHierarchyHolder &Group = Level().seniority_holder().team(g_Team()).squad(g_Squad()).group(g_Group());
	if (!m_bActive && (bForceActive || (Group.m_dwAliveCount*m_dwActiveCountPercent/100 >= Group.m_dwActiveCount))) {
		m_bActive = true;
		m_eCurrentState = aiRatFreeActive;
		++Group.m_dwActiveCount;
		shedule.t_min	= m_dwActiveScheduleMin;
		shedule.t_max	= m_dwActiveScheduleMax;
		vfRemoveStandingMember();
	}
	//Msg("* Group : alive[%2d], active[%2d]",Group.m_dwAliveCount,Group.m_dwActiveCount);
};

IC void CAI_Rat::vfRemoveActiveMember()
{
	CGroupHierarchyHolder &Group = Level().seniority_holder().team(g_Team()).squad(g_Squad()).group(g_Group());
	if (m_bActive) {
		R_ASSERT(Group.m_dwActiveCount > 0);
		--(Group.m_dwActiveCount);
		m_bActive = false;
		m_eCurrentState = aiRatFreePassive;
		shedule.t_min	= m_dwPassiveScheduleMin;
		shedule.t_max	= m_dwPassiveScheduleMax;
	}
	//Msg("* Group : alive[%2d], active[%2d]",Group.m_dwAliveCount,Group.m_dwActiveCount);
};

IC void CAI_Rat::vfAddStandingMember()
{
	CGroupHierarchyHolder &Group = Level().seniority_holder().team(g_Team()).squad(g_Squad()).group(g_Group());
	if ((Group.m_dwAliveCount*m_dwStandingCountPercent/100 >= Group.m_dwStandingCount) && (!m_bStanding)) {
		++Group.m_dwStandingCount;
		m_bStanding = true;
	}
};

IC void CAI_Rat::vfRemoveStandingMember()
{
	CGroupHierarchyHolder &Group = Level().seniority_holder().team(g_Team()).squad(g_Squad()).group(g_Group());
	if (m_bStanding) {
		R_ASSERT(Group.m_dwStandingCount > 0);
		--(Group.m_dwStandingCount);
		m_bStanding = false;
	}
};

IC bool CAI_Rat::bfCheckIfSoundFrightful()
{
	return(((m_tLastSound.eSoundType & SOUND_TYPE_WEAPON_BULLET_HIT) == SOUND_TYPE_WEAPON_BULLET_HIT) || ((m_tLastSound.eSoundType & SOUND_TYPE_WEAPON_SHOOTING) == SOUND_TYPE_WEAPON_SHOOTING));
};

IC	void CAI_Rat::update_morale_broadcast(float const &fValue, float const &/**fRadius/**/)
{
	CGroupHierarchyHolder &Group = Level().seniority_holder().team(g_Team()).squad(g_Squad()).group(g_Group());
	for (int i=0; i<(int)Group.members().size(); ++i)
		if (Group.members()[i]->g_Alive())
			Group.members()[i]->m_fMorale += fValue;
}
