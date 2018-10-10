////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_cover_location.cpp
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger cover location
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "danger_cover_location.h"
#include "cover_point.h"

const Fvector& CDangerCoverLocation::position() const { return (m_cover->position()); }
