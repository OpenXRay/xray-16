////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_communication_manager.cpp
//	Created 	: 03.09.2003
//  Modified 	: 14.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife communication manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_interaction_manager.h"
/**
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_graph_registry.h"
#include "alife_time_manager.h"

using namespace ALife;

/**/
CALifeInteractionManager::CALifeInteractionManager(IPureServer* server, LPCSTR section)
    : CALifeCombatManager(server, section), CALifeCommunicationManager(server, section),
      CALifeSimulatorBase(server, section)
{
    /**
        m_inventory_slot_count		= pSettings->r_u32("inventory","slots");
        m_temp_weapons.resize		(m_inventory_slot_count);
        m_temp_marks.assign			(u16(-1),false);
    /**/
}

/**
CALifeInteractionManager::~CALifeInteractionManager()
{
}

void CALifeInteractionManager::check_for_interaction(CSE_ALifeSchedulable *tpALifeSchedulable)
{
    if (!tpALifeSchedulable->bfActive())
        return;

    CSE_ALifeDynamicObject		*l_tpALifeDynamicObject = smart_cast<CSE_ALifeDynamicObject*>(tpALifeSchedulable);
    R_ASSERT2					(l_tpALifeDynamicObject,"Unknown schedulable object class");
    GameGraph::_GRAPH_ID		l_tGraphID = l_tpALifeDynamicObject->m_tGraphID;
    check_for_interaction		(tpALifeSchedulable,l_tGraphID);

    CGameGraph::const_iterator	I, E;
    ai().game_graph().begin		(l_tGraphID,I,E);
    for ( ; I != E; ++I)
        check_for_interaction	(tpALifeSchedulable,(*I).vertex_id());
}

class CCheckForInteractionPredicate {
public:
    CALifeInteractionManager	*	manager;
    mutable CSE_ALifeSchedulable	*tpALifeSchedulable;
    mutable GameGraph::_GRAPH_ID	tGraphID;
    mutable int						l_iGroupIndex;
    mutable bool					l_bMutualDetection;
    mutable CSE_ALifeHumanAbstract	*l_tpALifeHumanAbstract;
    mutable CSE_ALifeMonsterAbstract*l_tpALifeMonsterAbstract;

    IC	CCheckForInteractionPredicate(CALifeInteractionManager *manager, CSE_ALifeSchedulable *tpALifeSchedulable,
GameGraph::_GRAPH_ID tGraphID) :
        manager(manager),
        tpALifeSchedulable(tpALifeSchedulable),
        tGraphID(tGraphID)
    {
        l_tpALifeHumanAbstract	= smart_cast<CSE_ALifeHumanAbstract*>(tpALifeSchedulable);
        l_tpALifeMonsterAbstract= smart_cast<CSE_ALifeMonsterAbstract*>(tpALifeSchedulable);
        manager->vfFillCombatGroup	(tpALifeSchedulable,0);
    }

    IC	bool operator()	(CALifeGraphRegistry::OBJECT_REGISTRY::_iterator &I, u64 counter, bool) const
    {
        if (counter == (*I).second->m_switch_counter)
            return				(false);
        return					(!manager->m_tpaCombatGroups[0].empty());
    }

    IC	void operator()	(CALifeGraphRegistry::OBJECT_REGISTRY::_iterator &I, u64 counter) const
    {
        (*I).second->m_switch_counter = counter;

        VERIFY					(!manager->m_tpaCombatGroups[0].empty());

        if ((*I).first == tpALifeSchedulable->base()->ID)
            return;

        CSE_ALifeSchedulable	*l_tpALifeSchedulable = smart_cast<CSE_ALifeSchedulable*>((*I).second);
        if (!l_tpALifeSchedulable)
            return;

        if (!manager->bfCheckForInteraction(tpALifeSchedulable,l_tpALifeSchedulable,l_iGroupIndex,l_bMutualDetection))
            return;

        manager->vfFillCombatGroup		(l_tpALifeSchedulable,1);

        switch (manager->m_tpaCombatObjects[l_iGroupIndex]->tfGetActionType(manager->m_tpaCombatObjects[l_iGroupIndex ^
1],l_iGroupIndex,l_bMutualDetection)) {
            case eMeetActionTypeAttack : {
#ifdef DEBUG
                if (psAI_Flags.test(aiALife)) {
                    Msg("[LSS] %s started combat versus
%s",manager->m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace(),manager->m_tpaCombatObjects[l_iGroupIndex ^
1]->base()->name_replace());
                }
#endif
                ECombatResult	l_tCombatResult = eCombatResultRetreat12;
                bool					l_bDoNotContinue = false;
                for (int i=0; i<2*int(manager->m_dwMaxCombatIterationCount); ++i) {
                    if (eCombatActionAttack == manager->choose_combat_action(l_iGroupIndex)) {
#ifdef DEBUG
                        if (psAI_Flags.test(aiALife)) {
                            Msg("[LSS] %s choosed to attack
%s",manager->m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace(),manager->m_tpaCombatObjects[l_iGroupIndex ^
1]->base()->name_replace());
                        }
#endif
                        manager->vfPerformAttackAction(l_iGroupIndex);

                        l_bDoNotContinue = false;
                    }
                    else {
                        if (l_bDoNotContinue)
                            break;
#ifdef DEBUG
                        if (psAI_Flags.test(aiALife)) {
                            Msg("[LSS] %s choosed to retreat from
%s",manager->m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace(),manager->m_tpaCombatObjects[l_iGroupIndex ^
1]->base()->name_replace());
                        }
#endif
                        if (manager->bfCheckIfRetreated(l_iGroupIndex)) {
#ifdef DEBUG
                            if (psAI_Flags.test(aiALife)) {
                                Msg("[LSS] %s did retreat from
%s",manager->m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace(),manager->m_tpaCombatObjects[l_iGroupIndex ^
1]->base()->name_replace());
                            }
#endif
                            l_tCombatResult	= l_iGroupIndex ? eCombatResultRetreat2 : eCombatResultRetreat1;
                            break;
                        }
                        l_bDoNotContinue = true;
#ifdef DEBUG
                        if (psAI_Flags.test(aiALife)) {
                            Msg("[LSS] %s didn't retreat from
%s",manager->m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace(),manager->m_tpaCombatObjects[l_iGroupIndex ^
1]->base()->name_replace());
                        }
#endif
                    }

                    l_iGroupIndex		^= 1;

                    if (manager->m_tpaCombatGroups[l_iGroupIndex].empty()) {
#ifdef DEBUG
                        if (psAI_Flags.test(aiALife)) {
                            Msg("[LSS] %s is dead",manager->m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace());
                        }
#endif
                        l_tCombatResult	= l_iGroupIndex ? eCombatResult1Kill2 : eCombatResult2Kill1;
                        break;
                    }
                }
#ifdef DEBUG
                if (psAI_Flags.test(aiALife)) {
                    if (eCombatResultRetreat12 == l_tCombatResult)
                        Msg("[LSS] both combat groups decided not to continue combat");
                }
#endif
                manager->vfFinishCombat	(l_tCombatResult);
                break;
            }
            case eMeetActionTypeInteract : {
                R_ASSERT2				(l_tpALifeHumanAbstract,"Non-human objects сannot communicate with each other");
                CSE_ALifeHumanAbstract	*l_tpALifeHumanAbstract2 =
smart_cast<CSE_ALifeHumanAbstract*>(l_tpALifeSchedulable);
                R_ASSERT2				(l_tpALifeHumanAbstract2,"Non-human objects сannot communicate with each
other");
#ifdef DEBUG
                if (psAI_Flags.test(aiALife)) {
                    Msg					("[LSS] %s interacted with
%s",manager->m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace(),manager->m_tpaCombatObjects[l_iGroupIndex ^
1]->base()->name_replace());
                }
#endif
                manager->vfPerformCommunication	();
                break;
            }
            case eMeetActionTypeIgnore : {
#ifdef DEBUG
                if (psAI_Flags.test(aiALife)) {
                    Msg					("[LSS] %s refused from
combat",manager->m_tpaCombatObjects[l_iGroupIndex]->base()->name_replace());
                }
#endif
                return;
            }
            case eMeetActionSmartTerrain : {
                CSE_ALifeSmartZone		*smart_zone = smart_cast<CSE_ALifeSmartZone*>(l_tpALifeSchedulable);
                VERIFY					(smart_zone);
                VERIFY					(l_tpALifeMonsterAbstract);
                smart_zone->smart_touch	(l_tpALifeMonsterAbstract);
                return;
            }
            default : NODEFAULT;
        }
    }
};

void CALifeInteractionManager::check_for_interaction(CSE_ALifeSchedulable *tpALifeSchedulable, GameGraph::_GRAPH_ID
tGraphID)
{
    graph().iterate_objects(
        tGraphID,
        CCheckForInteractionPredicate(
            this,
            tpALifeSchedulable,
            tGraphID
        )
    );
}
/**/
