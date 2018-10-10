#include "StdAfx.h"
#include "anomaly_detector.h"
#include "basemonster/base_monster.h"
#include "restricted_object.h"
#include "CustomZone.h"
#include "Level.h"
#include "space_restriction_manager.h"

CAnomalyDetector::CAnomalyDetector(CBaseMonster* monster) : m_object(monster) {}
CAnomalyDetector::~CAnomalyDetector() {}
void CAnomalyDetector::load(LPCSTR section)
{
    m_radius = READ_IF_EXISTS(pSettings, r_float, section, "Anomaly_Detect_Radius", 15.f);
    m_time_to_rememeber = READ_IF_EXISTS(pSettings, r_u32, section, "Anomaly_Detect_Time_Remember", 30000);
}

void CAnomalyDetector::reinit()
{
    m_storage.clear();

    m_active = false;
}

void CAnomalyDetector::update_schedule()
{
    if (m_active)
        m_object->feel_touch_update(m_object->Position(), m_radius);

    if (m_storage.empty())
        return;

    xr_vector<u16> temp_out_restrictors;
    xr_vector<u16> temp_in_restrictors;

    temp_in_restrictors.reserve(m_storage.size());

    // add new restrictions
    for (auto it = m_storage.begin(); it != m_storage.end(); it++)
    {
        if (it->time_registered == 0)
        {
            temp_in_restrictors.push_back(it->object->ID());
            it->time_registered = time();
        }
    }

    m_object->control().path_builder().restrictions().add_restrictions(temp_out_restrictors, temp_in_restrictors);

    // remove old restrictions
    temp_in_restrictors.clear();
    for (auto it = m_storage.begin(); it != m_storage.end(); it++)
    {
        if (it->time_registered + m_time_to_rememeber < time())
        {
            temp_in_restrictors.push_back(it->object->ID());
        }
    }

    m_object->control().path_builder().restrictions().remove_restrictions(temp_out_restrictors, temp_in_restrictors);

    // remove from storage
    m_storage.erase(
        std::remove_if(m_storage.begin(), m_storage.end(), remove_predicate(m_time_to_rememeber)), m_storage.end());
}

void CAnomalyDetector::on_contact(IGameObject* obj)
{
    if (!m_active)
        return;

    CCustomZone* custom_zone = smart_cast<CCustomZone*>(obj);
    if (!custom_zone)
        return;

    // if its NOT A restrictor - skip
    if (custom_zone->restrictor_type() == RestrictionSpace::eRestrictorTypeNone)
        return;

    if (Level().space_restriction_manager().restriction_presented(
            m_object->control().path_builder().restrictions().in_restrictions(), custom_zone->cName()))
        return;

    auto it = std::find(m_storage.begin(), m_storage.end(), custom_zone);
    if (it != m_storage.end())
        return;

    SAnomalyInfo info;
    info.object = obj;
    info.time_registered = 0;
    m_storage.push_back(info);
}
