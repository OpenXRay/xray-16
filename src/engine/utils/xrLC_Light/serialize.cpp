#include "stdafx.h"

#include "serialize.h"





void write(IWriter	&w, const b_texture &b)
{
	w.w_string( b.name );
	w.w_u32( b.dwWidth );
	w.w_u32( b.dwHeight );
	w.w_s32( b.bHasAlpha );
	
	bool b_surface = !!b.pSurface;
	w.w_u8( u8( b_surface ) );
	if(b_surface)
	{
		u32 size = sizeof( u32 ) *  b.dwWidth * b.dwHeight;
		w.w( b.pSurface, size );
	}
}
void read(INetReader &r, b_texture &b)
{
	r.r_string( b.name, sizeof(b.name) );
	b.dwWidth = r.r_u32(  );
	b.dwHeight = r.r_u32( );
	b.bHasAlpha = r.r_s32(  );

	b.pSurface = NULL;
	bool b_surface = !!r.r_u8();
	if(b_surface)
	{
		u32 size = sizeof( u32 ) *  b.dwWidth * b.dwHeight;
		b.pSurface = (u32*)xr_malloc( size );
		r.r( b.pSurface, size );
	}
}


