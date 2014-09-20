////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_combat_manager.h
//	Created 	: 12.08.2003
//  Modified 	: 14.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife combat manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_simulator_base.h"
//#include "game_graph_space.h"

class CALifeCombatManager : public virtual CALifeSimulatorBase, CRandom {
/**
protected:
	typedef CALifeSimulatorBase inherited;

protected:
	u32								m_dwMaxCombatIterationCount;
	ALife::ECombatType				m_combat_type;

	// temporary buffers for combats
	CSE_ALifeSchedulable			*m_tpaCombatObjects[2];
	ALife::D_OBJECT_P_MAP			m_tpGraphPointObjects;

public:
	ALife::ITEM_P_VECTOR			m_tpTempItemBuffer;

protected:
			void					vfFillCombatGroup			(CSE_ALifeSchedulable		*tpALifeSchedulable,		int						iGroupIndex);
			bool					bfCheckObjectDetection		(CSE_ALifeSchedulable		*tpALifeSchedulable1,		CSE_ALifeSchedulable	*tpALifeSchedulable2);
			bool					bfCheckForInteraction		(CSE_ALifeSchedulable		*tpALifeSchedulable1,		CSE_ALifeSchedulable	*tpALifeSchedulable2,			int				&iCombatGroupIndex,			bool					&bMutualDetection);
			void					vfPerformAttackAction		(int						iCombatGroupIndex);
			bool					bfCheckIfRetreated			(int						iCombatGroupIndex);
			void					vfFinishCombat				(ALife::ECombatResult		tCombatResult);
/**/
public:
									CALifeCombatManager			(xrServer *server, LPCSTR section);
/**
	virtual							~CALifeCombatManager		();
	IC		ALife::ECombatType		combat_type					() const;
			ALife::ECombatAction	choose_combat_action		(int						iCombatGroupIndex);
			ALife::ERelationType	relation_type				(CSE_ALifeMonsterAbstract	*tpALifeMonsterAbstract1,	CSE_ALifeMonsterAbstract*tpALifeMonsterAbstract2) const;
/**/
			void					kill_entity					(CSE_ALifeMonsterAbstract	*l_tpALifeMonsterAbstract,	const GameGraph::_GRAPH_ID &l_tGraphID,					CSE_ALifeSchedulable *schedulable);
};

#include "alife_combat_manager_inline.h"