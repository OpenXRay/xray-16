////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_abstract_registry.h
//	Created 	: 30.06.2004
//  Modified 	: 30.06.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife abstract registry
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/object_interfaces.h"
#include "Common/object_broker.h"

template <typename _index_type, typename _data_type>
class CALifeAbstractRegistry : public ISerializable
{
public:
    typedef xr_map<_index_type, _data_type> OBJECT_REGISTRY;
    typedef typename OBJECT_REGISTRY::iterator iterator;
    typedef typename OBJECT_REGISTRY::const_iterator const_iterator;

protected:
    OBJECT_REGISTRY m_objects;

public:
    typedef _data_type _data;

    IC CALifeAbstractRegistry();
    virtual ~CALifeAbstractRegistry();
    virtual void save(IWriter& memory_stream);
    virtual void load(IReader& file_stream);
    IC const OBJECT_REGISTRY& objects() const;
    IC void add(const _index_type& index, _data_type& data, bool no_assert = false);
    IC void remove(const _index_type& index, bool no_assert = false);
    IC _data_type* object(const _index_type& index, bool no_assert = false);
};

#include "alife_abstract_registry_inline.h"
