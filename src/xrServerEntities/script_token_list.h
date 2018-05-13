////////////////////////////////////////////////////////////////////////////
//	Module 		: script_token_list.h
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script token list class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCommon/xr_vector.h"
#include "xrCore/xr_token.h"

class CScriptTokenList
{
protected:
    // XXX: tamlin: See if it's possible to place the vector in an impl.
    using TOKEN_LIST = xr_vector<xr_token>;
    using iterator = TOKEN_LIST::iterator;
    using const_iterator = TOKEN_LIST::const_iterator;

protected:
    struct CTokenPredicateName
    {
        pcstr m_name;

        constexpr CTokenPredicateName(pcstr name) noexcept : m_name(name) {}
        IC bool operator()(const xr_token& token) const noexcept { return token.name && !xr_strcmp(token.name, m_name); }
    };

    struct CTokenPredicateID
    {
        int m_id;

        CTokenPredicateID(int id) noexcept : m_id(id) {}
        IC bool operator()(const xr_token& token) const { return token.name && (token.id == m_id); }
    };

    TOKEN_LIST m_token_list;

    iterator token(pcstr name);
    iterator token(int id);

public:
    CScriptTokenList();
    ~CScriptTokenList();
    void add(pcstr name, int id);
    void remove(pcstr name);
    void clear();
    int id(pcstr name);
    pcstr name(int id);
    const TOKEN_LIST& tokens() const noexcept { return m_token_list; }
    TOKEN_LIST& tokens() noexcept { return m_token_list; }
};
