////////////////////////////////////////////////////////////////////////////
//	Module 		: script_value_container_impl.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script value container
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "object_broker.h"
#ifdef XRSE_FACTORY_EXPORTS
#	include "script_value.h"
#endif

IC	CScriptValueContainer::~CScriptValueContainer	()
{
	clear				();
}

IC	void CScriptValueContainer::add			(CScriptValue *new_value)
{
#ifdef XRSE_FACTORY_EXPORTS
	CScriptValue		*value = 0;
	xr_vector<CScriptValue*>::const_iterator	I = m_values.begin();
	xr_vector<CScriptValue*>::const_iterator	E = m_values.end();
	for ( ; I != E; ++I)
		if (!xr_strcmp((*I)->name(),new_value->name())) {
			value		= *I;
			break;
		}

	if (value)
		return;

	m_values.push_back	(new_value);
#endif
}

IC	void CScriptValueContainer::assign		()
{
#ifdef XRSE_FACTORY_EXPORTS
	xr_vector<CScriptValue*>::iterator	I = m_values.begin();
	xr_vector<CScriptValue*>::iterator	E = m_values.end();
	for ( ; I != E; ++I)
		(*I)->assign	();
#endif
}

IC	void CScriptValueContainer::clear		()
{
	delete_data			(m_values);
}
