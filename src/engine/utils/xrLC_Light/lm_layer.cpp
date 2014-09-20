#include "stdafx.h"

#include "lm_layer.h"
#include "serialize.h"

void lm_layer::Pack		(xr_vector<u32>& dest)const	
{
	dest.resize			(width*height);
	xr_vector<base_color>::const_iterator I=surface.begin();
	xr_vector<base_color>::const_iterator E=surface.end();
	xr_vector<u32>::iterator		W=dest.begin();
	for (; I!=E; I++)
	{
		base_color_c	C; I->_get(C);
		u8	_r	= u8_clr(C.rgb.x);
		u8	_g	= u8_clr(C.rgb.y);
		u8	_b	= u8_clr(C.rgb.z);
		u8	_d	= u8_clr(C.sun);
		*W++	= color_rgba(_r,_g,_b,_d);
	}
}
void lm_layer::Pack_hemi	(xr_vector<u32>& dest)const	//.
{
	dest.resize			(width*height);
	xr_vector<base_color>::const_iterator I=surface.begin	();
	xr_vector<base_color>::const_iterator E=surface.end	();
	xr_vector<u32>::iterator		W=dest.begin	();
	for (; I!=E; I++)
	{
		base_color_c	C;	I->_get(C);
		u8	_d	= u8_clr	(C.sun);
		u8	_h	= u8_clr	(C.hemi);
		//*W++	= color_rgba(_h,_h,_h,_d);
		*W++	= color_rgba(_d,_d,_d,_h);
	}
}
void lm_layer::Pixel	(u32 ID, u8& r, u8& g, u8& b, u8& s, u8& h)
{
	xr_vector<base_color>::iterator I = surface.begin()+ID;
	base_color_c	c;	I->_get(c);
	r	= u8_clr(c.rgb.x);
	g	= u8_clr(c.rgb.y);
	b	= u8_clr(c.rgb.z);
	s	= u8_clr(c.sun);
	h	= u8_clr(c.hemi);
}

/*
	u32						width;
	u32						height;
	xr_vector<base_color>	surface;
	xr_vector<u8>			marker;
	LMODE					mode;	
*/



void lm_layer::read( INetReader	&r )
{
	width	=r.r_u32();
	height	=r.r_u32();
	r_pod_vector(r,surface);
	r_pod_vector(r,marker);
//	mode    =(LMODE)r.r_u8();	
}
void lm_layer::write( IWriter	&w ) const
{
	w.w_u32(width);
	w.w_u32(height);
	w_pod_vector(w,surface);
	w_pod_vector(w,marker);
//	w.w_u8((u8)mode);
}

bool	lm_layer::similar			( const lm_layer &layer, float eps/* =EPS*/ ) const
{
	//if( mode != layer.mode )
		//return false;
	if( marker.size() != layer.marker.size() )
		return false;
	for( u32 i = 0; i< marker.size(); ++i )
	{
		if( marker[i]!=layer.marker[i] )
		{
			return false;
		}
	}
	if( surface.size() != layer.surface.size() )
		return false;
	for( u32 i = 0; i < surface.size(); ++i )
	{
		if( !surface[i].similar( layer.surface[i], EPS ) )
		{
			Msg("sufface diff id: %d", i);
			return false;
		}
	}

	return width ==  layer.width &&
		   height == layer.height;

}