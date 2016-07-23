////////////////////////////////////////////////////////////////////////////
//	Module 		: script_token_list_inline.h
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script token list class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <algorithm>

IC CScriptTokenList::CScriptTokenList() { clear(); }
IC void CScriptTokenList::add(pcstr name, int id)
{
    VERIFY((token(name) == m_token_list.end()) && (token(id) == m_token_list.end()));
    xr_token temp;
    //! memory leak is here
    temp.name = xr_strdup(name);
    temp.id = id;
    m_token_list.pop_back();
    m_token_list.push_back(temp);
    ZeroMemory(&temp, sizeof(temp));
    m_token_list.push_back(temp);
}

IC void CScriptTokenList::remove(pcstr name)
{
    iterator I = token(name);
    VERIFY(I != m_token_list.end());
    m_token_list.erase(I);
}

IC void CScriptTokenList::clear()
{
    m_token_list.clear();
    xr_token temp;
    ZeroMemory(&temp, sizeof(temp));
    m_token_list.push_back(temp);
}

IC int CScriptTokenList::id(pcstr name)
{
    iterator I = token(name);
    VERIFY(I != m_token_list.end());
    return ((*I).id);
}

IC pcstr CScriptTokenList::name(int id)
{
    iterator I = token(id);
    VERIFY(I != m_token_list.end());
    return ((*I).name);
}

IC CScriptTokenList::iterator CScriptTokenList::token(pcstr name)
{
    return (std::find_if(m_token_list.begin(), m_token_list.end(), CTokenPredicateName(name)));
}

IC CScriptTokenList::iterator CScriptTokenList::token(int id)
{
    return (std::find_if(m_token_list.begin(), m_token_list.end(), CTokenPredicateID(id)));
}
