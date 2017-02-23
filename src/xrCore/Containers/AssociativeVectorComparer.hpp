////////////////////////////////////////////////////////////////////////////
//	Module 		: AssociativeVectorComparer.hpp
//	Created 	: 14.10.2005
//  Modified 	: 14.10.2005
//	Author		: Dmitriy Iassenev
//	Description : associative vector compare predicate template class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/xrCore.h"
#include <utility>

template <typename TKey, typename TValue, typename TComparer>
class AssociativeVectorComparer : public TComparer
{
private:
    typedef TComparer inherited;

public:
    typedef std::pair<TKey, TValue> TItem;

    IC AssociativeVectorComparer();
    IC AssociativeVectorComparer(const TComparer& comparer);
    IC bool operator()(const TKey& lhs, const TKey& rhs) const;
    IC bool operator()(const TItem& lhs, const TItem& rhs) const;
    IC bool operator()(const TItem& lhs, const TKey& rhs) const;
    IC bool operator()(const TKey& lhs, const TItem& rhs) const;
};

#define TEMPLATE_SPECIALIZATION template <typename TKey, typename TValue, typename TComparer>
#define _associative_vector_compare_predicate AssociativeVectorComparer<TKey, TValue, TComparer>

TEMPLATE_SPECIALIZATION
IC _associative_vector_compare_predicate::AssociativeVectorComparer() {}
TEMPLATE_SPECIALIZATION
IC _associative_vector_compare_predicate::AssociativeVectorComparer(const TComparer& comparer) : inherited(comparer) {}
TEMPLATE_SPECIALIZATION
IC bool _associative_vector_compare_predicate::operator()(const TKey& lhs, const TKey& rhs) const
{
    return inherited::operator()(lhs, rhs);
}

TEMPLATE_SPECIALIZATION
IC bool _associative_vector_compare_predicate::operator()(const TItem& lhs, const TItem& rhs) const
{
    return operator()(lhs.first, rhs.first);
}

TEMPLATE_SPECIALIZATION
IC bool _associative_vector_compare_predicate::operator()(const TItem& lhs, const TKey& rhs) const
{
    return operator()(lhs.first, rhs);
}

TEMPLATE_SPECIALIZATION
IC bool _associative_vector_compare_predicate::operator()(const TKey& lhs, const TItem& rhs) const
{
    return operator()(lhs, rhs.first);
}

#undef TEMPLATE_SPECIALIZATION
#undef _associative_vector_compare_predicate
