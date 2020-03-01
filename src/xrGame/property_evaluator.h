////////////////////////////////////////////////////////////////////////////
//	Module 		: property_evaluator.h
//	Created 	: 12.03.2004
//  Modified 	: 12.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property evaluator
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "property_storage.h"
#include "action_management_config.h"

class CScriptGameObject;

template <typename _object_type>
class CPropertyEvaluator
{
public:
    typedef GraphEngineSpace::_solver_condition_type _condition_type;
    typedef GraphEngineSpace::_solver_value_type _value_type;

public:
    _object_type* m_object;
    CPropertyStorage* m_storage;
#if 1//def LOG_ACTION //Alundaio: m_evaluator_name
    LPCSTR m_evaluator_name;
#endif

public:
    IC CPropertyEvaluator(_object_type* object = 0, LPCSTR evaluator_name = "");
    virtual ~CPropertyEvaluator();
    IC void init(_object_type* object, LPCSTR evaluator_name);
    virtual void setup(_object_type* object, CPropertyStorage* storage);
    virtual void Load(LPCSTR section);
    virtual _value_type evaluate();
    IC const _value_type& property(const _condition_type& condition_id) const;

    virtual void save(NET_Packet& packet) {}
    virtual void load(IReader& packet) {}
};
typedef CPropertyEvaluator<CScriptGameObject> CScriptPropertyEvaluator;

#include "property_evaluator_inline.h"
