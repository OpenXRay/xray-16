////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_simulator_header.h
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator header
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "object_interfaces.h"
#include "alife_space.h"

class CALifeSimulatorHeader {
protected:
	u32								m_version;

public:
	IC								CALifeSimulatorHeader	(LPCSTR section);
	virtual							~CALifeSimulatorHeader	();
	virtual void					save					(IWriter &tMemoryStream);
	virtual void					load					(IReader &tFileStream);
	IC		u32						version					() const;
			bool					valid					(IReader &file_stream) const;
};

#include "alife_simulator_header_inline.h"