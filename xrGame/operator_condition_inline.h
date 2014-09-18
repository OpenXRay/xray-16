////////////////////////////////////////////////////////////////////////////
//	Module 		: operator_condition_inline.h
//	Created 	: 24.02.2004
//  Modified 	: 24.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Operator condition inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "random32.h"

#define TEMPLATE_SPECIALIZATION template<\
	typename _condition_type,\
	typename _value_type\
>

#define CAbstractOperatorCondition COperatorConditionAbstract<_condition_type,_value_type>

TEMPLATE_SPECIALIZATION
IC	CAbstractOperatorCondition::COperatorConditionAbstract	(const _condition_type condition, const _value_type value) :
	m_condition			(condition),
	m_value				(value)
{
	u32					seed = ::Random32.seed();
	::Random32.seed		(u32(condition) + 1);
	m_hash				= ::Random32.random(0xffffffff);
	::Random32.seed		(m_hash + u32(value));
	m_hash				^= ::Random32.random(0xffffffff);
	::Random32.seed		(seed);
}

TEMPLATE_SPECIALIZATION
IC	const _condition_type &CAbstractOperatorCondition::condition	() const
{
	return				(m_condition);
}

TEMPLATE_SPECIALIZATION
IC	const _value_type &CAbstractOperatorCondition::value			() const
{
	return				(m_value);
}

TEMPLATE_SPECIALIZATION
IC	const u32 &CAbstractOperatorCondition::hash_value	() const
{
	return				(m_hash);
}

TEMPLATE_SPECIALIZATION
IC	bool CAbstractOperatorCondition::operator<			(const COperatorCondition &_condition) const
{
	if (condition() < _condition.condition())
		return			(true);
	if (condition() > _condition.condition())
		return			(false);
	if (value() < _condition.value())
		return			(true);
	return				(false);
}

TEMPLATE_SPECIALIZATION
IC	bool CAbstractOperatorCondition::operator==			(const COperatorCondition &_condition) const
{
	if ((condition() == _condition.condition()) && (value() == _condition.value()))
		return			(true);
	return				(false);
}

#undef TEMPLATE_SPECIALIZATION
#undef CAbstractOperatorCondition
