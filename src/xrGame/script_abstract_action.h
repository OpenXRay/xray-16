////////////////////////////////////////////////////////////////////////////
//	Module 		: script_abstract_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script abstract action
////////////////////////////////////////////////////////////////////////////

#pragma once

class CScriptAbstractAction
{
public:
    bool m_bCompleted{ true };

public:
    CScriptAbstractAction() = default;
    virtual ~CScriptAbstractAction() = default;

    [[nodiscard]]
    bool completed() const { return m_bCompleted; }
};
