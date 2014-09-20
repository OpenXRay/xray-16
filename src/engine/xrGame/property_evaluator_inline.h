////////////////////////////////////////////////////////////////////////////
//	Module 		: property_evaluator_inline.h
//	Created 	: 12.03.2004
//  Modified 	: 12.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property evaluator inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <typename _object_type>
#define CEvaluator				CPropertyEvaluator<_object_type>

TEMPLATE_SPECIALIZATION
IC	CEvaluator::CPropertyEvaluator	(_object_type *object, LPCSTR evaluator_name)
{
	init				(object,evaluator_name);
}

TEMPLATE_SPECIALIZATION
IC	CEvaluator::~CPropertyEvaluator	()
{
}

TEMPLATE_SPECIALIZATION
IC	void CEvaluator::init			(_object_type *object, LPCSTR evaluator_name)
{
	m_object			= object;
#ifdef LOG_ACTION
	m_evaluator_name	= evaluator_name;
#endif
	m_storage			= 0;
}

TEMPLATE_SPECIALIZATION
void CEvaluator::setup				(_object_type *object, CPropertyStorage *storage)
{
	m_object			= object;
	m_storage			= storage;
}

TEMPLATE_SPECIALIZATION
void CEvaluator::Load				(LPCSTR section)
{
}

TEMPLATE_SPECIALIZATION
typename CEvaluator::_value_type CEvaluator::evaluate	()
{
	return				(0);
}

TEMPLATE_SPECIALIZATION
IC	const typename CEvaluator::_value_type &CEvaluator::property	(const _condition_type &condition_id) const
{
	VERIFY				(m_storage);
	return				(m_storage->property(condition_id));
}

#undef TEMPLATE_SPECIALIZATION
#undef CEvaluator