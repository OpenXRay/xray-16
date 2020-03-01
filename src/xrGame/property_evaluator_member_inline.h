////////////////////////////////////////////////////////////////////////////
//	Module 		: property_evaluator_member_inline.h
//	Created 	: 12.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property evaluator member inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <typename _object_type>
#define CEvaluator CPropertyEvaluatorMember<_object_type>

TEMPLATE_SPECIALIZATION
CEvaluator::CPropertyEvaluatorMember(
    CPropertyStorage* storage, _condition_type condition_id, _value_type value, bool equality, LPCSTR evaluator_name)
    : m_condition_id(condition_id), m_value(value), m_equality(equality)
{
#if 1//def LOG_ACTION //Alundaio: m_evaluator_name
    this->m_evaluator_name = evaluator_name;
#endif
    this->m_storage = storage;
}

TEMPLATE_SPECIALIZATION
void CEvaluator::setup(_object_type* object, CPropertyStorage* storage)
{
    inherited::setup(object, this->m_storage ? this->m_storage : storage);
}

TEMPLATE_SPECIALIZATION
typename CEvaluator::_value_type CEvaluator::evaluate()
{
    return ((this->m_storage->property(m_condition_id) == m_value) == m_equality);
}

#undef TEMPLATE_SPECIALIZATION
#undef CEvaluator
