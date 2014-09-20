////////////////////////////////////////////////////////////////////////////
//	Module 		: wrapper_abstract_inline.h
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Abastract wrapper inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

//#include "script_game_object.h"
class CScriptGameObject;

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _object_type,\
		template <typename _base_object_type> class ancestor,\
		typename _base_object_type\
	>
#define CWrapper CWrapperAbstract<_object_type,ancestor,_base_object_type>
#define CWrapper2 CWrapperAbstract2<_object_type,ancestor,_base_object_type>

TEMPLATE_SPECIALIZATION
IC	CWrapper::CWrapperAbstract	()
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
template <typename T1>
IC	CWrapper::CWrapperAbstract	(T1 t1) :
	inherited			(t1)
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
template <typename T1, typename T2, typename T3>
IC	CWrapper::CWrapperAbstract	(T1 t1, T2 t2, T3 t3) :
	inherited			(t1,t2,t3)
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
CWrapper::~CWrapperAbstract		()
{
}

TEMPLATE_SPECIALIZATION
void CWrapper::setup				(_object_type *object)
{
	VERIFY					(object);
	inherited::setup		(object->lua_game_object());
	m_object				= object;
}

TEMPLATE_SPECIALIZATION
void CWrapper::setup				(CScriptGameObject *object)
{
	VERIFY					(object);
	inherited::setup		(object);
	m_object				= smart_cast<_object_type*>(&object->object());
	VERIFY					(m_object);
}

TEMPLATE_SPECIALIZATION
IC	_object_type &CWrapper::object	() const
{
	VERIFY				(m_object);
	return				(*m_object);
}

//////////////////////////////////////////////////////////////////////////
// CWrapperAbstract2
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
IC	CWrapper2::CWrapperAbstract2	()
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
template <typename T1>
IC	CWrapper2::CWrapperAbstract2	(T1 t1) :
	inherited			(t1)
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
IC	_object_type &CWrapper2::object	() const
{
	VERIFY				(m_object);
	return				(*m_object);
}

TEMPLATE_SPECIALIZATION
template <typename T1, typename T2, typename T3>
IC	CWrapper2::CWrapperAbstract2	(T1 t1, T2 t2, T3 t3) :
	inherited			(t1,t2,t3)
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
template <typename T1, typename T2>
IC	CWrapper2::CWrapperAbstract2	(T1 t1, T2 t2) :
	inherited			(t1,t2)
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
template <typename T1, typename T2, typename T3, typename T4>
IC	CWrapper2::CWrapperAbstract2	(T1 t1, T2 t2, T3 t3, T4 t4) :
	inherited			(t1,t2,t3,t4)
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
template <typename T1, typename T2, typename T3, typename T4, typename T5>
IC	CWrapper2::CWrapperAbstract2	(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) :
	inherited			(t1,t2,t3,t4,t5)
{
	m_object			= 0;
}

TEMPLATE_SPECIALIZATION
CWrapper2::~CWrapperAbstract2		()
{
}

TEMPLATE_SPECIALIZATION
void CWrapper2::setup				(_object_type *object, CPropertyStorage *storage)
{
	VERIFY					(object);
	inherited::setup		(object->lua_game_object(),storage);
	m_object				= object;
}

TEMPLATE_SPECIALIZATION
void CWrapper2::setup				(CScriptGameObject *object, CPropertyStorage *storage)
{
	VERIFY					(object);
	inherited::setup		(object,storage);
	m_object				= smart_cast<_object_type*>(&object->object());
	VERIFY					(m_object);
}

#undef TEMPLATE_SPECIALIZATION
#undef CWrapper