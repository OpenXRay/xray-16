////////////////////////////////////////////////////////////////////////////
//	Module 		: associative_vector_compare_inline.h
//	Created 	: 14.10.2005
//  Modified 	: 14.10.2005
//	Author		: Dmitriy Iassenev
//	Description : associative vector compare predicate  template class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION	\
	template <\
		typename _key_type,\
		typename _value_type,\
		typename _compare_predicate_type\
	>

#define _associative_vector_compare_predicate	\
	associative_vector_compare_predicate<\
		_key_type,\
		_value_type,\
		_compare_predicate_type\
	>

TEMPLATE_SPECIALIZATION
IC	_associative_vector_compare_predicate::associative_vector_compare_predicate	()
{
}

TEMPLATE_SPECIALIZATION
IC	_associative_vector_compare_predicate::associative_vector_compare_predicate	(const _compare_predicate_type &compare_predicate) :
	inherited	(compare_predicate)
{
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector_compare_predicate::operator()						(const _key_type &lhs, const _key_type &rhs) const
{
	return		(inherited::operator()(lhs, rhs));
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector_compare_predicate::operator()						(const value_type &lhs, const value_type &rhs) const
{
	return		(operator()(lhs.first, rhs.first));
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector_compare_predicate::operator()						(const value_type &lhs, const _key_type &rhs) const
{
	return		(operator()(lhs.first, rhs));
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector_compare_predicate::operator()						(const _key_type &lhs, const value_type &rhs) const
{
	return		(operator()(lhs, rhs.first));
}

#undef TEMPLATE_SPECIALIZATION
#undef _associative_vector_compare_predicate