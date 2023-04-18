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

CSE_ALifeItemWeapon* CSE_ALifeAnomalousZone::tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower)
{
    m_tpCurrentBestWeapon = 0;
    m_tTimeID = ai().alife().time_manager().game_time();
    fHitPower = m_maxPower;
    tHitType = m_tHitType;
    return (m_tpCurrentBestWeapon);
}

ALife::EMeetActionType CSE_ALifeAnomalousZone::tfGetActionType(
    CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex, bool bMutualDetection)
{
    return (ALife::eMeetActionTypeAttack);
}

bool CSE_ALifeAnomalousZone::bfActive() { return fis_zero(m_maxPower,EPS_L) || !interactive(); }

CSE_ALifeDynamicObject* CSE_ALifeAnomalousZone::tpfGetBestDetector()
{
    VERIFY2(false, "This function shouldn't be called");
    NODEFAULT;
#ifdef DEBUG
    return (0);
#endif
}

void CSE_ALifeAnomalousZone::spawn_artefacts()
{
    VERIFY2(!m_bOnline,"Cannot spawn artefacts in online!");
    {
        const float min_start_power = pSettings->read_if_exists<float>(name(), "min_start_power", 0.0f);
        const float max_start_power = pSettings->read_if_exists<float>(name(), "max_start_power", 1.0f);

        m_maxPower = min_start_power;
        if (!fsimilar(min_start_power, max_start_power))
            m_maxPower = randF(min_start_power, max_start_power);
    }

    const u32 min_artefact_count = pSettings->read_if_exists<u32>(name(), "min_artefact_count", 0);
    const u32 max_artefact_count = pSettings->read_if_exists<u32>(name(), "max_artefact_count", 0);

    u32 artefact_count = min_artefact_count;
    if (min_artefact_count != max_artefact_count)
        artefact_count = randI((s32)min_artefact_count, (s32)max_artefact_count);

    if (artefact_count == 0)
        return;

    cpcstr artefacts = pSettings->read_if_exists<pcstr>(name(), "artefacts", nullptr);
    if (!artefacts)
    {
        Msg("! Artefact count and start power parameters are defined for [%s], but [artefacts] string is missing. Skipping.", name());
        return;
    }

    u32 n = _GetItemCount(artefacts);
    VERIFY2(!(n % 2), "Invalid parameters count in line artefacts for anomalous zone");
    n /= 2;

    typedef std::pair<shared_str, float> Weight;
    typedef buffer_vector<Weight> Weights;
    Weights weights(xr_alloca(n * sizeof(Weight)), n);

    u32 missing_artefacts = 0;
    for (u32 i = 0; i < n; ++i)
    {
        string256 temp0, temp1;
        _GetItem(artefacts, 2 * i + 0, temp0);
        if (!pSettings->section_exist(temp0))
        {
#ifndef MASTER_GOLD
            Msg("! [%s] wants to spawn artefact [%s] with missing INI section.", name(), temp0);
#endif
            ++missing_artefacts;
            continue;
        }
        _GetItem(artefacts, 2 * i + 1, temp1);
        weights.push_back(
            std::make_pair(
                temp0,
                (float)atof(temp1)
                )
            );
    }
    n -= missing_artefacts;

    for (u32 i = 0; i < artefact_count; ++i)
    {
        const float fProbability = randF(1.f);
        float fSum = 0.f;
        u32 p = 0;
        for (; p < n; ++p)
        {
            fSum += weights[p].second;
            if (fSum > fProbability)
                break;
        }
        if (p < n)
        {
            CSE_Abstract* l_tpSE_Abstract = alife().spawn_item(*weights[p].first, position(), m_tNodeID, m_tGraphID, 0xffff);
            R_ASSERT3(l_tpSE_Abstract, "Can't spawn artefact ", *weights[p].first);
            CSE_ALifeDynamicObject* object = smart_cast<CSE_ALifeDynamicObject*>(l_tpSE_Abstract);
            R_ASSERT2(object, "Non-ALife object in the 'game.spawn'");

            object->m_tSpawnID = m_tSpawnID;
            object->m_bALifeControl = true;
            ai().alife().spawns().assign_artefact_position(this, object);

            const Fvector t = object->o_Position;
            const u32 p = object->m_tNodeID;
            const float q = object->m_fDistance;
            alife().graph().change(object, m_tGraphID, object->m_tGraphID);
            object->o_Position = t;
            object->m_tNodeID = p;
            object->m_fDistance = q;

            CSE_ALifeItemArtefact* l_tpALifeItemArtefact = smart_cast<CSE_ALifeItemArtefact*>(object);
            R_ASSERT2(l_tpALifeItemArtefact,
                "Anomalous zone can't generate non-artefact objects since they don't have an 'anomaly property'!");

            l_tpALifeItemArtefact->m_fAnomalyValue = m_maxPower * (1.f - object->o_Position.distance_to(o_Position) /
                m_offline_interactive_radius);
        }
    }
}

void CSE_ALifeAnomalousZone::on_spawn()
{
    inherited::on_spawn();
    spawn_artefacts();
}

bool CSE_ALifeAnomalousZone::keep_saved_data_anyway() const /* noexcept */ { return true; }
