////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_registry_container_space.h
//	Created 	: 01.07.2004
//  Modified 	: 01.07.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife registry container space
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <loki/hierarchygenerators.h>
#define	registry_type_list				Loki::NullType
#define	add_to_registry_type_list(a)	typedef Loki::Typelist<a,registry_type_list> registry_##a;
#define	define_constant(a)				(a*)0 
#define	save_registry_type_list(a)		registry_##a
