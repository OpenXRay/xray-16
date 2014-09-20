////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_object_handler_inline.h
//	Created 	: 07.10.2005
//  Modified 	: 07.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human object handler class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeHumanObjectHandler::CALifeHumanObjectHandler						(object_type *object)
{
	VERIFY		(object);
	m_object	= object;
}

IC	CALifeHumanObjectHandler::object_type &CALifeHumanObjectHandler::object	() const
{
	VERIFY		(m_object);
	return		(*m_object);
}
