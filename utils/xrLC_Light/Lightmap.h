// Lightmap.h: interface for the CLightmap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LIGHTMAP_H__889100E6_CF29_47EA_ABFD_41AE28DAC6B1__INCLUDED_)
#define AFX_LIGHTMAP_H__889100E6_CF29_47EA_ABFD_41AE28DAC6B1__INCLUDED_
#pragma once

#include "lm_layer.h"
#include "serialize.h"
// refs
class CDeflector;

// def
class XRLC_LIGHT_API CLightmap  
{
public:
	lm_layer					lm;
	b_texture					lm_texture;
public:
	CLightmap					();
	~CLightmap					();
	void	read				( INetReader	&r );
	void	write				( IWriter	&w )const;
	void	Capture				( CDeflector *D, int b_u, int b_v, int s_u, int s_v, BOOL bRotate );
	void	Save				( LPCSTR path );
static CLightmap*	read_create ( );
};

typedef  vector_serialize< t_read<CLightmap, get_id_standart<CLightmap>> >		tread_lightmaps;
typedef  vector_serialize< t_write<CLightmap, get_id_standart<CLightmap>>  >		twrite_lightmaps;

extern	tread_lightmaps		*read_lightmaps		;
extern	twrite_lightmaps	*write_lightmaps	;

#endif // !defined(AFX_LIGHTMAP_H__889100E6_CF29_47EA_ABFD_41AE28DAC6B1__INCLUDED_)
