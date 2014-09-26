////////////////////////////////////////////////////////////////////////////
//	Module 		: buffer_vector.cpp
//	Created 	: 10.10.2007
//  Modified 	: 10.10.2007
//	Author		: Dmitriy Iassenev
//	Description : buffer vector template class test cases
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

struct test_struct {
	shared_str	a;

	test_struct	() : a("unique_string_value") {}
};

void buffer_vector_test	()
{
	typedef buffer_vector<test_struct>	ContainerType;
//	typedef buffer_vector<u16>			ContainerType;
	typedef ContainerType::value_type	value_type;
	ContainerType	container(_alloca(16*sizeof(value_type)), 16);
	Msg				("%s", container.empty() ? "true" : "false");
	Msg				("%d", container.size());
	
	for (u32 i=0; i<8; ++i)
		container.push_back	(value_type());

	container.clear			();

	for (u32 i=0; i<16; ++i)
		container.push_back	(value_type());

	container.resize		(4);

	for (u32 i=0; i<4; ++i)
		container.push_back	(value_type());

	container.resize		(16);

	{
		ContainerType::iterator	I = container.begin();
		ContainerType::iterator	E = container.end();
		for ( ; I != E; ++I)
			*I					= value_type();
	}

	{
		ContainerType::const_iterator	I = container.begin();
		ContainerType::const_iterator	E = container.end();
		for ( ; I != E; ++I)
//			*I					= value_type()
			;
	}

	{
		ContainerType::reverse_iterator	I = container.rbegin();
		ContainerType::reverse_iterator	E = container.rend();
		for ( ; I != E; ++I)
			*I					= value_type();
	}

	{
		ContainerType::const_reverse_iterator	I = container.rbegin();
		ContainerType::const_reverse_iterator	E = container.rend();
		for ( ; I != E; ++I)
//			*I					= value_type()
			;
	}

	ContainerType			other(_alloca(32*sizeof(test_struct)), 32);
	other.assign			(24, value_type());
	other.swap				(container);

	container.push_back		(value_type());

	other.swap				(container);
	other.assign			(container.begin(), container.end());

	other.insert			(other.begin() + 8, container.begin(), container.end());

	other.pop_back			();
	other.push_back			(value_type());
	other.resize			(24);
	other.insert			(other.begin() + 12, value_type());
	other.insert			(other.begin() + 12, 7, value_type());
//	other.insert			(other.begin() + 12, 1, value_type());
	other.erase				(other.begin() + 1, other.begin() + 31);

	other.clear				();
	container.clear			();
}