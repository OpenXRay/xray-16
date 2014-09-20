#ifndef	_XRLIGHT_IPLICIDDEFLECTOR
#define	_XRLIGHT_IPLICIDDEFLECTOR

#include "lm_layer.h"
#include "xrFacedefs.h"
struct  b_BuildTexture;
class ImplicitDeflector
{
public:
	b_BuildTexture*			texture;
	lm_layer				lmap;
	vecFace					faces;
	
	ImplicitDeflector() : texture(0)
	{
	}
	~ImplicitDeflector()
	{
		Deallocate	();
	}
	
	void			Allocate	()
	{
		lmap.create	(Width(),Height());
	}
	void			Deallocate	()
	{
		lmap.destroy();
	}
	
	u32			Width	()						;
	u32			Height	()						;	
	
	u32&		Texel	(u32 x, u32 y)			;
	base_color& Lumel	(u32 x, u32 y)			{ return lmap.surface[y*Width()+x];	}
	u8&			Marker	(u32 x, u32 y)			{ return lmap.marker [y*Width()+x];	}

	void		Bounds			(u32 ID, Fbox2& dest);
	void		Bounds_Summary	(Fbox2& bounds);

	void		read			( INetReader	&r );
	void		write			( IWriter	&w ) const ;
};
#endif