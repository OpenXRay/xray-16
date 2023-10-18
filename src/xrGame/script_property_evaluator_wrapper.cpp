////////////////////////////////////////////////////////////////////////////
//	Module 		: script_property_evaluator_wrapper.cpp
//	Created 	: 19.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script property evaluator wrapper
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_property_evaluator_wrapper.h"
#include "script_game_object.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"

void CScriptPropertyEvaluatorWrapper::setup(CScriptGameObject* object, CPropertyStorage* storage)
{
    luabind::call_member<void>(this, "setup", object, storage);
}

void CScriptPropertyEvaluatorWrapper::setup_static(
    CScriptPropertyEvaluator* evaluator, CScriptGameObject* object, CPropertyStorage* storage)
{
    evaluator->CScriptPropertyEvaluator::setup(object, storage);
}

bool CScriptPropertyEvaluatorWrapper::evaluate()
{
    try
    {
        return (luabind::call_member<bool>(this, "evaluate"));
    }
#if defined(DEBUG) && !defined(LUABIND_NO_EXCEPTIONS)
    catch (const luabind::cast_failed& exception)
    {
#ifdef LOG_ACTION
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "SCRIPT RUNTIME ERROR : evaluator [%s] returns value with not a %s type!", m_evaluator_name,
            exception.info().name());
#else
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "SCRIPT RUNTIME ERROR : evaluator returns value with not a %s type!", exception.info().name());
#endif
    }
#endif
    catch (...)
    {
        //Alundaio: m_evaluator_name
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "SCRIPT RUNTIME ERROR : evaluator [%s] returns value with not a bool type!", m_evaluator_name);
    }
    return (false);
}

bool CScriptPropertyEvaluatorWrapper::evaluate_static(CScriptPropertyEvaluator* evaluator)
{
    return (evaluator->CScriptPropertyEvaluator::evaluate());
}
