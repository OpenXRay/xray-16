////////////////////////////////////////////////////////////////////////////
//	Module 		: script_value_wrapper_inline.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script value wrapper inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T>
IC CScriptValueWrapperImpl<T>::CScriptValueWrapperImpl(luabind::object object, LPCSTR name)
    : inherited(object, name)
{
    m_value = luabind::object_cast<T>(object[name]);
}

template <typename T>
IC void CScriptValueWrapperImpl<T>::assign()
{
    m_object[*m_name] = m_value;
}

template <typename T>
IC T* CScriptValueWrapperImpl<T>::value()
{
    return (&m_value);
}

template <typename T>
IC CScriptValueWrapper<T>::CScriptValueWrapper(luabind::object object, LPCSTR name) : inherited(object, name)
{
}
