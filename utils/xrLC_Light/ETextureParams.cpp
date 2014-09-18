#include "stdafx.h"

#include "etextureparams.h"
#include "serialize.h"
/*
   // texture part
    ETFormat	        fmt;
    Flags32		        flags;
    u32			        border_color;
    u32			        fade_color;
    u32			        fade_amount;
	u8					fade_delay;
    u32			        mip_filter;
    int			        width;
    int			        height;
    // detail ext
    shared_str			detail_name;
    float		        detail_scale;
    ETType		        type;
    // material
    ETMaterial			material;
    float				material_weight;
    // bump	
	float 				bump_virtual_height;
    ETBumpMode			bump_mode;
    shared_str			bump_name;
    shared_str			ext_normal_map_name;
*/
void STextureParams::read( INetReader	&r )
{
	r_pod( r, fmt );
	r_pod( r, flags );
	border_color = r.r_u32();
	fade_color = r.r_u32();
	fade_amount= r.r_u32();
	fade_delay = r.r_u8();;
	mip_filter = r.r_u32();
	width	= r.r_s32();;
	height	= r.r_s32();
	//r.r_stringZ( detail_name );
	detail_scale = r.r_float();
	r_pod( r, type );
	r_pod( r, material );
	material_weight = r.r_float();
	bump_virtual_height= r.r_float();
	r_pod( r,bump_mode );
	//r.r_stringZ( bump_name );
	//r.r_stringZ( ext_normal_map_name );
}
void STextureParams::write( IWriter	&w ) const 
{
	w_pod( w, fmt );
	w_pod( w, flags );
	w.w_u32( border_color );
	w.w_u32( fade_color );
	w.w_u32( fade_amount );
	w.w_u8( fade_delay );
	w.w_u32( mip_filter );
	w.w_s32( width );
	w.w_s32( height );
	//w.w_stringZ( detail_name );
	w.w_float( detail_scale );
	w_pod( w, type );
	w_pod( w, material );
	w.w_float( material_weight );
	w.w_float( bump_virtual_height );
	w_pod( w, bump_mode );
	//w.w_stringZ( bump_name );
	//w.w_stringZ( ext_normal_map_name );
}