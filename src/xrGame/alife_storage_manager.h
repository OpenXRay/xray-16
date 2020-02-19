////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_storage_manager.h
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator storage manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_simulator_base.h"

class NET_Packet;

class CALifeStorageManager : public virtual CALifeSimulatorBase
{
    friend class CALifeUpdatePredicate;

protected:
    typedef CALifeSimulatorBase inherited;

protected:
    string_path m_save_name;
    const char* m_section;

private:
    void prepare_objects_for_save();
    void load(void* buffer, const u32& buffer_size, const char* file_name);

public:
    IC CALifeStorageManager(IPureServer* server, const char* section);
    virtual ~CALifeStorageManager();
    bool load(const char* save_name = 0);
    void save(const char* save_name = 0, bool update_name = true);
    void save(NET_Packet& net_packet);
};

#include "alife_storage_manager_inline.h"
