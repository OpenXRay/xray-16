////////////////////////////////////////////////////////////////////////////
//	Module 		: property_evaluator_const.h
//	Created 	: 12.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property evaluator const
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "property_evaluator.h"

template <typename _object_type>
class CPropertyEvaluatorConst : public CPropertyEvaluator<_object_type>
{
protected:
    using inherited = CPropertyEvaluator<_object_type>;

public:
    using _value_type = typename inherited::_value_type;

protected:
    _value_type m_value;

public:
    CPropertyEvaluatorConst(_value_type value, LPCSTR evaluator_name = "") : m_value(value)
    {
#if 1//def LOG_ACTION //Alundaio: m_evaluator_name
        this->m_evaluator_name = evaluator_name;
#endif
    }

    virtual _value_type evaluate()
    {
        return m_value;
    }
};
