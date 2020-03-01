////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_simulator.h
//	Created 	: 25.12.2002
//  Modified 	: 13.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_interaction_manager.h"
#include "alife_update_manager.h"

#pragma warning(push)
#pragma warning(disable : 4005)

class CALifeSimulator : public CALifeUpdateManager, public CALifeInteractionManager
{
protected:
    virtual void setup_simulator(CSE_ALifeObject* object);
    virtual void reload(LPCSTR section);

public:
    CALifeSimulator(IPureServer* server, shared_str* command_line);
    virtual ~CALifeSimulator();
    virtual void destroy();
    IReader const* get_config(shared_str config) const;

#if 0 // def DEBUG
			void	validate			();
#endif // DEBUG

private:
    typedef xr_list<std::pair<shared_str, IReader*>> configs_type;
    mutable configs_type m_configs_lru;
};

#pragma warning(pop)
