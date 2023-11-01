////////////////////////////////////////////////////////////////////////////
//	Module 		: wrapper_abstract_inline.h
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Abstract wrapper inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrGame/script_game_object.h"

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _object_type,\
		template <typename _base_object_type> class ancestor,\
		typename _base_object_type\
	>

/*
#define CWrapper CWrapperAbstract<_object_type,ancestor,_base_object_type>

TEMPLATE_SPECIALIZATION
void CWrapper::setup(_object_type *object)
{
    VERIFY(object);
    inherited::setup(object->lua_game_object());
    m_object = object;
}

TEMPLATE_SPECIALIZATION
void CWrapper::setup(CScriptGameObject *object)
{
    VERIFY(object);
    inherited::setup(object);
    m_object = smart_cast<_object_type*>(&object->object());
    VERIFY(m_object);
}

TEMPLATE_SPECIALIZATION
IC	_object_type &CWrapper::object() const
{
    VERIFY(m_object);
    return				(*m_object);
}

#undef CWrapper*/

//////////////////////////////////////////////////////////////////////////
// CWrapperAbstract2
//////////////////////////////////////////////////////////////////////////
#define CWrapper2 CWrapperAbstract2<_object_type, ancestor, _base_object_type>

TEMPLATE_SPECIALIZATION
IC	_object_type &CWrapper2::object() const
{
    VERIFY(m_object);
    return				(*m_object);
}

TEMPLATE_SPECIALIZATION
void CWrapper2::setup(_object_type *object, CPropertyStorage *storage)
{
    VERIFY(object);
    inherited::setup(object->lua_game_object(), storage);
    m_object = object;
}

TEMPLATE_SPECIALIZATION
void CWrapper2::setup(CScriptGameObject *object, CPropertyStorage *storage)
{
    VERIFY(object);
    inherited::setup(object, storage);
    m_object = smart_cast<_object_type*>(&object->object());
    VERIFY(m_object);
}

#undef CWrapper2
#undef TEMPLATE_SPECIALIZATION
