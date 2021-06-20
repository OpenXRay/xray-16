////////////////////////////////////////////////////////////////////////////
//	Module 		: AssociativeVector.h
//	Created 	: 14.10.2005
//  Modified 	: 14.10.2005
//	Author		: Dmitriy Iassenev
//	Description : associative vector container
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <functional>
#include <utility>

#include "xrCore/xrCore.h"
#include "xrCommon/xr_vector.h"
#include "AssociativeVectorComparer.hpp"

template <typename TKey, typename TValue, typename TKeyComparer = std::less<TKey>>
class AssociativeVector : protected xr_vector<std::pair<TKey, TValue>>,
                          protected AssociativeVectorComparer<TKey, TValue, TKeyComparer>
{
private:
    typedef AssociativeVector<TKey, TValue, TKeyComparer> TSelf;
    typedef xr_vector<std::pair<TKey, TValue>> inherited;

public:
    typedef AssociativeVectorComparer<TKey, TValue, TKeyComparer> TComparer;
    typedef typename inherited::allocator_type allocator_type;
    typedef typename inherited::const_pointer const_pointer;
    typedef typename inherited::const_reference const_reference;
    typedef typename inherited::const_iterator const_iterator;
    typedef typename inherited::const_reverse_iterator const_reverse_iterator;
    typedef typename inherited::pointer pointer;
    typedef typename inherited::reference reference;
    typedef typename inherited::iterator iterator;
    typedef typename inherited::reverse_iterator reverse_iterator;
    typedef typename allocator_type::difference_type difference_type;
    typedef TKeyComparer key_compare;
    typedef TKey key_type;
    typedef TValue mapped_type;
    typedef typename inherited::size_type size_type;
    typedef typename inherited::value_type value_type;
    typedef std::pair<iterator, bool> insert_result;
    typedef std::pair<iterator, iterator> equal_range_result;
    typedef std::pair<const_iterator, const_iterator> const_equal_range_result;

private:
    IC void actualize() const;

public:
    template <typename TIterator>
    IC AssociativeVector(TIterator first, TIterator last, const key_compare& predicate = key_compare(),
        const allocator_type& allocator = allocator_type());
    IC AssociativeVector(
        const key_compare& predicate = key_compare(), const allocator_type& allocator = allocator_type());
    IC explicit AssociativeVector(const key_compare& predicate);
    IC iterator begin();
    IC iterator end();
    IC reverse_iterator rbegin();
    IC iterator rend();
    IC insert_result insert(const value_type& value);
    IC iterator insert(iterator where, const value_type& value);
    template <class TIterator>
    IC void insert(TIterator first, TIterator last);
    IC void erase(iterator element);
    IC void erase(iterator first, iterator last);
    IC size_type erase(const key_type& key);
    IC void clear();
    IC void reserve(size_t new_capacity);
    IC iterator find(const key_type& key);
    IC iterator lower_bound(const key_type& key);
    IC iterator upper_bound(const key_type& key);
    IC equal_range_result equal_range(const key_type& key);
    IC void swap(TSelf& object);

public:
    IC const_iterator begin() const;
    IC const_iterator end() const;
    IC const_reverse_iterator rbegin() const;
    IC const_reverse_iterator rend() const;
    IC const_iterator find(const key_type& key) const;
    IC const_iterator lower_bound(const key_type& key) const;
    IC const_iterator upper_bound(const key_type& key) const;
    IC const_equal_range_result equal_range(const key_type& key) const;
    IC size_type count(const key_type& key) const;
    IC size_type max_size() const;
    IC size_type size() const;
    IC bool empty() const;
    IC key_compare key_comp() const;
    IC TComparer value_comp() const;
    IC allocator_type get_allocator() const;

    IC mapped_type& operator[](const key_type& key);
    IC TSelf& operator=(const TSelf& right);
    IC bool operator<(const TSelf& right) const;
    IC bool operator<=(const TSelf& right) const;
    IC bool operator>(const TSelf& right) const;
    IC bool operator>=(const TSelf& right) const;
    IC bool operator==(const TSelf& right) const;
    IC bool operator!=(const TSelf& right) const;
};

