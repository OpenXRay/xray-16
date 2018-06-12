////////////////////////////////////////////////////////////////////////////
//	Module 		: condition_state.h
//	Created 	: 26.02.2004
//  Modified 	: 26.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Condition state
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Components/operator_condition.h"

template <typename _world_property>
class CConditionState
{
public:
    typedef _world_property COperatorCondition;

protected:
    xr_vector<_world_property> m_conditions;
    u32 m_hash;

public:
    IC CConditionState();
    IC virtual ~CConditionState();
    IC const xr_vector<_world_property>& conditions() const;
    IC u8 weight(const _world_property& condition) const;
    IC void add_condition(const _world_property& condition);
    IC void remove_condition(const typename _world_property::condition_type& condition);
    IC void add_condition(
        typename xr_vector<_world_property>::const_iterator& J, const _world_property& condition);
    IC void add_condition_back(const _world_property& condition);
    IC bool includes(const CConditionState& condition) const;
    IC void clear();
    IC bool operator<(const CConditionState& condition) const;
    IC CConditionState<_world_property>& operator-=(const CConditionState& condition);
    IC bool operator==(const CConditionState& condition) const;
    IC u32 hash_value() const;
    IC const _world_property* property(const typename _world_property::condition_type& condition) const;
};

#include "xrAICore/Components/condition_state_inline.h"
