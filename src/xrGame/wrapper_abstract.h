////////////////////////////////////////////////////////////////////////////
//	Module 		: wrapper_abstract.h
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Abstract wrapper
////////////////////////////////////////////////////////////////////////////

#pragma once

class CScriptGameObject;
class CPropertyStorage;

/* Xottab_DUTY: commented since I can't compile it and it's unused
template <typename _object_type, template <typename _base_object_type> class ancestor,
    typename _base_object_type = CScriptGameObject>
class CWrapperAbstract : public ancestor<_base_object_type>
{
protected:
    using inherited = ancestor<__base_object_type>;

    _object_type* m_object;

public:
    CWrapperAbstract() noexcept : m_object(nullptr) {}

    template <typename T1>
    CWrapperAbstract(T1 t1) : inherited(t1), m_object(nullptr) {}

    template <typename T1, typename T2, typename T3>
    CWrapperAbstract(T1 t1, T2 t2, T3 t3) : inherited(t1, t2, t3), m_object(nullptr) {}

    virtual ~CWrapperAbstract() {}
    virtual void setup(_object_type* object);
    virtual void setup(CScriptGameObject* object);
    _object_type& object() const;
};*/

template <typename _object_type, template <typename _base_object_type> class ancestor,
    typename _base_object_type = CScriptGameObject>
class CWrapperAbstract2 : public ancestor<_base_object_type>
{
protected:
    using inherited = ancestor<_base_object_type>;

    _object_type* m_object;

public:
    CWrapperAbstract2() noexcept : m_object(nullptr) {}

    template <typename T1>
    CWrapperAbstract2(T1 t1) : inherited(t1), m_object(nullptr) {}

    template <typename T1, typename T2>
    CWrapperAbstract2(T1 t1, T2 t2) : inherited(t1, t2), m_object(nullptr) {}

    template <typename T1, typename T2, typename T3>
    CWrapperAbstract2(T1 t1, T2 t2, T3 t3) : inherited(t1, t2, t3), m_object(nullptr) {}

    template <typename T1, typename T2, typename T3, typename T4>
    CWrapperAbstract2(T1 t1, T2 t2, T3 t3, T4 t4) : inherited(t1, t2, t3, t4), m_object(nullptr) {}

    template <typename T1, typename T2, typename T3, typename T4, typename T5>
    CWrapperAbstract2(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) : inherited(t1, t2, t3, t4, t5), m_object(nullptr) {}

    virtual ~CWrapperAbstract2() {}
    virtual void setup(_object_type* object, CPropertyStorage* storage);
    virtual void setup(CScriptGameObject* object, CPropertyStorage* storage);
    _object_type& object() const;
};

#include "wrapper_abstract_inline.h"
