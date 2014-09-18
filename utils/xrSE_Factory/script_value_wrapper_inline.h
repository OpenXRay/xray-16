////////////////////////////////////////////////////////////////////////////
//	Module 		: script_value_wrapper_inline.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script value wrapper inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename _type>
IC	CScriptValueWrapperImpl<_type>::CScriptValueWrapperImpl	(luabind::object object, LPCSTR name) :
	inherited			(object,name)
{
	m_value				= luabind::object_cast<_type>(object[name]);
}

template <typename _type>
IC	void CScriptValueWrapperImpl<_type>::assign		()
{
	m_object[*m_name]	= m_value;
}

template <typename _type>
IC	_type *CScriptValueWrapperImpl<_type>::value		()
{
	return				(&m_value);
}

template <typename _type>
IC	CScriptValueWrapper<_type>::CScriptValueWrapper	(luabind::object object, LPCSTR name) :
	inherited			(object,name)
{
}
