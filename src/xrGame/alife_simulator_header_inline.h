////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_simulator_header_inline.h
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator header inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeSimulatorHeader::CALifeSimulatorHeader(LPCSTR	section) :
	m_version	(ALIFE_VERSION)
{
}

IC	u32 CALifeSimulatorHeader::version			() const
{
	return		(m_version);
}
