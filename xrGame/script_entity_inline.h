////////////////////////////////////////////////////////////////////////////
//	Module 		: script_entity_inline.h
//	Created 	: 06.10.2003
//  Modified 	: 14.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Script entity class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CGameObject	&CScriptEntity::object	() const
{
	VERIFY			(m_object);
	return			(*m_object);
}
