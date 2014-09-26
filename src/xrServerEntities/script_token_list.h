////////////////////////////////////////////////////////////////////////////
//	Module 		: script_token_list.h
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script token list class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

class CScriptTokenList {
protected:
	typedef xr_vector<xr_token>			TOKEN_LIST;
	typedef TOKEN_LIST::iterator		iterator;
	typedef TOKEN_LIST::const_iterator	const_iterator;

protected:
	struct CTokenPredicateName {
		LPCSTR			m_name;

		IC				CTokenPredicateName	(LPCSTR name)
		{
			m_name		= name;
		}

		IC		bool	operator()		(const xr_token &token) const
		{
			return		(token.name && !xr_strcmp(token.name,m_name));
		}
	};
	
	struct CTokenPredicateID {
		int				m_id;

		IC				CTokenPredicateID	(int id)
		{
			m_id		= id;
		}

		IC		bool	operator()		(const xr_token &token) const
		{
			return		(token.name && (token.id == m_id));
		}
	};

protected:
	TOKEN_LIST					m_token_list;

protected:
	IC		iterator			token				(LPCSTR name);
	IC		iterator			token				(int id);

public:
	IC							CScriptTokenList	();
								~CScriptTokenList	();
	IC		void				add					(LPCSTR name, int id);
	IC		void				remove				(LPCSTR name);
	IC		void				clear				();
	IC		int					id					(LPCSTR name);
	IC		LPCSTR				name				(int id);
	IC		const TOKEN_LIST	&tokens				() const;
	IC		TOKEN_LIST			&tokens				();
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptTokenList)
#undef script_type_list
#define script_type_list save_type_list(CScriptTokenList)

#include "script_token_list_inline.h"