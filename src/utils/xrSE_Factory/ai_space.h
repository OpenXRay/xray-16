////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_space.h
//	Created 	: 12.11.2003
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : AI space class
////////////////////////////////////////////////////////////////////////////

#pragma once

class CScriptEngine;

class CAI_Space
{
private:
    bool m_inited = false;

    void init();
    void RegisterScriptClasses();

public:
    CAI_Space() = default;
    CAI_Space(const CAI_Space&) = delete;
    CAI_Space& operator=(const CAI_Space&) = delete;
    virtual ~CAI_Space();
    static CAI_Space& GetInstance();

    IC CScriptEngine& script_engine() const;
};

IC CAI_Space& ai();

#include "ai_space_inline.h"
