////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_surge_manager.h
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator surge manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_simulator_base.h"
#include "xrServer_Space.h"

class CSE_ALifeTrader;

class CALifeSurgeManager : public virtual CALifeSimulatorBase
{
protected:
    typedef CALifeSimulatorBase inherited;

protected:
    xr_vector<ALife::_SPAWN_ID> m_temp_spawns;
    xr_vector<ALife::_SPAWN_ID> m_temp_spawned_objects;

private:
    void fill_spawned_objects();
    void spawn_new_spawns();

protected:
    void spawn_new_objects();

public:
    IC CALifeSurgeManager(IPureServer* server, LPCSTR section);
    virtual ~CALifeSurgeManager();
};

#include "alife_surge_manager_inline.h"
