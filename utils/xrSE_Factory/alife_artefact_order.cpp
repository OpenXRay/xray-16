////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_artefact_demand.h
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife artefact demand class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_artefact_order.h"

void SArtefactOrder::load	(NET_Packet &packet)
{
	packet.r_stringZ		(m_section);
	packet.r_u32			(m_count);
	packet.r_u32			(m_price);
}

void SArtefactOrder::save	(NET_Packet &packet)
{
	packet.w_stringZ		(m_section);
	packet.w_u32			(m_count);
	packet.w_u32			(m_price);
}

void SArtefactOrder::load	(IReader &packet)
{
	packet.r_stringZ		(m_section);
	m_count					= packet.r_u32();
	m_price					= packet.r_u32();
}

void SArtefactOrder::save	(IWriter &packet)
{
	packet.w_stringZ		(m_section);
	packet.w_u32			(m_count);
	packet.w_u32			(m_price);
}