template <typename TKey, typename TValue, typename TKeyComparer>
IC void swap(AssociativeVector<TKey, TValue, TKeyComparer>& left, AssociativeVector<TKey, TValue, TKeyComparer>& right);

#define TEMPLATE_SPECIALIZATION template <typename TKey, typename TValue, typename TKeyComparer>
#define _associative_vector AssociativeVector<TKey, TValue, TKeyComparer>

TEMPLATE_SPECIALIZATION
IC _associative_vector::AssociativeVector(const key_compare& predicate, const allocator_type& /*allocator*/)
    : TComparer(predicate)
{
}

TEMPLATE_SPECIALIZATION
IC _associative_vector::AssociativeVector(const key_compare& predicate) : TComparer(predicate) {}
TEMPLATE_SPECIALIZATION
template <typename TIterator>
IC _associative_vector::AssociativeVector(
    TIterator first, TIterator last, const key_compare& predicate, const allocator_type& allocator)
    : inherited(first, last), TComparer(predicate)
{
    std::sort(begin(), end(), static_cast<TComparer&>(*this));
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::iterator _associative_vector::begin()
{
    actualize();
    return inherited::begin();
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::iterator _associative_vector::end()
{
    actualize();
    return inherited::end();
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::const_iterator _associative_vector::begin() const
{
    actualize();
    return inherited::begin();
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::const_iterator _associative_vector::end() const
{
    actualize();
    return inherited::end();
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::reverse_iterator _associative_vector::rbegin()
{
    actualize();
    return inherited::rbegin();
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::iterator _associative_vector::rend()
{
    actualize();
    return inherited::rend();
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::const_reverse_iterator _associative_vector::rbegin() const
{
    actualize();
    return inherited::rbegin();
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::const_reverse_iterator _associative_vector::rend() const
{
    actualize();
    return inherited::rend();
}

TEMPLATE_SPECIALIZATION
IC void _associative_vector::clear() { inherited::clear(); }
TEMPLATE_SPECIALIZATION
IC void _associative_vector::reserve(size_t new_capacity) { inherited::reserve(new_capacity); }
TEMPLATE_SPECIALIZATION
IC typename _associative_vector::size_type _associative_vector::max_size() const { return inherited::max_size(); }
TEMPLATE_SPECIALIZATION
IC typename _associative_vector::size_type _associative_vector::size() const { return inherited::size(); }
TEMPLATE_SPECIALIZATION
IC bool _associative_vector::empty() const { return inherited::empty(); }
TEMPLATE_SPECIALIZATION
IC typename _associative_vector::allocator_type _associative_vector::get_allocator() const
{
    return inherited::get_allocator();
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::mapped_type& _associative_vector::operator[](const key_type& key)
{
    iterator I = find(key);
    if (I != end())
        return ((*I).second);
    return insert(value_type(key, mapped_type())).first->second;
}

TEMPLATE_SPECIALIZATION
IC void _associative_vector::swap(TSelf& right) { inherited::swap(right); }
TEMPLATE_SPECIALIZATION
IC void swap(_associative_vector& left, _associative_vector& right) { left.swap(right); }
TEMPLATE_SPECIALIZATION
IC typename _associative_vector::key_compare _associative_vector::key_comp() const { return key_compare(); }
TEMPLATE_SPECIALIZATION
IC typename _associative_vector::TComparer _associative_vector::value_comp() const { return TComparer(key_comp()); }
TEMPLATE_SPECIALIZATION
IC void _associative_vector::erase(iterator element) { inherited::erase(element); }
TEMPLATE_SPECIALIZATION
IC void _associative_vector::erase(iterator first, iterator last) { inherited::erase(first, last); }
TEMPLATE_SPECIALIZATION
IC typename _associative_vector::size_type _associative_vector::erase(const key_type& key)
{
    iterator I = find(key);
    if (I == end())
        return 0;
    erase(I);
    return 1;
}

TEMPLATE_SPECIALIZATION
IC void _associative_vector::actualize() const {}
TEMPLATE_SPECIALIZATION
IC typename _associative_vector::iterator _associative_vector::lower_bound(const key_type& key)
{
    actualize();
    TComparer& self_ = *this;
    return std::lower_bound(begin(), end(), key, self_);
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::const_iterator _associative_vector::lower_bound(const key_type& key) const
{
    actualize();
    const TComparer& self_ = *this;
    return std::lower_bound(begin(), end(), key, self_);
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::iterator _associative_vector::upper_bound(const key_type& key)
{
    actualize();
    TComparer& self_ = *this;
    return std::upper_bound(begin(), end(), key, self_);
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::const_iterator _associative_vector::upper_bound(const key_type& key) const
{
    actualize();
    const TComparer& self_ = *this;
    return std::upper_bound(begin(), end(), key, self_);
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::insert_result _associative_vector::insert(const value_type& value)
{
    actualize();
    bool found = true;
    iterator I = lower_bound(value.first);
    if (I == end() || (*this)(value.first, (*I).first))
    {
        I = inherited::insert(I, value);
        found = false;
    }
    else
        *I = value;
    return insert_result(I, !found);
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::iterator _associative_vector::insert(iterator where, const value_type& value)
{
    if (where != end() && (*this)(*where, value) && (where - begin()) == size() &&
        !(*this)(value, *(where + 1)) && (*this)(*(where + 1), value))
    {
        return inherited::insert(where, value);
    }
    return insert(value).first;
}

TEMPLATE_SPECIALIZATION
template <class TIterator>
IC void _associative_vector::insert(TIterator first, TIterator last)
{
    if ((last - first) < log2(size() + (last - first)))
    {
        for (; first != last; ++first)
            insert(*first);
        return;
    }
    inherited::insert(end(), first, last);
    std::sort(begin(), end(), static_cast<TComparer&>(*this));
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::iterator _associative_vector::find(const key_type& key)
{
    actualize();
    iterator I = lower_bound(key);
    if (I == end())
        return end();
    if ((*this)(key, (*I).first))
        return end();
    return I;
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::const_iterator _associative_vector::find(const key_type& key) const
{
    actualize();
    const_iterator I = lower_bound(key);
    if (I == end())
        return end();
    if ((*this)(key, (*I).first))
        return end();
    return I;
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::size_type _associative_vector::count(const key_type& key) const
{
    actualize();
    return find(key) == end() ? 0 : 1;
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::equal_range_result _associative_vector::equal_range(const key_type& key)
{
    actualize();
    iterator I = lower_bound(key);
    if (I == end())
        return equal_range_result(end(), end());
    if ((*this)(key, (*I).first))
        return equal_range_result(I, I);
    VERIFY(!operator()(key, (*I).first));
    return equal_range_result(I, I + 1);
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::const_equal_range_result _associative_vector::equal_range(const key_type& key) const
{
    actualize();
    const_iterator I = lower_bound(key);
    if (I == end())
        return const_equal_range_result(end(), end());
    if ((*this)(key, (*I).first))
        return const_equal_range_result(I, I);
    VERIFY(!operator()(key, (*I).first));
    return const_equal_range_result(I, I + 1);
}

TEMPLATE_SPECIALIZATION
IC typename _associative_vector::TSelf& _associative_vector::operator=(const TSelf& right)
{
    static_cast<inherited&>(*this) = right;
    return *this;
}

TEMPLATE_SPECIALIZATION
IC bool _associative_vector::operator<(const TSelf& right) const
{
    return static_cast<const inherited&>(*this) < right;
}

TEMPLATE_SPECIALIZATION
IC bool _associative_vector::operator<=(const TSelf& right) const { return !(right < *this); }
TEMPLATE_SPECIALIZATION
IC bool _associative_vector::operator>(const TSelf& right) const { return right < *this; }
TEMPLATE_SPECIALIZATION
IC bool _associative_vector::operator>=(const TSelf& right) const { return !(*this < right); }
TEMPLATE_SPECIALIZATION
IC bool _associative_vector::operator==(const TSelf& right) const
{
    return static_cast<const inherited&>(*this) == right;
}

TEMPLATE_SPECIALIZATION
IC bool _associative_vector::operator!=(const TSelf& right) const { return !(*this == right); }
#undef TEMPLATE_SPECIALIZATION
#undef _associative_vector
