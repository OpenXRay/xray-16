////////////////////////////////////////////////////////////////////////////
//	Module 		: script_token_list.h
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script token list class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCommon/xr_vector.h"
#include "xrCore/xrstring.h"
#include "xrCore/xr_token.h"

class CScriptTokenList
{
protected:
    typedef xr_vector<xr_token> TOKEN_LIST;
    typedef TOKEN_LIST::iterator iterator;
    typedef TOKEN_LIST::const_iterator const_iterator;

protected:
    struct CTokenPredicateName
    {
        pcstr m_name;

        CTokenPredicateName(pcstr name) throw() : m_name(name) {}
        IC bool operator()(const xr_token& token) const throw() { return token.name && !xr_strcmp(token.name, m_name); }
    };

    struct CTokenPredicateID
    {
        int m_id;

        CTokenPredicateID(int id) throw() : m_id(id) {}
        IC bool operator()(const xr_token& token) const { return token.name && (token.id == m_id); }
    };

protected:
    TOKEN_LIST m_token_list;

protected:
    IC iterator token(pcstr name);
    IC iterator token(int id);

public:
    IC CScriptTokenList();
    ~CScriptTokenList();
    IC void add(pcstr name, int id);
    IC void remove(pcstr name);
    IC void clear();
    IC int id(pcstr name);
    IC pcstr name(int id);
    const TOKEN_LIST& tokens() const throw() { return m_token_list; }
    TOKEN_LIST& tokens() throw() { return m_token_list; }
};

#include "script_token_list_inline.h"
