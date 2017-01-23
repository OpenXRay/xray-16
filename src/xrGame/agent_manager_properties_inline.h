////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_manager_properties_inline.h
//	Created 	: 25.05.2004
//  Modified 	: 25.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Agent manager properties inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CAgentManagerPropertyEvaluatorItem::CAgentManagerPropertyEvaluatorItem		(CAgentManager *object, LPCSTR evaluator_name) :
	inherited(object,evaluator_name)
{
}

IC	CAgentManagerPropertyEvaluatorEnemy::CAgentManagerPropertyEvaluatorEnemy	(CAgentManager *object, LPCSTR evaluator_name) :
	inherited(object,evaluator_name)
{
}

IC	CAgentManagerPropertyEvaluatorDanger::CAgentManagerPropertyEvaluatorDanger	(CAgentManager *object, LPCSTR evaluator_name) :
	inherited(object,evaluator_name)
{
}
