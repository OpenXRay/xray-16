////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_collection_storage_inline.h
//	Created 	: 13.10.2005
//  Modified 	: 13.10.2005
//	Author		: Dmitriy Iassenev
//	Description : sound collection storage inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSoundCollectionStorage &sound_collection_storage	()
{
	if (g_sound_collection_storage)
		return					(*g_sound_collection_storage);

	g_sound_collection_storage	= xr_new<CSoundCollectionStorage>();
	return						(*g_sound_collection_storage);
}
