////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_combat_manager.h
//	Created 	: 12.08.2003
//  Modified 	: 14.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife combat manager
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_combat_manager.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_graph_registry.h"
#include "alife_schedule_registry.h"
/**
#include "ef_storage.h"
#include "ef_pattern.h"
#include "alife_object_registry.h"
#include "alife_spawn_registry.h"
#include "alife_time_manager.h"
#include "relation_registry.h"

using namespace ALife;

#define NORMALIZE_VARIABLE(a,b,c,d) a = u32(b % c) + d, b /= c;

void print_time(LPCSTR S, _TIME_ID tTimeID)
{
	u32 Milliseconds,Seconds,Minutes,Hours,Days,Week,Months,Years;
	NORMALIZE_VARIABLE(Milliseconds,tTimeID,1000,0);
	NORMALIZE_VARIABLE(Seconds,		tTimeID,  60,0);
	NORMALIZE_VARIABLE(Minutes,		tTimeID,  60,0);
	NORMALIZE_VARIABLE(Hours,		tTimeID,  24,0);
	NORMALIZE_VARIABLE(Days,		tTimeID,   7,1);
	NORMALIZE_VARIABLE(Week,		tTimeID,   4,1);
	NORMALIZE_VARIABLE(Months,		tTimeID,  12,1);
	Years = u32(tTimeID) + 1;
	Msg("%s year %d month %d week %d day %d time %d:%d:%d.%d",S,Years,Months,Week,Days,Hours,Minutes,Seconds,Milliseconds);
}

/**/
CALifeCombatManager::CALifeCombatManager	(xrServer *server, LPCSTR section) :
	CALifeSimulatorBase	(server,section)
{
/**
	seed						(u32(CPU::QPC() & 0xffffffff));
	m_dwMaxCombatIterationCount	= pSettings->r_u32	(section,"max_combat_iteration_count");
	for (int i=0; i<2; ++i) {
		m_tpaCombatGroups[i].clear();
		m_tpaCombatGroups[i].reserve(255);
	}
/**/
}

