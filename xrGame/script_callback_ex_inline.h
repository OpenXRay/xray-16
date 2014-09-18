////////////////////////////////////////////////////////////////////////////
//	Module 		: script_callback_ex_inline.h
//	Created 	: 06.02.2004
//  Modified 	: 11.01.2005
//	Author		: Sergey Zhemeitsev and Dmitriy Iassenev
//	Description : Script callbacks with return value inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION			template <typename _return_type>
#define CSScriptCallbackEx				CScriptCallbackEx_<_return_type>

TEMPLATE_SPECIALIZATION
IC	CSScriptCallbackEx::CScriptCallbackEx_				() 
{
}

TEMPLATE_SPECIALIZATION
IC	CSScriptCallbackEx::~CScriptCallbackEx_				()
{
}

TEMPLATE_SPECIALIZATION
IC	void CSScriptCallbackEx::clear						()
{
	m_functor.~functor_type			();
	new (&m_functor) functor_type	();

	m_object.~object_type			();
	new (&m_object)	object_type		();
}

TEMPLATE_SPECIALIZATION
IC	CSScriptCallbackEx::CScriptCallbackEx_				(const CScriptCallbackEx_ &callback)
{
	clear				();
	*this				= callback;
}

TEMPLATE_SPECIALIZATION
IC	CSScriptCallbackEx &CSScriptCallbackEx::operator=	(const CScriptCallbackEx_ &callback)
{
	clear				();
	
	if (callback.m_functor.is_valid() && callback.m_functor.lua_state())
		m_functor		= callback.m_functor;

	if (callback.m_object.is_valid() && callback.m_object.lua_state())
		m_object		= callback.m_object;

	return				(*this);
}

TEMPLATE_SPECIALIZATION
IC	void CSScriptCallbackEx::set						(const functor_type &functor)
{
	clear				();
	m_functor			= functor;
}

TEMPLATE_SPECIALIZATION
IC	void CSScriptCallbackEx::set						(const functor_type &functor, const object_type &object)
{
	clear				();
	
	m_functor			= functor;
	m_object			= object;
}

TEMPLATE_SPECIALIZATION
IC	bool CSScriptCallbackEx::empty						() const
{
	return				(!!m_functor.lua_state());
}

#undef TEMPLATE_SPECIALIZATION
#undef CSScriptCallbackEx