////////////////////////////////////////////////////////////////////////////
// alife_registry_wrapper.h - обертка для реестра, предусматривающая работу
//							  без alife()
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai_space.h"
#include "alife_simulator.h"


template <typename _registry_type>
class CALifeRegistryWrapper
{
public:
	IC				CALifeRegistryWrapper	() {holder_id = 0xffff;}
	virtual			~CALifeRegistryWrapper	() {delete_data(local_registry);}

	IC	void		init					(u16 id) {holder_id = id;}
	
	typename _registry_type::_data&			objects					();
	const typename _registry_type::_data*	objects_ptr				();

	typename _registry_type::_data&			objects					(u16 id);
	const typename _registry_type::_data*	objects_ptr				(u16 id);

private:
	//id - владельца реестра
	u16	holder_id;

	//реестр на случай, если нет ALife (для отладки)
//	typename _registry_type::_data	local_registry;
	typename _registry_type::OBJECT_REGISTRY local_registry;
};

template <typename _registry_type>
const typename _registry_type::_data* CALifeRegistryWrapper<_registry_type>::objects_ptr	(u16 id)
{
//	if(NULL == ai().get_alife()) return &local_registry;
	if(NULL == ai().get_alife()) 
	{
		typename _registry_type::iterator		I = local_registry.find(id);
		if (I == local_registry.end()) {
			typename _registry_type::_data new_registry;
			std::pair<_registry_type::iterator, bool> p = local_registry.insert(std::make_pair(id, new_registry));
			VERIFY(p.second);
			return &(*local_registry.find(id)).second;
		}
		return			(&(*I).second);
	}

	VERIFY(0xffff != id);

	typename _registry_type::_data* registy_container = ai().alife().registry((_registry_type*)NULL).object(id, true);
	return registy_container;
}

template <typename _registry_type>
typename _registry_type::_data& CALifeRegistryWrapper<_registry_type>::objects	(u16 id)
{
//	if(NULL == ai().get_alife()) return local_registry;
	if(NULL == ai().get_alife()) 
	{
		typename _registry_type::iterator		I = local_registry.find(id);
		if (I == local_registry.end()) {
			typename _registry_type::_data new_registry;
			std::pair<_registry_type::iterator, bool> p = local_registry.insert(std::make_pair(id, new_registry));
			VERIFY(p.second);
			return (*local_registry.find(id)).second;
						
		}else
			return	((*I).second);

	}

	typename _registry_type::_data* registy_container = ai().alife().registry((_registry_type*)NULL).object(id, true);

	if(!registy_container)	
	{
		typename _registry_type::_data new_registry;
		ai().alife().registry((_registry_type*)NULL).add(id, new_registry, false);
		registy_container = ai().alife().registry((_registry_type*)NULL).object(id, true);
		VERIFY(registy_container);
	}
	return *registy_container;
}


template <typename _registry_type>
const typename _registry_type::_data* CALifeRegistryWrapper<_registry_type>::objects_ptr	()
{
	return objects_ptr(holder_id);
}

template <typename _registry_type>
typename _registry_type::_data& CALifeRegistryWrapper<_registry_type>::objects	()
{
	return objects(holder_id);
}
