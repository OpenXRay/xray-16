#pragma once

IC void	CMonsterMorale::set_despondent()
{
	m_state	= eDespondent;
}

IC void	CMonsterMorale::set_take_heart()
{
	m_state	= eTakeHeart;
}

IC void	CMonsterMorale::set_normal_state()
{
	m_state	= eStable;
}

IC void	CMonsterMorale::change(float value)
{
	m_morale +=	value;
	clamp	(m_morale,0.f,1.f);
}

IC bool	CMonsterMorale::is_despondent()
{
	return ((m_state == eDespondent) || (m_morale < m_despondent_threshold));	
}

IC float CMonsterMorale::get_morale()
{
	return m_morale;
}
