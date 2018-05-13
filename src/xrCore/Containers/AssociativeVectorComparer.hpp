////////////////////////////////////////////////////////////////////////////
//	Module 		: AssociativeVectorComparer.hpp
//	Created 	: 14.10.2005
//  Modified 	: 14.10.2005
//	Author		: Dmitriy Iassenev
//	Description : associative vector compare predicate template class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <utility>

template <typename TKey, typename TValue, typename TComparer>
class AssociativeVectorComparer : public TComparer
{
    using inherited = TComparer;

public:
    using TItem = std::pair<TKey, TValue>;

    AssociativeVectorComparer() {}
    AssociativeVectorComparer(const TComparer& comparer) : inherited(comparer) {}
    bool operator()(const TKey& lhs, const TKey& rhs) const { return inherited::operator()(lhs, rhs); }
    bool operator()(const TItem& lhs, const TItem& rhs) const { return operator()(lhs.first, rhs.first); }
    bool operator()(const TItem& lhs, const TKey& rhs) const { return operator()(lhs.first, rhs); }
    bool operator()(const TKey& lhs, const TItem& rhs) const { return operator()(lhs, rhs.first); }
};
