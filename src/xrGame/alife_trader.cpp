////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_trader.cpp
//	Created 	: 03.09.2003
//  Modified 	: 03.09.2003
//	Author		: Dmitriy Iassenev
//	Description : ALife trader class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"

void CSE_ALifeTrader::spawn_supplies()
{
    inherited1::spawn_supplies();
    inherited2::spawn_supplies();
}

u32 CSE_ALifeTrader::dwfGetItemCost(CSE_ALifeInventoryItem* tpALifeInventoryItem)
{
#pragma todo("Dima to Dima : correct price for non-artefact objects")
    CSE_ALifeItemArtefact* l_tpALifeItemArtefact = smart_cast<CSE_ALifeItemArtefact*>(tpALifeInventoryItem);
    if (!l_tpALifeItemArtefact)
        return (tpALifeInventoryItem->m_dwCost);

    u32 l_dwPurchasedCount = 0;
#pragma todo("Dima to Dima : optimize this cycle by keeping additional data structure with bought items")
    {
        ALife::OBJECT_IT i = children.begin();
        ALife::OBJECT_IT e = children.end();
        for (; i != e; ++i)
            if (!xr_strcmp(ai().alife().objects().object(*i)->s_name, l_tpALifeItemArtefact->s_name))
                ++l_dwPurchasedCount;
    }
    return (tpALifeInventoryItem->m_dwCost);
}

void CSE_ALifeTrader::add_online(const bool& update_registries)
{
    CSE_ALifeTraderAbstract::add_online(update_registries);
}

void CSE_ALifeTrader::add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries)
{
    CSE_ALifeTraderAbstract::add_offline(saved_children, update_registries);
}
