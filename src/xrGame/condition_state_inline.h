////////////////////////////////////////////////////////////////////////////
//	Module 		: condition_state_inline.h
//	Created 	: 26.02.2004
//  Modified 	: 26.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Condition state inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template<typename _world_property>
#define CConditionStateAbstract CConditionState<_world_property>

TEMPLATE_SPECIALIZATION
IC	CConditionStateAbstract::CConditionState	()
{
//	m_conditions.reserve	(32);
	m_hash					= 0;
}

TEMPLATE_SPECIALIZATION
CConditionStateAbstract::~CConditionState	()
{
}

TEMPLATE_SPECIALIZATION
IC	const xr_vector<typename CConditionStateAbstract::COperatorCondition> &CConditionStateAbstract::conditions	() const
{
	return					(m_conditions);
}

TEMPLATE_SPECIALIZATION
IC	void CConditionStateAbstract::add_condition_back	(const COperatorCondition &condition)
{
	THROW					(m_conditions.empty() || (m_conditions.back().condition() < condition.condition()));
	m_conditions.push_back	(condition);
	m_hash					^= condition.hash_value();
}

TEMPLATE_SPECIALIZATION
IC	void CConditionStateAbstract::add_condition	(const COperatorCondition &condition)
{
	xr_vector<COperatorCondition>::iterator	I = std::lower_bound(m_conditions.begin(),m_conditions.end(),condition);
	THROW					((I == m_conditions.end()) || ((*I).condition() != condition.condition()));
	m_conditions.insert		(I,condition);
	m_hash					^= condition.hash_value();
}

TEMPLATE_SPECIALIZATION
IC	void CConditionStateAbstract::remove_condition	(const typename COperatorCondition::_condition_type &condition)
{
	xr_vector<COperatorCondition>::iterator	I = std::lower_bound(m_conditions.begin(),m_conditions.end(),COperatorCondition(condition,COperatorCondition::_value_type(0)));
	THROW					((I != m_conditions.end()) && ((*I).condition() == condition));
	m_hash					^= (*I).hash_value();
	m_conditions.erase		(I);
}

TEMPLATE_SPECIALIZATION
IC	void CConditionStateAbstract::add_condition	(typename xr_vector<COperatorCondition>::const_iterator &J, const COperatorCondition &condition)
{
	m_conditions.insert		(m_conditions.begin() + (J - m_conditions.begin()),condition);
	m_hash					^= condition.hash_value();
}

TEMPLATE_SPECIALIZATION
IC	void CConditionStateAbstract::clear	()
{
	m_conditions.clear		();
	m_hash					= 0;
}

TEMPLATE_SPECIALIZATION
IC	u8	CConditionStateAbstract::weight		(const CConditionState &condition) const
{
	u8						result = 0;
	xr_vector<COperatorCondition>::const_iterator	I = conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	E = conditions().end();
	xr_vector<COperatorCondition>::const_iterator	i = condition.conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	e = condition.conditions().end();
	for ( ; (I != E) && (i != e); )
		if ((*I).condition() < (*i).condition())
			++I;
		else
			if ((*I).condition() > (*i).condition())
				++i;
			else {
				if ((*I).value() != (*i).value())
					++result;
				++I;
				++i;
			}
	return					(result);
}

TEMPLATE_SPECIALIZATION
IC	bool CConditionStateAbstract::operator<	(const CConditionState &condition) const
{
	xr_vector<COperatorCondition>::const_iterator	I = conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	E = conditions().end();
	xr_vector<COperatorCondition>::const_iterator	i = condition.conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	e = condition.conditions().end();
	for ( ; (I != E) && (i != e); ++I, ++i)
		if (*I < *i)
			return			(true);
		else
			if (*i < *I)
				return		(false);
	if (I == E)
		if (i == e)
			return			(false);
		else
			return			(true);
	else
		return				(false);
}

TEMPLATE_SPECIALIZATION
IC	bool CConditionStateAbstract::operator==	(const CConditionState &condition) const
{
	if (hash_value() != condition.hash_value())
		return				(false);
	xr_vector<COperatorCondition>::const_iterator	I = conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	E = conditions().end();
	xr_vector<COperatorCondition>::const_iterator	i = condition.conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	e = condition.conditions().end();
	for ( ; (I != E) && (i != e); ++I, ++i)
		if (!(*I == *i))
			return			(false);
	if ((I == E) && (i == e))
		return				(true);
	return					(false);
}

TEMPLATE_SPECIALIZATION
IC	CConditionState<_world_property> &CConditionStateAbstract::operator-=(const CConditionState &condition)
{
	m_hash							= 0;
	xr_vector<COperatorCondition>	temp;
	xr_vector<COperatorCondition>::const_iterator	I = conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	E = conditions().end();
	xr_vector<COperatorCondition>::const_iterator	i = condition.conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	e = condition.conditions().end();
	for ( ; (I != E) && (i != e); )
		if ((*I).condition() < (*i).condition())
			++I;
		else
			if ((*I).condition() > (*i).condition())
				++i;
			else {
				if ((*I).value() != (*i).value()) {
					temp.push_back	(*I);
					m_hash			^= (*I).hash_value();
				}
				++I;
				++i;
			}
	m_conditions				= temp;
	return						(*this);
}

TEMPLATE_SPECIALIZATION
IC	bool CConditionStateAbstract::includes(const CConditionState &condition) const
{
	xr_vector<COperatorCondition>::const_iterator	I = conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	E = conditions().end();
	xr_vector<COperatorCondition>::const_iterator	i = condition.conditions().begin();
	xr_vector<COperatorCondition>::const_iterator	e = condition.conditions().end();
	for ( ; (I != E) && (i != e); )
		if ((*I).condition() < (*i).condition())
			++I;
		else
			if ((*I).condition() > (*i).condition())
				return			(false);
			else
				if ((*I).value() != (*i).value())
					return		(false);
				else {
					++I;
					++i;
				}
	return						(i == e);
}

TEMPLATE_SPECIALIZATION
IC	u32	CConditionStateAbstract::hash_value		() const
{
	return					(m_hash);
}

TEMPLATE_SPECIALIZATION
IC	const typename CConditionStateAbstract::COperatorCondition *CConditionStateAbstract::property (const typename CConditionStateAbstract::COperatorCondition::_condition_type &condition) const
{
	xr_vector<COperatorCondition>::const_iterator	I = std::lower_bound(conditions().begin(),conditions().end(),COperatorCondition(condition,COperatorCondition::_value_type(0)));
	if (I == m_conditions.end())
		return				(0);
	else
		return				(&*I);
}

#undef TEMPLATE_SPECIALIZATION
#undef CConditionStateAbstract