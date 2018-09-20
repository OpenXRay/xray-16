////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_anomalous_zone.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife anomalous zone class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_time_manager.h"
#include "alife_spawn_registry.h"
#include "alife_graph_registry.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

CSE_ALifeItemWeapon* CSE_ALifeAnomalousZone::tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower)
{
    m_tpCurrentBestWeapon = 0;
    m_tTimeID = ai().alife().time_manager().game_time();
    fHitPower = 1.0f; // m_maxPower;
    tHitType = m_tHitType;
    return (m_tpCurrentBestWeapon);
}

ALife::EMeetActionType CSE_ALifeAnomalousZone::tfGetActionType(
    CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex, bool bMutualDetection)
{
    return (ALife::eMeetActionTypeAttack);
}

bool CSE_ALifeAnomalousZone::bfActive() { return (false /*fis_zero(m_maxPower,EPS_L)*/ || !interactive()); }
CSE_ALifeDynamicObject* CSE_ALifeAnomalousZone::tpfGetBestDetector()
{
    VERIFY2(false, "This function shouldn't be called");
    NODEFAULT;
#ifdef DEBUG
    return (0);
#endif
}
/*
void CSE_ALifeAnomalousZone::spawn_artefacts				()
{
    VERIFY2					(!m_bOnline,"Cannot spawn artefacts in online!");

    float					m_min_start_power	= pSettings->r_float(name(),"min_start_power");
    float					m_max_start_power	= pSettings->r_float(name(),"max_start_power");
    u32						m_min_artefact_count= pSettings->r_u32	(name(),"min_artefact_count");;
    u32						m_max_artefact_count= pSettings->r_u32	(name(),"max_artefact_count");;
    u32						m_artefact_count;

    if (m_min_artefact_count == m_max_artefact_count)
        m_artefact_count	= m_min_artefact_count;
    else
        m_artefact_count	= randI(m_min_artefact_count,m_max_artefact_count);

    if (m_min_start_power == m_max_start_power)
        m_maxPower			= m_min_start_power;
    else
        m_maxPower			= randF(m_min_start_power,m_max_start_power);

    LPCSTR					artefacts = pSettings->r_string(name(),"artefacts");
    u32						n = _GetItemCount(artefacts);
    VERIFY2					(!(n % 2),"Invalid parameters count in line artefacts for anomalous zone");
    n						>>= 1;

    typedef std::pair<shared_str,float>	Weight;
    typedef buffer_vector<Weight>		Weights;
    Weights weights			(
        _alloca(n*sizeof(Weight)),
        n
    );

    for (u32 i=0; i<n; ++i) {
        string256			temp0, temp1;
        _GetItem			( artefacts, 2*i + 0, temp0 );
        _GetItem			( artefacts, 2*i + 1, temp1 );
        weights.push_back	(
            std::make_pair(
                temp0,
                (float)atof(temp1)
            )
        );
    }

    for (u32 ii=0; ii<m_artefact_count; ++ii) {
        float fProbability		= randF(1.f);
        float fSum				= 0.f;
        for (u16 p=0; p<n; ++p) {
            fSum				+= weights[p].second;
            if (fSum > fProbability)
                break;
        }
        if (p < n) {
            CSE_Abstract		*l_tpSE_Abstract =
alife().spawn_item(*weights[p].first,position(),m_tNodeID,m_tGraphID,0xffff);
            R_ASSERT3			(l_tpSE_Abstract,"Can't spawn artefact ",*weights[p].first);
            CSE_ALifeDynamicObject	*i = smart_cast<CSE_ALifeDynamicObject*>(l_tpSE_Abstract);
            R_ASSERT2			(i,"Non-ALife object in the 'game.spawn'");

            i->m_tSpawnID		= m_tSpawnID;
            i->m_bALifeControl	= true;
            ai().alife().spawns().assign_artefact_position(this,i);

            Fvector				t = i->o_Position	;
            u32					p = i->m_tNodeID	;
            float				q = i->m_fDistance	;
            alife().graph().change(i,m_tGraphID,i->m_tGraphID);
            i->o_Position		= t;
            i->m_tNodeID		= p;
            i->m_fDistance		= q;

            CSE_ALifeItemArtefact *l_tpALifeItemArtefact = smart_cast<CSE_ALifeItemArtefact*>(i);
            R_ASSERT2		(l_tpALifeItemArtefact,"Anomalous zone can't generate non-artefact objects since they don't
have an 'anomaly property'!");

            l_tpALifeItemArtefact->m_fAnomalyValue = m_maxPower*(1.f -
i->o_Position.distance_to(o_Position)/m_offline_interactive_radius);
        }
    }
}*/

void CSE_ALifeAnomalousZone::on_spawn()
{
    inherited::on_spawn();
    //	spawn_artefacts			();
}

bool CSE_ALifeAnomalousZone::keep_saved_data_anyway() const /* noexcept */ { return true; }
