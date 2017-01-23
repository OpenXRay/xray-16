////////////////////////////////////////////////////////////////////////////
//	Module 		: property_evaluator_member.h
//	Created 	: 12.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property evaluator member
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "property_evaluator.h"

template <typename _object_type>
class CPropertyEvaluatorMember : public CPropertyEvaluator<_object_type> {
protected:
	typedef CPropertyEvaluator<_object_type>	inherited;

protected:
	_condition_type		m_condition_id;
	_value_type			m_value;
	bool				m_equality;

public:
						CPropertyEvaluatorMember(CPropertyStorage *storage, _condition_type condition_id, _value_type value, bool equality = true, LPCSTR evaluator_name = "");
	virtual void		setup					(_object_type *object, CPropertyStorage *storage);
	virtual _value_type	evaluate				();
};


#include "property_evaluator_member_inline.h"