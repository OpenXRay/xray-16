////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_switch_manager.h
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator switch manager
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "alife_simulator_base.h"

// XXX: WTF is this? CALifeSwitchManager IS-A CRandom??? I think NOT! CRandom should be aggregated, NOT inherited from!
class CALifeSwitchManager : public virtual CALifeSimulatorBase, CRandom
{
protected:
    typedef CALifeSimulatorBase inherited;
    typedef ALife::OBJECT_VECTOR OBJECT_VECTOR;

protected:
    float m_switch_distance;
    float m_switch_factor;
    float m_online_distance;
    float m_offline_distance;

private:
    OBJECT_VECTOR m_saved_chidren;

protected:
    bool synchronize_location(CSE_ALifeDynamicObject* object);

public:
    void try_switch_online(CSE_ALifeDynamicObject* object);
    void try_switch_offline(CSE_ALifeDynamicObject* object);
    void switch_online(CSE_ALifeDynamicObject* object);
    void switch_offline(CSE_ALifeDynamicObject* object);
    void remove_online(CSE_ALifeDynamicObject* object, bool update_registries = true);
    void add_online(CSE_ALifeDynamicObject* object, bool update_registries = true);

public:
    IC CALifeSwitchManager(IPureServer* server, LPCSTR section);
    virtual ~CALifeSwitchManager();
    void switch_object(CSE_ALifeDynamicObject* object);
    IC float online_distance() const noexcept;
    IC float offline_distance() const noexcept;
    IC float switch_distance() const noexcept;
    IC void set_switch_distance(float switch_distance) noexcept;
    IC void set_switch_factor(float switch_factor) noexcept;
};

#include "alife_switch_manager_inline.h"
