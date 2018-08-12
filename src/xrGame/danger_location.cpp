////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_location.cpp
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger location
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "danger_location.h"

bool CDangerLocation::useful() const { return (!(Device.dwTimeGlobal > m_level_time + m_interval)); }
