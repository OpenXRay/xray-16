#ifndef TRAFFIC_OPTIMIZATION_INCLUDED
#define TRAFFIC_OPTIMIZATION_INCLUDED

#include "xrCore/compression_ppmd_stream.h"
#include "xrCore/lzo_compressor.h"

namespace compression
{

namespace ppmd
{
	class stream;
};//namespace ppmd


typedef compression::ppmd::stream		ppmd_trained_stream;
void init_ppmd_trained_stream	(ppmd_trained_stream* & dest);
void deinit_ppmd_trained_stream	(ppmd_trained_stream* & src);


struct lzo_dictionary_buffer
{
	u8 *data;
	u32	size;
};

void init_lzo	(u8* & dest_wm, u8* & wm_buffer, lzo_dictionary_buffer & dest_dict);
void deinit_lzo	(u8* & src_wm_buffer, lzo_dictionary_buffer & src_dict);

} //namespace compression

enum enum_traffic_optimization
{
	eto_none				=	0,
	eto_ppmd_compression	=	1 << 0,
	eto_lzo_compression		=	1 << 1,
	eto_last_change			=	1 << 2,
};//enum enum_traffic_optimization

extern u32	g_sv_traffic_optimization_level;

#endif//#ifndef TRAFFIC_OPTIMIZATION_INCLUDED