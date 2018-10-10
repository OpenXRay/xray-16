////////////////////////////////////////////////////////////////////////////
//	Module 		: rat_state_base.cpp
//	Created 	: 31.08.2007
//  Modified 	: 31.08.2007
//	Author		: Dmitriy Iassenev
//	Description : rat state base class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "rat_state_base.h"

void rat_state_base::construct(CAI_Rat* object)
{
    VERIFY(object);
    m_object = object;
}
