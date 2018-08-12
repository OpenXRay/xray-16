////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_spawn_registry_header.cpp
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife spawn registry header
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_spawn_registry_header.h"
#include "alife_space.h"
#include "Common/LevelStructure.hpp"

CALifeSpawnHeader::~CALifeSpawnHeader() {}
void CALifeSpawnHeader::load(IReader& file_stream)
{
    m_version = file_stream.r_u32();
    R_ASSERT2(XRAI_CURRENT_VERSION == m_version, "'game.spawn' version mismatch!");
    file_stream.r(&m_guid, sizeof(m_guid));
    file_stream.r(&m_graph_guid, sizeof(m_graph_guid));
    m_count = file_stream.r_u32();
    m_level_count = file_stream.r_u32();
}
