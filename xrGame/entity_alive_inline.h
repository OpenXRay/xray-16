#pragma once

IC	CEntityCondition &CEntityAlive::conditions	() const
{
	VERIFY		(m_entity_condition);
	return		(*m_entity_condition);
}

IC	bool const &CEntityAlive::is_agresive		() const
{
	return		(m_is_agresive);
}

IC	void CEntityAlive::is_agresive				(bool const &value)
{
	m_is_agresive = value;
}

IC	bool const &CEntityAlive::is_start_attack	() const
{
	return		(m_is_start_attack);
}

IC	void CEntityAlive::is_start_attack			(bool const &value)
{
	m_is_start_attack = value;
}