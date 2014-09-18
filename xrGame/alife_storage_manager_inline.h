////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_storage_manager_inline.h
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator storage manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeStorageManager::CALifeStorageManager			(xrServer *server, LPCSTR section) :
	inherited	(server,section)
{
	m_section				= section;
	xr_strcpy					(m_save_name,"");
}
