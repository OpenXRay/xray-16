////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_communication_manager.h
//	Created 	: 03.09.2003
//  Modified 	: 14.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife communication manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrserver_space.h"
#include "alife_combat_manager.h"
#include "alife_communication_manager.h"

class CALifeInteractionManager : 
	public CALifeCombatManager,
	public CALifeCommunicationManager
{
/**
	friend class CCheckForInteractionPredicate;
protected:
	u32								m_inventory_slot_count;

public:
	BOOL_VECTOR						m_temp_marks;
	ALife::WEAPON_P_VECTOR			m_temp_weapons;	

/**/
public:
									CALifeInteractionManager	(xrServer *server, LPCSTR section);
/**
	virtual							~CALifeInteractionManager	();
			void					check_for_interaction		(CSE_ALifeSchedulable		*tpALifeSchedulable);
			void					check_for_interaction		(CSE_ALifeSchedulable		*tpALifeSchedulable,		GameGraph::_GRAPH_ID		tGraphID);
/**/
};