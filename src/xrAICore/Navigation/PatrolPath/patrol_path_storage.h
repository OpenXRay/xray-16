////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_storage.h
//	Created 	: 15.06.2004
//  Modified 	: 15.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol path storage
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Common/object_interfaces.h"
#include "xrCore/Containers/AssociativeVector.hpp"
#include "xrScriptEngine/DebugMacros.hpp"
#include "xrCore/xrstring.h"

class CPatrolPath;
class CLevelGraph;
class CGameLevelCrossTable;
class CGameGraph;

class XRAICORE_API CPatrolPathStorage : public ISerializable
{
private:
    typedef ISerializable inherited;

public:
    typedef AssociativeVector<shared_str, CPatrolPath*> PATROL_REGISTRY;
    typedef PATROL_REGISTRY::iterator iterator;
    typedef PATROL_REGISTRY::const_iterator const_iterator;

protected:
    PATROL_REGISTRY m_registry;

public:
    IC CPatrolPathStorage();
    virtual ~CPatrolPathStorage();
    virtual void load(IReader& stream);
    virtual void save(IWriter& stream);

public:
    void load_raw(const CLevelGraph* level_graph, const CGameLevelCrossTable* cross, const CGameGraph* game_graph,
        IReader& stream);
    IC const CPatrolPath* path(shared_str patrol_name, bool no_assert = false) const;
    IC const PATROL_REGISTRY& patrol_paths() const;

    const CPatrolPath* add_alias_if_exist(shared_str patrol_name, shared_str duplicate_name);
};

#include "xrAICore/Navigation/PatrolPath/patrol_path_storage_inline.h"
