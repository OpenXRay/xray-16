////////////////////////////////////////////////////////////////////////////
//	Module 		: wrapper_abstract.h
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Abastract wrapper
////////////////////////////////////////////////////////////////////////////

#pragma once

class CScriptGameObject;

template <
	typename _object_type,
	template <typename _base_object_type> class ancestor,
	typename _base_object_type = CScriptGameObject
>
class CWrapperAbstract : public ancestor<_base_object_type> {
protected:
	typedef ancestor<_base_object_type> inherited;

protected:
	_object_type	*m_object;

public:
	IC				CWrapperAbstract	();
	template <typename T1>
	IC				CWrapperAbstract	(T1 t1);
	template <typename T1, typename T2, typename T3>
	IC				CWrapperAbstract	(T1 t1, T2 t2, T3 t3);
	virtual			~CWrapperAbstract	();
	virtual void	setup				(_object_type *object);
	virtual void	setup				(CScriptGameObject *object);
	IC		_object_type &object		() const;
};

class CPropertyStorage;

template <
	typename _object_type,
	template <typename _base_object_type> class ancestor,
	typename _base_object_type = CScriptGameObject
>
class CWrapperAbstract2 : public ancestor<_base_object_type> {
protected:
	typedef ancestor<_base_object_type> inherited;

protected:
	_object_type	*m_object;

public:
	IC				CWrapperAbstract2	();
	template <typename T1>
	IC				CWrapperAbstract2	(T1 t1);
	template <typename T1, typename T2>
	IC				CWrapperAbstract2	(T1 t1, T2 t2);
	template <typename T1, typename T2, typename T3>
	IC				CWrapperAbstract2	(T1 t1, T2 t2, T3 t3);
	template <typename T1, typename T2, typename T3, typename T4>
	IC				CWrapperAbstract2	(T1 t1, T2 t2, T3 t3, T4 t4);
	template <typename T1, typename T2, typename T3, typename T4, typename T5>
	IC				CWrapperAbstract2	(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5);
	virtual			~CWrapperAbstract2	();
	virtual void	setup				(_object_type *object, CPropertyStorage *storage);
	virtual void	setup				(CScriptGameObject *object, CPropertyStorage *storage);
	IC		_object_type &object		() const;
};

#include "wrapper_abstract_inline.h"