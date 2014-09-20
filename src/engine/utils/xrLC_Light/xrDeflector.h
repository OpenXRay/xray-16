#pragma once

#include "xrfacedefs.h"
#include "base_color.h"
#include "lm_layer.h"
#include "uv_tri.h"
#include "../../xrcdb/xrCDB.h"
#include "serialize.h"
#include "xrdeflectordefs.h"
#include "execute_statistics.h"
class  base_lighting;
class net_task_callback;
class CDeflector;
//extern XRLC_LIGHT_API CDeflector*		Deflector		;
class execute_statistics;
class XRLC_LIGHT_API CDeflector
{

public:
	net_task_callback			*_net_session;
	xr_vector<UVtri>			UVpolys;
	Fvector						normal;
	lm_layer					layer;
	Fsphere						Sphere;
	
	BOOL						bMerged;
public:

						CDeflector					();
//public:
//						CDeflector					(CDeflector** g_defl)	{ CDeflector(); Deflector = this ;}
						~CDeflector					();
static	CDeflector*		read_create					();	

	void	OA_SetNormal		(Fvector &_N )	{ normal.set(_N); normal.normalize(); VERIFY(_valid(normal)); }
	BOOL	OA_Place			(Face *owner);
	void	OA_Place			(vecFace& lst);
	void	OA_Export			();
		
	void	GetRect				(Fvector2 &min, Fvector2 &max);
	u32		GetFaceCount()		{ return (u32)UVpolys.size();	};
		
	void	Light				(CDB::COLLIDER* DB, base_lighting* LightsSelected, HASH& H	);
	void	L_Direct			(CDB::COLLIDER* DB, base_lighting* LightsSelected, HASH& H  );
	void	L_Direct_Edge		(CDB::COLLIDER* DB, base_lighting* LightsSelected, Fvector2& p1, Fvector2& p2, Fvector& v1, Fvector& v2, Fvector& N, float texel_size, Face* skip);
	void	L_Calculate			(CDB::COLLIDER* DB, base_lighting* LightsSelected, HASH& H  );
	u32		weight				() { return layer.Area(); }	
	u16	GetBaseMaterial		() ;

	void	Bounds				(u32 ID, Fbox2& dest)
	{
		UVtri& TC		= UVpolys[ID];
		dest.min.set	(TC.uv[0]);
		dest.max.set	(TC.uv[0]);
		dest.modify		(TC.uv[1]);
		dest.modify		(TC.uv[2]);
	}
	void	Bounds_Summary		(Fbox2& bounds)
	{
		bounds.invalidate();
		for (u32 I=0; I<UVpolys.size(); I++)
		{
			Fbox2	B;
			Bounds	(I,B);
			bounds.merge(B);
		}
	}
	void	RemapUV				(xr_vector<UVtri>& dest, u32 base_u, u32 base_v, u32 size_u, u32 size_v, u32 lm_u, u32 lm_v, BOOL bRotate);
	void	RemapUV				(u32 base_u, u32 base_v, u32 size_u, u32 size_v, u32 lm_u, u32 lm_v, BOOL bRotate);
	void	read				( INetReader	&r );
	void	write				( IWriter	&w ) const ;
	



	void	receive_result		( INetReader	&r );
	void	send_result			( IWriter	&w ) const ;
	
	bool	similar				( const CDeflector &D, float eps =EPS ) const;
	
#ifdef	COLLECT_EXECUTION_STATS
public:
			execute_time_statistics	time_stat;
	void	statistic_log			(  )const;

#endif
};


typedef xr_vector<UVtri>::iterator UVIt;

extern XRLC_LIGHT_API void		Jitter_Select	(Fvector2* &Jitter, u32& Jcount);
extern void		blit			(u32* dest,		u32 ds_x, u32 ds_y, u32* src,		u32 ss_x, u32 ss_y, u32 px, u32 py, u32 aREF);
extern XRLC_LIGHT_API void		blit			(lm_layer& dst, u32 ds_x, u32 ds_y, lm_layer& src,	u32 ss_x, u32 ss_y, u32 px, u32 py, u32 aREF);
extern void		blit_r			(u32* dest,		u32 ds_x, u32 ds_y, u32* src,		u32 ss_x, u32 ss_y, u32 px, u32 py, u32 aREF);
extern XRLC_LIGHT_API void		blit_r			(lm_layer& dst, u32 ds_x, u32 ds_y, lm_layer& src,	u32 ss_x, u32 ss_y, u32 px, u32 py, u32 aREF);
extern void		lblit			(lm_layer& dst, lm_layer& src, u32 px, u32 py, u32 aREF);
extern XRLC_LIGHT_API void		LightPoint		(CDB::COLLIDER* DB, CDB::MODEL* MDL, base_color_c &C, Fvector &P, Fvector &N, base_lighting& lights, u32 flags, Face* skip);
extern XRLC_LIGHT_API BOOL		ApplyBorders	(lm_layer &lm, u32 ref);
extern XRLC_LIGHT_API void		DumpDeflctor	( u32 id );
extern XRLC_LIGHT_API void		DumpDeflctor	( const CDeflector &d );
extern XRLC_LIGHT_API void		DeflectorsStats ();
extern XRLC_LIGHT_API void		DumpDeflctor	( u32 id );


static const	u32								c_LMAP_size				= 1024;			// pixels

#define rms_zero	((4+g_params().m_lm_rms_zero)/2)
#define rms_shrink	((8+g_params().m_lm_rms)/2)

typedef  vector_serialize< t_read<CDeflector,  get_id_standart<CDeflector> > >		tread_deflectors;
typedef  vector_serialize< t_write<CDeflector, get_id_standart<CDeflector> > >	twrite_deflectors;

extern	tread_deflectors	*read_deflectors	;
extern	twrite_deflectors	*write_deflectors	;


