////////////////////////////////////////////////////////////////////////////
//	Module 		: script_token_list.cpp
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script token list class
////////////////////////////////////////////////////////////////////////////

#include "script_token_list.h"
#include "stdafx.h"

CScriptTokenList::~CScriptTokenList()
{
    iterator I = tokens().begin();
    iterator E = tokens().end();
    for (; I != E; ++I)
        xr_free((*I).name);
}
