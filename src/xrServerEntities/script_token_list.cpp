////////////////////////////////////////////////////////////////////////////
//	Module 		: script_token_list.cpp
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script token list class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "script_token_list.h"
#include <algorithm>

CScriptTokenList::CScriptTokenList()
{
    clear();
}

CScriptTokenList::~CScriptTokenList() noexcept
{
    // XXX: Xottab_DUTY: Replace with range-based for
    iterator I = tokens().begin();
    iterator E = tokens().end();
    for (; I != E; ++I)
        xr_free((*I).name);
}

void CScriptTokenList::add(pcstr name, int id)
{
    VERIFY(token(name) == m_token_list.end() && token(id) == m_token_list.end());
    xr_token temp;
#pragma todo("Memory leak from xr_strdup() of token name!")
    //! memory leak is here
    temp.name = xr_strdup(name);
    temp.id = id;
    m_token_list.pop_back();
    m_token_list.push_back(temp);
    ZeroMemory(&temp, sizeof temp);
    m_token_list.push_back(temp);
}

void CScriptTokenList::remove(pcstr name)
{
    iterator I = token(name);
    VERIFY(I != m_token_list.end());
    m_token_list.erase(I);
}

void CScriptTokenList::clear()
{
    m_token_list.clear();
    xr_token temp;
    ZeroMemory(&temp, sizeof(temp));
    m_token_list.push_back(temp);
}

int CScriptTokenList::id(pcstr name)
{
    iterator I = token(name);
    VERIFY(I != m_token_list.end());
    return (*I).id;
}

pcstr CScriptTokenList::name(int id)
{
    iterator I = token(id);
    VERIFY(I != m_token_list.end());
    return (*I).name;
}

CScriptTokenList::iterator CScriptTokenList::token(pcstr name)
{
    return std::find_if(m_token_list.begin(), m_token_list.end(), CTokenPredicateName(name));
}

CScriptTokenList::iterator CScriptTokenList::token(int id)
{
    return std::find_if(m_token_list.begin(), m_token_list.end(), CTokenPredicateID(id));
}
