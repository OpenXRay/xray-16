#pragma once

struct	r_aabb_ssa	{
	u8	ssa	[6]		;
};

class	r_pixel_calculator
{
	ref_rt						rt;
	IDirect3DSurface9*			zb;
public:
	void			begin		();
	r_aabb_ssa		calculate	(dxRender_Visual* V);
	void			end			();

	void			run			();
};
