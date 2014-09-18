#pragma once

class XRLC_LIGHT_API base_basis
{
public:
	u8						x,y,z;
	void	set				(Fvector	N)
	{
		N.normalize_safe	();
		N.add				(1.f);
		N.mul				(.5f*255.f);
		s32 nx				= iFloor(N.x);	clamp(nx,0,255);	x=u8(nx);
		s32 ny				= iFloor(N.y);	clamp(ny,0,255);	y=u8(ny);
		s32 nz				= iFloor(N.z);	clamp(nz,0,255);	z=u8(nz);
	}
	void	set				(float x, float y, float z)
	{
		Fvector	N			= { x,y,z };
		set					(N);
	}
	bool	similar			(const base_basis& o);
};