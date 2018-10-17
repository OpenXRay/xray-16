////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_space_inline.h
//	Created 	: 12.11.2003
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : AI space class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC CScriptEngine& CAI_Space::script_engine() const
{
    VERIFY(GEnv.ScriptEngine);
    return *GEnv.ScriptEngine;
}

IC CAI_Space& ai() { return CAI_Space::GetInstance(); }
