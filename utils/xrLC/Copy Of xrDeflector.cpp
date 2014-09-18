#include "stdafx.h"
#include "resource.h"
#include "build.h"
#include "xrIsect.h"
#include "math.h"

void lm_layer::Pack		(xr_vector<u32>& dest)	
{
	dest.resize			(width*height);
	xr_vector<base_color>::iterator I=surface.begin();
	xr_vector<base_color>::iterator E=surface.end();
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
void lm_layer::Pack_hemi	(xr_vector<u32>& dest)	//.
{
	dest.resize			(width*height);
	xr_vector<base_color>::iterator I=surface.begin	();
	xr_vector<base_color>::iterator E=surface.end	();
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

void blit			(u32* dest, u32 ds_x, u32 ds_y, u32* src, u32 ss_x, u32 ss_y, u32 px, u32 py, u32 aREF)
{
	R_ASSERT(ds_x>=(ss_x+px));
	R_ASSERT(ds_y>=(ss_y+py));
	for (u32 y=0; y<ss_y; y++)
		for (u32 x=0; x<ss_x; x++)
		{
			u32 dx = px+x;
			u32 dy = py+y;
			u32 sc = src[y*ss_x+x];
			if (color_get_A(sc)>=aREF) dest[dy*ds_x+dx] = sc;
		}
}

void lblit			(lm_layer& dst, lm_layer& src, u32 px, u32 py, u32 aREF)
{
	u32		ds_x	= dst.width;
	u32		ds_y	= dst.height;
	u32		ss_x	= src.width;
	u32		ss_y	= src.height;
	R_ASSERT(ds_x>=(ss_x+px));
	R_ASSERT(ds_y>=(ss_y+py));
	for (u32 y=0; y<ss_y; y++)
		for (u32 x=0; x<ss_x; x++)
		{
			u32 dx = px+x;
			u32 dy = py+y;
			base_color	sc = src.surface[y*ss_x+x];
			u8			sm = src.marker [y*ss_x+x];
			if (sm>=aREF) {
				dst.surface	[dy*ds_x+dx] = sc;
				dst.marker	[dy*ds_x+dx] = sm;
			}
		}
}

void blit			(lm_layer& dst, u32 ds_x, u32 ds_y, lm_layer& src,	u32 ss_x, u32 ss_y, u32 px, u32 py, u32 aREF)
{
	R_ASSERT(ds_x>=(ss_x+px));
	R_ASSERT(ds_y>=(ss_y+py));
	for (u32 y=0; y<ss_y; y++)
		for (u32 x=0; x<ss_x; x++)
		{
			u32 dx = px+x;
			u32 dy = py+y;
			base_color	sc = src.surface[y*ss_x+x];
			u8			sm = src.marker [y*ss_x+x];
			if (sm>=aREF) {
				dst.surface	[dy*ds_x+dx] = sc;
				dst.marker	[dy*ds_x+dx] = sm;
			}
		}
}

void blit_r	(u32* dest, u32 ds_x, u32 ds_y, u32* src, u32 ss_x, u32 ss_y, u32 px, u32 py, u32 aREF)
{
	R_ASSERT(ds_x>=(ss_y+px));
	R_ASSERT(ds_y>=(ss_x+py));
	for (u32 y=0; y<ss_y; y++)
		for (u32 x=0; x<ss_x; x++)
		{
			u32 dx = px+y;
			u32 dy = py+x;
			u32 sc = src[y*ss_x+x];
			if (color_get_A(sc)>=aREF) dest[dy*ds_x+dx] = sc;
		}
}


void blit_r	(lm_layer& dst, u32 ds_x, u32 ds_y, lm_layer& src, u32 ss_x, u32 ss_y, u32 px, u32 py, u32 aREF)
{
	R_ASSERT(ds_x>=(ss_y+px));
	R_ASSERT(ds_y>=(ss_x+py));
	for (u32 y=0; y<ss_y; y++)
		for (u32 x=0; x<ss_x; x++)
		{
			u32 dx = px+y;
			u32 dy = py+x;
			base_color	sc = src.surface[y*ss_x+x];
			u8			sm = src.marker [y*ss_x+x];
			if (sm>=aREF) {
				dst.surface	[dy*ds_x+dx] = sc;
				dst.marker	[dy*ds_x+dx] = sm;
			}
		}
}

//-------------------------------------
vecDefl					g_deflectors;
CDeflector*				Deflector = 0;

IC BOOL UVpointInside(Fvector2 &P, UVtri &T)
{
	Fvector B;
	return T.isInside(P,B);
}

CDeflector::CDeflector()
{
	Deflector		= this;
	normal.set		(0,1,0);
	Sphere.P.set	(flt_max,flt_max,flt_max);
	Sphere.R		= 0;
	bMerged			= FALSE;
	UVpolys.reserve	(32);
}
CDeflector::~CDeflector()
{
}

void CDeflector::OA_Export()
{
	if (UVpolys.empty()) return;

	// Correct normal
	//  (semi-proportional to pixel density)
	FPU::m64r		();
	Fvector			tN;
	tN.set			(0,0,0);
	float density	= 0;
	float fcount	= 0;
	for (UVIt it = UVpolys.begin(); it!=UVpolys.end(); it++)
	{
		Face	*F = it->owner;
		Fvector	SN;
		SN.set	(F->N);
		SN.mul	(1+EPS*F->CalcArea());
		tN.add	(SN);

		density	+= F->Shader().lm_density;
		fcount	+= 1.f;
	}
	if (tN.magnitude()>EPS_S && _valid(tN))	normal.set(tN).normalize();
	else									clMsg("* ERROR: Internal precision error in CDeflector::OA_Export");
	density			/= fcount;
	
	// Orbitrary Oriented Ortho - Projection
	Fmatrix		mView;
    Fvector		at,from,up,right,y;
	at.set		(0,0,0);
	from.add	(at,normal);
	y.set		(0,1,0);
	if (_abs(normal.y)>.99f)		y.set(1,0,0);
	right.crossproduct(y,normal);	right.normalize_safe();
	up.crossproduct(normal,right);	up.normalize_safe();
	mView.build_camera(from,at,up);

	Fbox bb; bb.invalidate();
	for (UVIt it = UVpolys.begin(); it!=UVpolys.end(); it++)
	{
		UVtri	*T = &*it;
		Face	*F = T->owner;
		Fvector	P;	// projected

		for (int i=0; i<3; i++) {
			mView.transform_tiny	(P,F->v[i]->P);
			T->uv[i].set			(P.x,P.y);
			bb.modify				(F->v[i]->P);
		}
	}
	bb.getsphere(Sphere.P,Sphere.R);

	// UV rect
	Fvector2		min,max,size;
	GetRect			(min,max);
	size.sub		(max,min);

	// Surface
	u32 dwWidth		= iCeil(size.x*g_params.m_lm_pixels_per_meter*density+.5f); clamp(dwWidth, 1u,512u-2*BORDER);
	u32 dwHeight	= iCeil(size.y*g_params.m_lm_pixels_per_meter*density+.5f); clamp(dwHeight,1u,512u-2*BORDER);
	layer.create	(dwWidth,dwHeight);
}

BOOL CDeflector::OA_Place	(Face *owner)
{
	// It is not correct to rely solely on normal-split-angle for lmaps - imagine smooth sphere
	float cosa = normal.dotproduct(owner->N);
	if (cosa<_cos(deg2rad(g_params.m_sm_angle+1))) return FALSE;

	UVtri				T;
	T.owner				= owner;
	owner->pDeflector	= this;
	UVpolys.push_back	(T);
	return TRUE;
}

void CDeflector::OA_Place	(vecFace& lst)
{
	UVpolys.clear	();
	UVpolys.reserve	(lst.size());
	for (u32 I=0; I<lst.size(); I++)
	{
		UVtri T;
		Face* F			= lst[I];
		T.owner			= F;
		F->pDeflector	= this;
		UVpolys.push_back(T);
	}
}

void CDeflector::GetRect	(Fvector2 &min, Fvector2 &max)
{
	// Calculate bounds
	xr_vector<UVtri>::iterator it=UVpolys.begin();
	min = max = it->uv[0];
	for (;it != UVpolys.end(); it++)
	{
		for (int i=0; i<3; i++) {
			min.min(it->uv[i]);
			max.max(it->uv[i]);
		}
	}
}

void CDeflector::RemapUV	(xr_vector<UVtri>& dest, u32 base_u, u32 base_v, u32 size_u, u32 size_v, u32 lm_u, u32 lm_v, BOOL bRotate)
{
	dest.clear	();
	dest.reserve(UVpolys.size());
	
	// UV rect (actual)
	Fvector2		a_min,a_max,a_size;
	GetRect		(a_min,a_max);
	a_size.sub	(a_max,a_min);
	
	// UV rect (dedicated)
	Fvector2		d_min,d_max,d_size;
	d_min.x		= (float(base_u)+.5f)/float(lm_u);
	d_min.y		= (float(base_v)+.5f)/float(lm_v);
	d_max.x		= (float(base_u+size_u)-.5f)/float(lm_u);
	d_max.y		= (float(base_v+size_v)-.5f)/float(lm_v);
	if (d_min.x>=d_max.x)	{ d_min.x=d_max.x=(d_min.x+d_max.x)/2; d_min.x-=EPS_S; d_max.x+=EPS_S; }
	if (d_min.y>=d_max.y)	{ d_min.y=d_max.y=(d_min.y+d_max.y)/2; d_min.y-=EPS_S; d_max.y+=EPS_S; }
	d_size.sub	(d_max,d_min);
	
	// Remapping
	Fvector2		tc;
	UVtri		tnew;
	if (bRotate)	{
		for (UVIt it = UVpolys.begin(); it!=UVpolys.end(); it++)
		{
			UVtri&	T	= *it;
			tnew.owner	= T.owner;
			for (int i=0; i<3; i++) 
			{
				tc.x = ((T.uv[i].y-a_min.y)/a_size.y)*d_size.x + d_min.x;
				tc.y = ((T.uv[i].x-a_min.x)/a_size.x)*d_size.y + d_min.y;
				tnew.uv[i].set(tc);
			}
			dest.push_back	(tnew);
		}
	} else {
		for (UVIt it = UVpolys.begin(); it!=UVpolys.end(); it++)
		{
			UVtri&	T	= *it;
			tnew.owner	= T.owner;
			for (int i=0; i<3; i++) 
			{
				tc.x = ((T.uv[i].x-a_min.x)/a_size.x)*d_size.x + d_min.x;
				tc.y = ((T.uv[i].y-a_min.y)/a_size.y)*d_size.y + d_min.y;
				tnew.uv[i].set(tc);
			}
			dest.push_back	(tnew);
		}
	}
}

void CDeflector::RemapUV(u32 base_u, u32 base_v, u32 size_u, u32 size_v, u32 lm_u, u32 lm_v, BOOL bRotate)
{
	xr_vector<UVtri>	tris_new;
	RemapUV			(tris_new,base_u,base_v,size_u,size_v,lm_u,lm_v,bRotate);
	UVpolys			= tris_new;
}
