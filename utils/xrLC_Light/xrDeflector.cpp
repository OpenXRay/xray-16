#include "stdafx.h"
//#include "resource.h"
//#include "build.h"
#include "xrdeflector.h"
#include "xrIsect.h"
#include "xrlc_globaldata.h"

#include "math.h"
#include "xrface.h"
#include "serialize.h"

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

//CDeflector*				Deflector = 0;

IC BOOL UVpointInside(Fvector2 &P, UVtri &T)
{
	Fvector B;
	return T.isInside(P,B);
}

CDeflector::CDeflector(): _net_session(0)
{
	//Deflector		= this;
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
	else
	{
		clMsg("* ERROR: Internal precision error in CDeflector::OA_Export");
		for (UVIt it = UVpolys.begin(); it!=UVpolys.end(); it++)
		{
			Face &fc = *((*it).owner);
			inlc_global_data()->err_tjunction().w_fvector3(fc.v[0]->P);
			inlc_global_data()->err_tjunction().w_fvector3(fc.v[1]->P);

			inlc_global_data()->err_tjunction().w_fvector3(fc.v[1]->P);
			inlc_global_data()->err_tjunction().w_fvector3(fc.v[2]->P);

			inlc_global_data()->err_tjunction().w_fvector3(fc.v[2]->P);
			inlc_global_data()->err_tjunction().w_fvector3(fc.v[0]->P);
		}
	}
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
	VERIFY(inlc_global_data());
	u32 dwWidth		= iCeil(size.x*inlc_global_data()->g_params().m_lm_pixels_per_meter*density+.5f); clamp(dwWidth, 1u,512u-2*BORDER);
	u32 dwHeight	= iCeil(size.y*inlc_global_data()->g_params().m_lm_pixels_per_meter*density+.5f); clamp(dwHeight,1u,512u-2*BORDER);
	layer.create	(dwWidth,dwHeight);
}

BOOL CDeflector::OA_Place	(Face *owner)
{
	// It is not correct to rely solely on normal-split-angle for lmaps - imagine smooth sphere
	float cosa = normal.dotproduct(owner->N);
	VERIFY( inlc_global_data() );
	if (cosa<_cos(deg2rad(inlc_global_data()->g_params().m_sm_angle+1)))
		return FALSE;

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


void CDeflector::L_Calculate(CDB::COLLIDER* DB, base_lighting* LightsSelected, HASH& H)
{
	try {
		lm_layer&		lm	= layer;

		// UV & HASH
		RemapUV			(0,0,lm.width,lm.height,lm.width,lm.height,FALSE);
		Fbox2			bounds;
		Bounds_Summary	(bounds);
		H.initialize	(bounds,(u32)UVpolys.size());
		for (u32 fid=0; fid<UVpolys.size(); fid++)	{
			UVtri* T	= &(UVpolys[fid]);
			Bounds		(fid,bounds);
			H.add		(bounds,T);
		}

		// Calculate
		R_ASSERT		(lm.width	<=(c_LMAP_size-2*BORDER));
		R_ASSERT		(lm.height	<=(c_LMAP_size-2*BORDER));
		lm.create		(lm.width,lm.height);
		L_Direct		(DB,LightsSelected,H);
	} catch (...)
	{
		clMsg("* ERROR: CDeflector::L_Calculate");
	}
}

u16	CDeflector:: GetBaseMaterial		() 
{
	return UVpolys.front().owner->dwMaterial;	
}

	/*
	xr_vector<UVtri>			UVpolys;
	Fvector						normal;
	lm_layer					layer;
	Fsphere						Sphere;
	
	BOOL						bMerged;
	*/

void	CDeflector::receive_result		( INetReader	&r )
{
	read( r );
	layer.read( r );
#ifdef	COLLECT_EXECUTION_STATS
	time_stat.read( r );
#endif
}
void	CDeflector::send_result			( IWriter	&w ) const
{
	write( w );
	layer.write( w );
#ifdef	COLLECT_EXECUTION_STATS
	time_stat.write( w );
#endif
}

void	CDeflector::read				( INetReader	&r )
{
	u32 sz_polys = r.r_u32();
	UVpolys.resize( sz_polys );

	for(u32 i = 0; i < sz_polys; ++i )
	{
		UVpolys[i].read( r );
		VERIFY( UVpolys[i].owner );
		//VERIFY( !UVpolys[i].owner->pDeflector );
		UVpolys[i].owner->pDeflector = this;
	}

	r.r_fvector3( normal );

	//layer.read( r );
	layer.width =	r.r_u32 ();
	layer.height =	r.r_u32 ();

	r_sphere( r, Sphere );

	 bMerged = (BOOL) r.r_u8( );
}


void	CDeflector::write				( IWriter	&w ) const
{
	
	u32 sz_polys = UVpolys.size();
	w.w_u32( sz_polys );
	for(u32 i = 0; i < sz_polys; ++i )
		UVpolys[i].write( w );

	w.w_fvector3( normal );

	//layer.write( w );
	w.w_u32 ( layer.width );
	w.w_u32 ( layer.height );
	
	w_sphere( w, Sphere );

	w.w_u8( (u8) bMerged );
}



bool	CDeflector::similar					( const CDeflector &D, float eps/* =EPS */ ) const
{
	if( bMerged != D.bMerged )
		return false;
	
	if( !normal.similar( D.normal, eps ) )
		return false;

	if( !Sphere.P.similar( D.Sphere.P, eps ) )
		return false;

	if( !fsimilar( Sphere.R, D.Sphere.R, eps ) )
		return false;

	if( UVpolys.size() != D.UVpolys.size() )
		return false;

	for( u32 i = 0; i < UVpolys.size(); ++i )
	{
		if( !UVpolys[i].similar( D.UVpolys[i], eps ) )
		{
			return false;
		}
	}

	return 
		layer.similar( D.layer, eps );
}


CDeflector*		CDeflector::read_create					()
{
	return xr_new<CDeflector>();
}

void DumpDeflctor( u32 id )
{
	VERIFY( inlc_global_data()->g_deflectors().size()>id );
	const CDeflector &D = *inlc_global_data()->g_deflectors()[id];
	clMsg( "deflector id: %d - faces num: %d ", id, D.UVpolys.size() );
	

}

void DumpDeflctor( const CDeflector &D )
{
	clMsg( "lightmap size: %d ", D.layer.width * D.layer.height );
	clMsg( "lightmap width/height : %d/%d", D.layer.width, D.layer.height  );
	clMsg( "deflector - faces num: %d ", D.UVpolys.size() );
}

void DeflectorsStats ()
{
	u32 size =  inlc_global_data()->g_deflectors().size();
	clMsg( "num deflectors: %d", size);
	for( u32 i = 0; i <size ; i++ )
			DumpDeflctor( i ); 
}

#ifdef	COLLECT_EXECUTION_STATS

void	CDeflector::statistic_log			(  ) const
{
	time_stat.log();
	DumpDeflctor( *this );
}

#endif