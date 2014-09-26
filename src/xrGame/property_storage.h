////////////////////////////////////////////////////////////////////////////
//	Module 		: property_storage.h
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property storage class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "graph_engine_space.h"
#include "script_export_space.h"

class CPropertyStorage {
public:
	typedef GraphEngineSpace::_solver_condition_type	_condition_type;
	typedef GraphEngineSpace::_solver_value_type		_value_type;
	typedef GraphEngineSpace::CSolverConditionValue		CConditionValue;
	typedef GraphEngineSpace::CSolverConditionStorage	CConditionStorage;

public:
	CConditionStorage			m_storage;

public:
	IC		void				clear			();
	IC		void				set_property	(const _condition_type &condition_id, const _value_type &value);
	IC		const _value_type	&property		(const _condition_type &condition_id) const;
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CPropertyStorage)
#undef script_type_list
#define script_type_list save_type_list(CPropertyStorage)

#include "property_storage_inline.h"