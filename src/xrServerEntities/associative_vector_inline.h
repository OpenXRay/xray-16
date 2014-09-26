////////////////////////////////////////////////////////////////////////////
//	Module 		: associative_vector_inline.h
//	Created 	: 14.10.2005
//  Modified 	: 14.10.2005
//	Author		: Dmitriy Iassenev
//	Description : associative vector container inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION	\
	template <\
		typename _key_type,\
		typename _data_type,\
		typename _compare_predicate_type\
	>

#define _associative_vector	\
	associative_vector<\
		_key_type,\
		_data_type,\
		_compare_predicate_type\
	>

TEMPLATE_SPECIALIZATION
IC	_associative_vector::associative_vector													(const key_compare &predicate, const allocator_type &allocator) :
//	inherited			(allocator),
	value_compare		(predicate)
{
}

TEMPLATE_SPECIALIZATION
IC	_associative_vector::associative_vector													(const key_compare &predicate) :
	value_compare		(predicate)
{
}

TEMPLATE_SPECIALIZATION
template <typename _iterator_type>
IC	_associative_vector::associative_vector													(_iterator_type first, _iterator_type last, const key_compare &predicate = key_compare(), const allocator_type &allocator = allocator_type()) :
//	inherited			(first,last,allocator),
	inherited			(first,last),
	value_compare		(predicate)
{
	std::sort			(begin(),end(),(value_compare&)(*this));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::iterator _associative_vector::begin						()
{
	actualize			();
	return				(inherited::begin());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::iterator _associative_vector::end							()
{
	actualize			();
	return				(inherited::end());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::const_iterator _associative_vector::begin					() const
{
	actualize			();
	return				(inherited::begin());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::const_iterator _associative_vector::end					() const
{
	actualize			();
	return				(inherited::end());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::reverse_iterator _associative_vector::rbegin				()
{
	actualize			();
	return				(inherited::rbegin());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::iterator _associative_vector::rend						()
{
	actualize			();
	return				(inherited::rend());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::const_reverse_iterator _associative_vector::rbegin		() const
{
	actualize			();
	return				(inherited::rbegin());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::const_reverse_iterator _associative_vector::rend			() const
{
	actualize			();
	return				(inherited::rend());
}

TEMPLATE_SPECIALIZATION
IC	void _associative_vector::clear															()
{
	inherited::clear	();
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::size_type _associative_vector::max_size					() const
{
	return				(inherited::max_size());
}

TEMPLATE_SPECIALIZATION
//IC	typename _associative_vector::size_type _associative_vector::size						() const
IC	u32 _associative_vector::size															() const
{
	return				(inherited::size());
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector::empty															() const
{
	return				(inherited::empty());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::allocator_type _associative_vector::get_allocator			() const
{
	return				(inherited::get_allocator());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::mapped_type &_associative_vector::operator[]				(const key_type &key)
{
	iterator			I = find(key);
	if (I != end())
		return			((*I).second);

	return				(insert(value_type(key,mapped_type())).first->second);
}

TEMPLATE_SPECIALIZATION
IC	void _associative_vector::swap															(self_type &right)
{
	inherited::swap		(right);
}

TEMPLATE_SPECIALIZATION
IC	void swap																				(_associative_vector &left, _associative_vector &right)
{
	left.swap			(right);
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::key_compare _associative_vector::key_comp					() const
{
	return				(key_compare());
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::value_compare _associative_vector::value_comp				() const
{
	return				(value_compare(key_comp()));
}

TEMPLATE_SPECIALIZATION
IC	void _associative_vector::erase															(iterator element)
{
	inherited::erase	(element);
}

TEMPLATE_SPECIALIZATION
IC	void _associative_vector::erase															(iterator first, iterator last)
{
	inherited::erase	(first,last);
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::size_type _associative_vector::erase						(const key_type &key)
{
	iterator			I = find(key);
	if (I == end())
		return			(0);

	erase				(I);
	return				(1);
}

TEMPLATE_SPECIALIZATION
IC	void _associative_vector::actualize														() const
{
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::iterator _associative_vector::lower_bound					(const key_type &key)
{
	actualize			();
	value_compare		&self = *this;
	return				(std::lower_bound(begin(),end(),key,self));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::const_iterator _associative_vector::lower_bound			(const key_type &key) const
{
	actualize			();
	const value_compare	&self = *this;
	return				(std::lower_bound(begin(),end(),key,self));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::iterator _associative_vector::upper_bound					(const key_type &key)
{
	actualize			();
	value_compare		&self = *this;
	return				(std::upper_bound(begin(),end(),key,self));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::const_iterator _associative_vector::upper_bound			(const key_type &key) const
{
	actualize			();
	const value_compare	&self = *this;
	return				(std::upper_bound(begin(),end(),key,self));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::insert_result _associative_vector::insert					(const value_type &value)
{
	actualize			();
	bool				found = true;
	iterator			I = lower_bound(value.first);
	if (I == end() || operator()(value.first,(*I).first)) {
		I				= inherited::insert(I,value);
		found			= false;
	}
	else
		*I				= value;
	return				(insert_result(I,!found));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::iterator _associative_vector::insert						(iterator where, const value_type &value)
{
	if	(
			(where != end()) && 
			(operator()(*where,value)) &&
			((where - begin()) == size()) &&
			(!operator()(value,*(where + 1))) &&
			(operator()(*(where + 1),value))
		)
			return		(inherited::insert(where,value));

	return				(insert(val).first);
}

TEMPLATE_SPECIALIZATION
template <class _iterator_type>
IC	void _associative_vector::insert														(_iterator_type first, _iterator_type last)
{
	if ((last - first) < log2(size() + (last - first))) {
		for ( ; first != last; ++first)
			insert		(*first);

		return;
	}

	inherited::insert	(end(),first,last);
	std::sort			(begin(),end(),(value_compare&)(*this));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::iterator _associative_vector::find						(const key_type &key)
{
	actualize			();
	iterator			I = lower_bound(key);
	if (I == end())
		return			(end());

	if (operator()(key,(*I).first))
		return			(end());

	return				(I);
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::const_iterator _associative_vector::find					(const key_type &key) const
{
	actualize			();
	const_iterator		I = lower_bound(key);
	if (I == end())
		return			(end());

	if (operator()(key,(*I).first))
		return			(end());

	return				(I);
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::size_type _associative_vector::count						(const key_type &key) const
{
	actualize			();
	return				(find(key) == end() ? 0 : 1);
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::equal_range_result _associative_vector::equal_range		(const key_type &key)
{
	actualize			();
	iterator			I = lower_bound(key);
	if (I == end())
		return			(equal_range_result(end(),end()));

	if (operator()(key,(*I).first))
		return			(equal_range_result(I,I));

	VERIFY				(!operator()(key,(*I).first));
	return				(equal_range_result(I,I+1));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::const_equal_range_result _associative_vector::equal_range	(const key_type &key) const
{
	actualize			();
	const_iterator		I = lower_bound(key);
	if (I == end())
		return			(const_equal_range_result(end(),end()));

	if (operator()(key,(*I).first))
		return			(const_equal_range_result(I,I));

	VERIFY				(!operator()(key,(*I).first));
	return				(const_equal_range_result(I,I+1));
}

TEMPLATE_SPECIALIZATION
IC	typename _associative_vector::self_type &_associative_vector::operator=					(const self_type &right)
{
	(inherited&)(*this)	= right;
	return				(*this);
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector::operator<														(const self_type &right) const
{
	return				(((const inherited &)(*this)) < right);
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector::operator<=													(const self_type &right) const
{
	return				!(right < left);
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector::operator>														(const self_type &right) const
{
	return				(right < left);
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector::operator>=													(const self_type &right) const
{
	return				!(left < right);
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector::operator==													(const self_type &right) const
{
	return				(((const inherited &)(*this)) == right);
}

TEMPLATE_SPECIALIZATION
IC	bool _associative_vector::operator!=													(const self_type &right) const
{
	return				!(left == right);
}

#undef TEMPLATE_SPECIALIZATION
#undef _associative_vector