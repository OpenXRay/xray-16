////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_data_storage_inline.h
//	Created 	: 13.10.2005
//  Modified 	: 13.10.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation data storage inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CStalkerAnimationDataStorage &stalker_animation_data_storage	()
{
	if (g_stalker_animation_data_storage)
		return							(*g_stalker_animation_data_storage);

	g_stalker_animation_data_storage	= xr_new<CStalkerAnimationDataStorage>();
	return								(*g_stalker_animation_data_storage);
}
