////////////////////////////////////////////////////////////////////////////
//	Module 		: script_value_container_impl.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script value container
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/object_broker.h"
#ifdef XRSE_FACTORY_EXPORTS
#include "script_value.h"
#endif

IC CScriptValueContainer::~CScriptValueContainer() { clear(); }
IC void CScriptValueContainer::add(CScriptValue* new_value)
{
#ifdef XRSE_FACTORY_EXPORTS
    const auto I = std::find_if(m_values.begin(), m_values.end(), [new_value](const CScriptValue* value)
    {
        return 0 == xr_strcmp(value->name(), new_value->name());
    });

    if (I != m_values.end())
        return;

    m_values.emplace_back(new_value);
#endif
}

IC void CScriptValueContainer::assign() const
{
#ifdef XRSE_FACTORY_EXPORTS
    for (const auto& value : m_values)
        value->assign();
#endif
}

IC void CScriptValueContainer::clear() { delete_data(m_values); }