/**
CALifeCombatManager::~CALifeCombatManager	()
{
}

void CALifeCombatManager::vfFillCombatGroup(CSE_ALifeSchedulable *tpALifeSchedulable, int iGroupIndex)
{
	EHitType				l_tHitType;
	float					l_fHitPower;
	SCHEDULE_P_VECTOR		&tpGroupVector = m_tpaCombatGroups[iGroupIndex];
	CSE_ALifeGroupAbstract	*l_tpALifeGroupAbstract = smart_cast<CSE_ALifeGroupAbstract*>(tpALifeSchedulable);
	tpGroupVector.clear		();
	if (l_tpALifeGroupAbstract) {
		OBJECT_IT			I = l_tpALifeGroupAbstract->m_tpMembers.begin();
		OBJECT_IT			E = l_tpALifeGroupAbstract->m_tpMembers.end();
		for ( ; I != E; ++I) {
			CSE_ALifeSchedulable	*l_tpALifeSchedulable = smart_cast<CSE_ALifeSchedulable*>(objects().object(*I));
			R_ASSERT2				(l_tpALifeSchedulable,"Invalid combat object");
			tpGroupVector.push_back(l_tpALifeSchedulable);
			l_tpALifeSchedulable->tpfGetBestWeapon(l_tHitType,l_fHitPower);
		}
	}
	else {
		tpGroupVector.push_back(tpALifeSchedulable);
		tpALifeSchedulable->tpfGetBestWeapon(l_tHitType,l_fHitPower);
	}
	m_tpaCombatObjects[iGroupIndex] = tpALifeSchedulable;
}

ECombatAction CALifeCombatManager::choose_combat_action(int iCombatGroupIndex)
{
	ai().ef_storage().alife_evaluation(true);
	if (eCombatTypeMonsterMonster != combat_type())
		if (((eCombatTypeAnomalyMonster == combat_type()) && !iCombatGroupIndex) || ((eCombatTypeMonsterAnomaly == combat_type()) && iCombatGroupIndex))
			return(eCombatActionAttack);
		else
			return(eCombatActionRetreat);

	SCHEDULE_P_VECTOR	&Members = m_tpaCombatGroups[iCombatGroupIndex];
	SCHEDULE_P_VECTOR	&Enemies = m_tpaCombatGroups[iCombatGroupIndex ^ 1];
	int i = 0, j = 0, I = (int)Members.size(), J = (int)Enemies.size();
	float	fMinProbability;
	if (!I)
		fMinProbability = 0;
	else {
		CSE_ALifeMonsterAbstract	*l_tpALifeMonsterAbstract = smart_cast<CSE_ALifeMonsterAbstract*>(Members[0]);
		R_ASSERT2					(l_tpALifeMonsterAbstract,"Invalid combat object");
		fMinProbability				= l_tpALifeMonsterAbstract->m_fRetreatThreshold;
	}
	while ((i < I) && (j < J)) {
		ai().ef_storage().alife().member()	= smart_cast<CSE_ALifeMonsterAbstract*>(Members[i]);
		ai().ef_storage().alife().enemy()	= smart_cast<CSE_ALifeMonsterAbstract*>(Enemies[j]);
		float fProbability = ai().ef_storage().m_pfVictoryProbability->ffGetValue()/100.f, fCurrentProbability;
		if (fProbability > fMinProbability) {
			fCurrentProbability = fProbability;
			for (++j; (i < I) && (j < J); ++j) {
				ai().ef_storage().alife().enemy() = smart_cast<CSE_ALifeMonsterAbstract*>(Enemies[j]);
				fProbability = ai().ef_storage().m_pfVictoryProbability->ffGetValue()/100.f;
				if (fCurrentProbability*fProbability < fMinProbability) {
					++i;
					break;
				}
				else
					fCurrentProbability *= fProbability;
			}
		}
		else {
			fCurrentProbability = 1.0f - fProbability;
			for (++i; (i < I) && (j < J); ++i) {
				ai().ef_storage().alife().member()	= smart_cast<CSE_ALifeMonsterAbstract*>(Members[i]);
				fProbability = 1.0f - ai().ef_storage().m_pfVictoryProbability->ffGetValue()/100.f;
				if (fCurrentProbability*fProbability < fMinProbability) {
					++j;
					break;
				}
				else
					fCurrentProbability *= fProbability;
			}
		}
	}
	return((j >= J) ? eCombatActionAttack : eCombatActionRetreat);
}

bool CALifeCombatManager::bfCheckObjectDetection(CSE_ALifeSchedulable *tpALifeSchedulable1, CSE_ALifeSchedulable *tpALifeSchedulable2)
{
	ai().ef_storage().alife_evaluation(true);
	switch (combat_type()) {
		case eCombatTypeMonsterMonster : {
			ai().ef_storage().alife().member_item()	= smart_cast<CSE_ALifeMonsterAbstract*>(tpALifeSchedulable1);
			ai().ef_storage().alife().member()	= tpALifeSchedulable1;
			ai().ef_storage().alife().enemy()		= tpALifeSchedulable2;
			return										(randF(100) < ai().ef_storage().m_pfEnemyDetectProbability->ffGetValue());
		}
		case eCombatTypeAnomalyMonster : {
			ai().ef_storage().alife().enemy()		= tpALifeSchedulable1;
			return										(randF(100) < ai().ef_storage().m_pfAnomalyInteractProbability->ffGetValue());
		}
		case eCombatTypeMonsterAnomaly : {
			ai().ef_storage().alife().member_item()	= tpALifeSchedulable1->tpfGetBestDetector();
			ai().ef_storage().alife().member()	= tpALifeSchedulable1;
			ai().ef_storage().alife().enemy()		= tpALifeSchedulable2;
			return										(randF(100) < ai().ef_storage().m_pfAnomalyDetectProbability->ffGetValue());
		}
		case eCombatTypeSmartTerrain : {
			CSE_ALifeSmartZone							*smart_zone	= smart_cast<CSE_ALifeSmartZone*>(tpALifeSchedulable1);
			return										(!smart_zone ? false : randF(100) < 100.f*smart_zone->detect_probability());
		}
		default :										NODEFAULT;
	}
#ifdef DEBUG
	return												(false);
#endif // DEBUG
}

bool CALifeCombatManager::bfCheckForInteraction(CSE_ALifeSchedulable *tpALifeSchedulable1, CSE_ALifeSchedulable *tpALifeSchedulable2, int &iCombatGroupIndex, bool &bMutualDetection)
{
	if (!tpALifeSchedulable1->bfActive() || !tpALifeSchedulable2->bfActive())
		return(false);
	
	// determine combat type
	CSE_ALifeMonsterAbstract	*l_tpALifeMonsterAbstract1	= smart_cast<CSE_ALifeMonsterAbstract*>(tpALifeSchedulable1);
	CSE_ALifeMonsterAbstract	*l_tpALifeMonsterAbstract2	= smart_cast<CSE_ALifeMonsterAbstract*>(tpALifeSchedulable2);
	if (!l_tpALifeMonsterAbstract1) {
		if (!l_tpALifeMonsterAbstract2)
			return(false);
		else {
			CSE_ALifeCustomZone		*l_tpALifeSpaceRestrictor = smart_cast<CSE_ALifeCustomZone*>(tpALifeSchedulable2);
			R_ASSERT2				(l_tpALifeSpaceRestrictor,"Unknown schedulable object class");
			m_combat_type			= eCombatTypeAnomalyMonster;
		}
	}
	else {
		if (!l_tpALifeMonsterAbstract2) {
			CSE_ALifeCustomZone			*l_tpALifeSpaceRestrictor = smart_cast<CSE_ALifeCustomZone*>(tpALifeSchedulable2);
			if (!l_tpALifeSpaceRestrictor) {
				CSE_ALifeSmartZone		*smart_zone = smart_cast<CSE_ALifeSmartZone*>(tpALifeSchedulable2);
				if (smart_zone)
					m_combat_type		= eCombatTypeSmartTerrain;
				else
					R_ASSERT2			(false,"Unknown schedulable object class");
			}
			else
				m_combat_type			= eCombatTypeMonsterAnomaly;
		}
		else {
			m_combat_type			= eCombatTypeMonsterMonster;
			if (eRelationTypeFriend == relation_type(l_tpALifeMonsterAbstract1,l_tpALifeMonsterAbstract2)) {
				CSE_ALifeHumanAbstract	*l_tpALifeHumanAbstract1 = smart_cast<CSE_ALifeHumanAbstract*>(l_tpALifeMonsterAbstract1);
				CSE_ALifeHumanAbstract	*l_tpALifeHumanAbstract2 = smart_cast<CSE_ALifeHumanAbstract*>(l_tpALifeMonsterAbstract2);
				if (l_tpALifeHumanAbstract1 && l_tpALifeHumanAbstract2) {
					iCombatGroupIndex = 0;
					return(true);
				}
				else
					return(false);
			}
		}
	}
	
	// perform interaction
#ifdef DEBUG
	if (psAI_Flags.test(aiALife)) {
		GameGraph::_GRAPH_ID			l_tGraphID = l_tpALifeMonsterAbstract1 ? l_tpALifeMonsterAbstract1->m_tGraphID : l_tpALifeMonsterAbstract2->m_tGraphID;
		print_time						("\n[LSS]",time_manager().game_time());
		Msg								("[LSS] %s met %s on the graph point %d (level %s[%d][%d][%d][%d], [%f][%f][%f])",
			tpALifeSchedulable1->base()->name_replace(),
			tpALifeSchedulable2->base()->name_replace(),
			l_tGraphID,
			*ai().game_graph().header().levels().find(ai().game_graph().vertex(l_tGraphID)->level_id())->second.name(),
			ai().game_graph().vertex(l_tGraphID)->vertex_type()[0],
			ai().game_graph().vertex(l_tGraphID)->vertex_type()[1],
			ai().game_graph().vertex(l_tGraphID)->vertex_type()[2],
			ai().game_graph().vertex(l_tGraphID)->vertex_type()[3],
			VPUSH(ai().game_graph().vertex(l_tGraphID)->level_point())
		);
	}
#endif
	
	bMutualDetection				= false;
	iCombatGroupIndex				= -1;

	if (bfCheckObjectDetection(tpALifeSchedulable1,tpALifeSchedulable2)) {
		iCombatGroupIndex			= 0;
#ifdef DEBUG
		if (psAI_Flags.test(aiALife)) {
			Msg						("[LSS] %s detected %s",tpALifeSchedulable1->base()->name_replace(),tpALifeSchedulable2->base()->name_replace());
		}
#endif
	}
	else {
#ifdef DEBUG
		if (psAI_Flags.test(aiALife)) {
			Msg						("[LSS] %s didn't detect %s",tpALifeSchedulable1->base()->name_replace(),tpALifeSchedulable2->base()->name_replace());
		}
#endif
	}

	if (eCombatTypeMonsterAnomaly == combat_type())
		m_combat_type = eCombatTypeAnomalyMonster;
	else
		if (eCombatTypeAnomalyMonster == combat_type())
			m_combat_type = eCombatTypeMonsterAnomaly;

	if (bfCheckObjectDetection(tpALifeSchedulable2,tpALifeSchedulable1)) {
#ifdef DEBUG
		if (psAI_Flags.test(aiALife)) {
			Msg						("[LSS] %s detected %s",tpALifeSchedulable2->base()->name_replace(),tpALifeSchedulable1->base()->name_replace());
		}
#endif
		if (!iCombatGroupIndex)
			bMutualDetection		= true;
		else
			iCombatGroupIndex		= 1;
	}
	else {
#ifdef DEBUG
		if (psAI_Flags.test(aiALife)) {
			Msg						("[LSS] %s didn't detect %s",tpALifeSchedulable2->base()->name_replace(),tpALifeSchedulable1->base()->name_replace());
		}
#endif
	}

	if (eCombatTypeMonsterAnomaly == combat_type())
		m_combat_type = eCombatTypeAnomalyMonster;
	else
		if (eCombatTypeAnomalyMonster == combat_type())
			m_combat_type = eCombatTypeMonsterAnomaly;

	if (iCombatGroupIndex < 0) {
#ifdef DEBUG
		if (psAI_Flags.test(aiALife)) {
			Msg						("[LSS] There is no interaction");
		}
#endif
		return						(false);
	}
	else
		return						(true);
}

bool CALifeCombatManager::bfCheckIfRetreated(int iCombatGroupIndex)
{
	ai().ef_storage().alife_evaluation(true);
	ai().ef_storage().alife().member_item()	= (eCombatTypeMonsterMonster == combat_type()) ? smart_cast<CSE_ALifeObject*>(m_tpaCombatGroups[iCombatGroupIndex][0]) : m_tpaCombatGroups[iCombatGroupIndex][0]->m_tpBestDetector;
	ai().ef_storage().alife().member()	= m_tpaCombatGroups[iCombatGroupIndex][0];
	ai().ef_storage().alife().enemy()		= m_tpaCombatGroups[iCombatGroupIndex ^ 1][0];
	return										(randF(100) < ((eCombatTypeMonsterMonster != combat_type()) ? ai().ef_storage().m_pfAnomalyRetreatProbability->ffGetValue() : ai().ef_storage().m_pfEnemyRetreatProbability->ffGetValue()));
}

void CALifeCombatManager::vfPerformAttackAction(int iCombatGroupIndex)
{
	ai().ef_storage().alife_evaluation(true);
	SCHEDULE_P_VECTOR		&l_tCombatGroup = m_tpaCombatGroups[iCombatGroupIndex];
	SCHEDULE_P_IT			I = l_tCombatGroup.begin();
	SCHEDULE_P_IT			E = l_tCombatGroup.end();
	for ( ; I != E; ++I) {
		EHitType			l_tHitType = eHitTypeMax;
		float				l_fHitPower = 0.f;
		if (!(*I)->m_tpCurrentBestWeapon) {
			CSE_ALifeItemWeapon	*l_tpALifeItemWeapon = (*I)->tpfGetBestWeapon(l_tHitType,l_fHitPower);
			if (!l_tpALifeItemWeapon && (l_fHitPower <= EPS_L))
				continue;
		}
		else {
			l_tHitType		= (*I)->m_tpCurrentBestWeapon->m_tHitType;
			l_fHitPower		= (*I)->m_tpCurrentBestWeapon->m_fHitPower;
		}
		
		ai().ef_storage().alife().member_item() = smart_cast<CSE_ALifeObject*>(*I);
		ai().ef_storage().alife().member() = *I;
#ifdef DEBUG
		if (psAI_Flags.test(aiALife)) {
			Msg				("[LSS] %s attacks with %s(%d ammo) %d times in a row",(*I)->base()->name_replace(),(*I)->m_tpCurrentBestWeapon ? (*I)->m_tpCurrentBestWeapon->name_replace() : "its natural weapon",(*I)->m_tpCurrentBestWeapon ? (*I)->m_tpCurrentBestWeapon->m_dwAmmoAvailable : 0,iFloor(ai().ef_storage().m_pfWeaponAttackTimes->ffGetValue() + .5f));
		}
#endif
		for (int i=0, n=iFloor(ai().ef_storage().m_pfWeaponAttackTimes->ffGetValue() + .5f); i<n; ++i) {
			if (randF(100) < ai().ef_storage().m_pfWeaponSuccessProbability->ffGetValue()) {
				// choose random enemy group member and perform hit with random power
				// multiplied by immunity factor
				int							l_iIndex = randI(m_tpaCombatGroups[iCombatGroupIndex ^ 1].size());
				CSE_ALifeMonsterAbstract	*l_tpALifeMonsterAbstract = smart_cast<CSE_ALifeMonsterAbstract*>(m_tpaCombatGroups[iCombatGroupIndex ^ 1][l_iIndex]);
				R_ASSERT2					(l_tpALifeMonsterAbstract,"Invalid combat object");
				float						l_fHit = randF(l_fHitPower*0.5f,l_fHitPower*1.5f);
				l_tpALifeMonsterAbstract->fHealth -= l_tpALifeMonsterAbstract->m_fpImmunityFactors[l_tHitType]*l_fHit;
#ifdef DEBUG
				if (psAI_Flags.test(aiALife)) {
					Msg						("[LSS] %s %s %s [power %5.2f][damage %5.2f][health %5.2f][creatures left %d]",(*I)->base()->name_replace(),l_tpALifeMonsterAbstract->fHealth <= 0 ? "killed" : "attacked",l_tpALifeMonsterAbstract->name_replace(),l_fHit,l_tpALifeMonsterAbstract->m_fpImmunityFactors[l_tHitType]*l_fHit,_max(l_tpALifeMonsterAbstract->fHealth,0.f),l_tpALifeMonsterAbstract->fHealth >= EPS_L ? m_tpaCombatGroups[iCombatGroupIndex ^ 1].size() : m_tpaCombatGroups[iCombatGroupIndex ^ 1].size() - 1);
				}
#endif
				// check if victim became dead
				if (l_tpALifeMonsterAbstract->fHealth <= 0) {
					m_tpaCombatGroups[iCombatGroupIndex ^ 1].erase(m_tpaCombatGroups[iCombatGroupIndex ^ 1].begin() + l_iIndex);
					if (m_tpaCombatGroups[iCombatGroupIndex ^ 1].empty())
						return;
				}
			}
			else {
#ifdef DEBUG
				if (psAI_Flags.test(aiALife)) {
					Msg		("[LSS] %s missed",(*I)->base()->name_replace());
				}
#endif
			}
			// perform attack (if we use a weapon we should delete ammo we used)
			if (!(*I)->bfPerformAttack())
				break;
		}
	}
}

void CALifeCombatManager::vfFinishCombat(ECombatResult tCombatResult)
{
	// processing weapons and dead monsters
	CSE_ALifeDynamicObject	*l_tpALifeDynamicObject = smart_cast<CSE_ALifeDynamicObject*>(m_tpaCombatObjects[0]);
	R_ASSERT2				(l_tpALifeDynamicObject,"Unknown schedulable object class");
	GameGraph::_GRAPH_ID	l_tGraphID = l_tpALifeDynamicObject->m_tGraphID;
	m_temp_item_vector.clear();
	for (int i=0; i<2; ++i) {
		CSE_ALifeGroupAbstract	*l_tpALifeGroupAbstract = smart_cast<CSE_ALifeGroupAbstract*>(m_tpaCombatObjects[i]);
		if (l_tpALifeGroupAbstract) {
			for (int I=0, N=l_tpALifeGroupAbstract->m_tpMembers.size() ; I<N; ++I) {
				CSE_ALifeMonsterAbstract	*l_tpALifeMonsterAbstract = smart_cast<CSE_ALifeMonsterAbstract*>(objects().object(l_tpALifeGroupAbstract->m_tpMembers[I]));
				R_ASSERT2					(l_tpALifeMonsterAbstract,"Invalid group member!");
				l_tpALifeMonsterAbstract->vfUpdateWeaponAmmo	();
				if (l_tpALifeMonsterAbstract->fHealth <= EPS_L) {
					append_item_vector							(l_tpALifeMonsterAbstract->children,m_temp_item_vector);
					l_tpALifeMonsterAbstract->m_bDirectControl	= true;
					l_tpALifeGroupAbstract->m_tpMembers.erase	(l_tpALifeGroupAbstract->m_tpMembers.begin() + I);
					assign_death_position						(l_tpALifeMonsterAbstract, l_tGraphID, m_tpaCombatObjects[i ^ 1]);
					l_tpALifeMonsterAbstract->vfDetachAll		();
					R_ASSERT									(l_tpALifeMonsterAbstract->children.empty());
					register_object								(l_tpALifeMonsterAbstract);
					CSE_ALifeInventoryItem *l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(l_tpALifeMonsterAbstract);
					if (l_tpALifeInventoryItem)
						m_temp_item_vector.push_back			(l_tpALifeInventoryItem);
					--l_tpALifeGroupAbstract->m_wCount;
					--I;
					--N;
				}
			}
		}
		else {
			m_tpaCombatObjects[i]->vfUpdateWeaponAmmo			();
			CSE_ALifeMonsterAbstract							*l_tpALifeMonsterAbstract = smart_cast<CSE_ALifeMonsterAbstract*>(m_tpaCombatObjects[i]);
			if (l_tpALifeMonsterAbstract && (l_tpALifeMonsterAbstract->fHealth <= EPS_L)) {
				kill_entity										(l_tpALifeMonsterAbstract,l_tGraphID,m_tpaCombatObjects[i ^ 1]);
			}
		}
	}
	
	if (m_temp_item_vector.empty() || (eCombatTypeMonsterMonster != combat_type())) {
#ifdef DEBUG
		if (psAI_Flags.test(aiALife)) {
			Msg							("[LSS] There is nothing to take");
		}
#endif
		return;
	}

	int			l_iGroupIndex = -1;
	switch (tCombatResult) {
		case eCombatResultBothKilled	:
		case eCombatResultRetreat12		: break;
		case eCombatResultRetreat2		:
		case eCombatResult1Kill2		: {
			l_iGroupIndex				= 0;
			break;
		}
		case eCombatResultRetreat1		:
		case eCombatResult2Kill1		: {
			l_iGroupIndex				= 1;
			break;
		}
		default							: NODEFAULT;
	}
	
	if (l_iGroupIndex >= 0) {
#ifdef DEBUG
		if (psAI_Flags.test(aiALife)) {
			Msg							("[LSS] Starting taking items [%s][%f]",m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace(),smart_cast<CSE_ALifeMonsterAbstract*>(m_tpaCombatObjects[l_iGroupIndex])->fHealth);
		}
#endif
		m_tpaCombatObjects[l_iGroupIndex]->vfAttachItems();
	}
	m_temp_item_vector.clear();
}

ALife::ERelationType	CALifeCombatManager::relation_type	(CSE_ALifeMonsterAbstract *tpALifeMonsterAbstract1, CSE_ALifeMonsterAbstract *tpALifeMonsterAbstract2) const
{
	CSE_ALifeTraderAbstract* human1 = smart_cast<CSE_ALifeTraderAbstract*>(tpALifeMonsterAbstract1);
	CSE_ALifeTraderAbstract* human2 = smart_cast<CSE_ALifeTraderAbstract*>(tpALifeMonsterAbstract2);

	if(human1 && human2)
	{
		ALife::ERelationType rel = RELATION_REGISTRY().GetRelationBetween(human1, human2);
		return rel;
	}

	if (tpALifeMonsterAbstract1->g_team() != tpALifeMonsterAbstract2->g_team())
		return(ALife::eRelationTypeEnemy);
	else
		return(ALife::eRelationTypeNeutral);
}
/**/
void CALifeCombatManager::kill_entity	(CSE_ALifeMonsterAbstract *l_tpALifeMonsterAbstract, const GameGraph::_GRAPH_ID &l_tGraphID, CSE_ALifeSchedulable *schedulable)
{
	VERIFY									(l_tpALifeMonsterAbstract->g_Alive());
	append_item_vector						(l_tpALifeMonsterAbstract->children,m_temp_item_vector);
	GameGraph::_GRAPH_ID					l_tGraphID1 = l_tpALifeMonsterAbstract->m_tGraphID;
	assign_death_position					(l_tpALifeMonsterAbstract, l_tGraphID, schedulable);
	l_tpALifeMonsterAbstract->vfDetachAll	();
	R_ASSERT								(l_tpALifeMonsterAbstract->children.empty());
	scheduled().remove						(l_tpALifeMonsterAbstract);
	if (l_tpALifeMonsterAbstract->m_tGraphID != l_tGraphID1) {
		graph().remove						(l_tpALifeMonsterAbstract,l_tGraphID1);
		graph().add							(l_tpALifeMonsterAbstract,l_tpALifeMonsterAbstract->m_tGraphID);
	}
	CSE_ALifeInventoryItem *l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(l_tpALifeMonsterAbstract);
	if (l_tpALifeInventoryItem)
		m_temp_item_vector.push_back		(l_tpALifeInventoryItem);
}
