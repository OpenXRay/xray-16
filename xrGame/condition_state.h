////////////////////////////////////////////////////////////////////////////
//	Module 		: condition_state.h
//	Created 	: 26.02.2004
//  Modified 	: 26.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Condition state
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "operator_condition.h"

template <typename _world_property>
class CConditionState {
public:
	typedef _world_property						COperatorCondition;

protected:
	xr_vector<COperatorCondition>				m_conditions;
	u32											m_hash;

public:
	IC											CConditionState		();
	virtual										~CConditionState	();
	IC		const xr_vector<COperatorCondition>	&conditions			() const;
	IC		u8									weight				(const CConditionState &condition) const;
	IC		void								add_condition		(const COperatorCondition &condition);
	IC		void								remove_condition	(const typename COperatorCondition::_condition_type &condition);
	IC		void								add_condition		(typename xr_vector<COperatorCondition>::const_iterator &J, const COperatorCondition &condition);
	IC		void								add_condition_back	(const COperatorCondition &condition);
	IC		bool								includes			(const CConditionState &condition) const;
	IC		void								clear				();
	IC		bool								operator<			(const CConditionState &condition) const;
	IC		CConditionState<_world_property>	&operator-=			(const CConditionState &condition);
	IC		bool 								operator==			(const CConditionState &condition) const;
	IC		u32									hash_value			() const;
	IC		const COperatorCondition			*property			(const typename COperatorCondition::_condition_type &condition) const;
};

#include "condition_state_inline.h"