////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_velocity_holder.h
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Stalker velocity holder
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "associative_vector.h"

class CStalkerVelocityCollection;

class CStalkerVelocityHolder {
public:
	typedef CStalkerVelocityCollection					COLLECTION;
	typedef associative_vector<shared_str,COLLECTION*>	COLLECTIONS;

private:
	COLLECTIONS					m_collections;

public:
								~CStalkerVelocityHolder	();
			const COLLECTION	&collection				(const shared_str &section);
};

IC		CStalkerVelocityHolder	&stalker_velocity_holder();

extern	CStalkerVelocityHolder	*g_stalker_velocity_holder;

#include "stalker_velocity_holder_inline.h"