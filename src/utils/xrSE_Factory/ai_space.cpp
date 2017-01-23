////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_space.h
//	Created 	: 12.11.2003
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : AI space class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"

CAI_Space *g_ai_space = 0;

CAI_Space::CAI_Space				()
{
	m_script_engine			= 0;
}

void CAI_Space::init				()
{
	VERIFY					(!m_script_engine);
	m_script_engine			= xr_new<CScriptEngine>();
	m_script_engine->init	();
#error additional initialization is required here: RegisterScriptClasses + object_factory().register_script()
    //RegisterScriptClasses();
    //object_factory().register_script();
    //LoadCommonScripts(); // for game only
}

CAI_Space::~CAI_Space				()
{
	xr_delete				(m_script_engine);
}
