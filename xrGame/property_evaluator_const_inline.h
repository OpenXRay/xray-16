////////////////////////////////////////////////////////////////////////////
//	Module 		: property_evaluator_const_inline.h
//	Created 	: 12.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property evaluator const inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _object_type\
	>
#define CEvaluator	CPropertyEvaluatorConst<_object_type>

TEMPLATE_SPECIALIZATION
IC	CEvaluator::CPropertyEvaluatorConst	(_value_type value, LPCSTR evaluator_name) :
	m_value			(value)
{
#ifdef LOG_ACTION
	m_evaluator_name	= evaluator_name;
#endif
}

TEMPLATE_SPECIALIZATION
typename CEvaluator::_value_type CEvaluator::evaluate	()
{
	return			(m_value);
}

#undef TEMPLATE_SPECIALIZATION
#undef CEvaluator