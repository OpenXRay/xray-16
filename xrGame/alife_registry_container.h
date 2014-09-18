////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_registry_container.h
//	Created 	: 01.07.2004
//  Modified 	: 01.07.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife registry container class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_registry_container_space.h"
#include "alife_registry_container_composition.h"
#include "alife_abstract_registry.h"

template <typename _type, typename _base>
struct CLinearRegistryType : public _base, public _type {};

class CALifeRegistryContainer : public Loki::GenLinearHierarchy<registry_type_list,CLinearRegistryType>::LinBase {
private:
	typedef registry_type_list TYPE_LIST;

public:
	template <typename T>
	IC		T		&operator()	(const T*);
	template <typename T>
	IC		const T &operator()	(const T*) const;
	virtual	void	load		(IReader &file_stream);
	virtual void	save		(IWriter &memory_stream);
};

#include "alife_registry_container_inline.h"