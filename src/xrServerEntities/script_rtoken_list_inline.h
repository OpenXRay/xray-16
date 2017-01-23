////////////////////////////////////////////////////////////////////////////
//	Module 		: script_rtoken_list_inline.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script rtoken list class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	void	CScriptRTokenList::add		(LPCSTR value)
{
	m_values.push_back(shared_str(value));
}

IC	void	CScriptRTokenList::remove	(u32 index)
{
	if (index >= size())
		return;
	m_values.erase	(m_values.begin() + index);
}

IC	LPCSTR	CScriptRTokenList::get		(u32 index)
{
	if (index >= size())
		return	(0);
	return		(*m_values[index]);
}

IC	u32		CScriptRTokenList::size		()
{
	return			(u32(m_values.size()));
}

IC	void	CScriptRTokenList::clear	()
{
	m_values.clear	();
}

IC	CScriptRTokenList::RTOKEN_LIST	&CScriptRTokenList::tokens	()
{
	return			(m_values);
}
