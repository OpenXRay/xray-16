#include "stdafx.h"
#include "traffic_optimization.h"

namespace compression
{

void init_ppmd_trained_stream	(ppmd_trained_stream* & dest)
{
	VERIFY				(dest == NULL);
	string_path			file_name;
	FS.update_path		(file_name,"$game_config$", "mp\\ppmd_updates.mdl");
	R_ASSERT2			(FS.exist(file_name), "can't find configs\\mp\\ppmd_updates.mdl");
	
	IReader*			reader = FS.r_open(file_name);
	R_ASSERT			(reader);
	u32					buffer_size = reader->length();
	u8*					buffer = (u8*)xr_malloc(buffer_size);
	reader->r			(buffer,buffer_size);
	FS.r_close			(reader);
	dest				= xr_new<compression::ppmd::stream>(buffer,buffer_size);
}

void deinit_ppmd_trained_stream	(ppmd_trained_stream* & src)
{
	VERIFY(src);
	src->rewind	();
	u8* buffer	= src->buffer();
	xr_free		(buffer);
	xr_delete	(src);
}


void init_lzo	(u8* & dest_wm, u8* & wm_buffer, lzo_dictionary_buffer & dest_dict)
{
	lzo_initialize();
	wm_buffer	= static_cast<u8*>(xr_malloc(LZO1X_999_MEM_COMPRESS+16));
	// buffer must be alligned to 16 bytes
    dest_wm		= (u8*)(size_t(wm_buffer + 16) & ~0xf);

	string_path			file_name;
	FS.update_path		(file_name, "$game_config$", "mp\\lzo_updates.dic");
	R_ASSERT2			(FS.exist(file_name), "can't find configs\\mp\\lzo_updates.dic");
	IReader*			reader = FS.r_open(file_name);
	u32					buffer_size = reader->length();
	u8*					buffer = (u8*)xr_malloc(buffer_size);
	reader->r			(buffer,buffer_size);
	FS.r_close			(reader);
	
	dest_dict.data		= buffer;
	dest_dict.size		= buffer_size;
}

void deinit_lzo	(u8* & src_wm_buffer, lzo_dictionary_buffer & src_dict)
{
	xr_free(src_wm_buffer);
	xr_free(src_dict.data);
}

}//namespace compression