// DynamicHeightMap.h: interface for the CDynamicHeightMap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DYNAMICHEIGHTMAP_H__5A5BD0B8_1FC7_4067_A5A4_D40422E8B6D1__INCLUDED_)
#define AFX_DYNAMICHEIGHTMAP_H__5A5BD0B8_1FC7_4067_A5A4_D40422E8B6D1__INCLUDED_
#pragma once

const int	dhm_line		= 4;
const int	dhm_matrix		= (dhm_line+1+dhm_line);	// 9x9 array
const float dhm_size		= 4.f;						// 4m per slot
const int	dhm_precision	= 16;						// 32x32 subdivs per slot
const int	dhm_total		= dhm_matrix*dhm_matrix;

//
class CHM_Static
{
	struct	Slot
	{
		float		data	[dhm_precision][dhm_precision];
		
		BOOL		bReady;
		int			x,z;
		
		IC			void		set		(int _x, int _z)	{ x=_x; z=_z; }
		IC			void		clear	()
		{
			for (u32 i=0; i<dhm_precision; ++i)
				for (u32 j=0; j<dhm_precision; ++j)
					data	[i][j]	= flt_min;
		}
		Slot()
		{
			clear	();
			set		(0,0);
			bReady	= TRUE;
		}
	};
	struct Poly
	{
		Fvector		v[3];
	};

	Slot						pool	[dhm_matrix*dhm_matrix];			// pool 
	Slot*						data	[dhm_matrix][dhm_matrix];			// database
	int							c_x,c_z;									// center of heighmap
	svector<Slot*,dhm_total>	task;
	xr_vector<Poly>				polys;
public: 
	void						Update		();
	float						Query		(float x, float z);				// 2D query
	
	CHM_Static					();
};

//
class CHM_Dynamic
{
public:
	void						Update		();
	float						Query		(float x, float z);				// 2D query
};

//
class CHeightMap
{
	CHM_Static					hm_static;
	CHM_Dynamic					hm_dynamic;
	u32						dwFrame;
public:
	float						Query		(float x, float z);				// 2D query
	Fvector						Query		(Fvector& pos, Fvector& dir);	// 3D ray-query
};

#endif // !defined(AFX_DYNAMICHEIGHTMAP_H__5A5BD0B8_1FC7_4067_A5A4_D40422E8B6D1__INCLUDED_)
