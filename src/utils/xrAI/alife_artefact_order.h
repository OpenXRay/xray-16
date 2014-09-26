////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_artefact_demand.h
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife artefact demand class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "object_interfaces.h"

struct SArtefactOrder : 
	public IPureSerializeObject<IReader,IWriter>,
	public IPureSerializeObject<NET_Packet,NET_Packet>
{
	shared_str		m_section;
	u32				m_count;
	u32				m_price;

	virtual void	load		(NET_Packet &packet);
	virtual void	save		(NET_Packet &packet);
	virtual void	load		(IReader &packet);
	virtual void	save		(IWriter &packet);
};
