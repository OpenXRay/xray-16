#include "stdafx.h"
#include "xrHemisphere.h"
#include "build.h"

#include "../xrlc_light/xrThread.h"
#include "../xrLC_Light/xrLC_GlobalData.h"
#include "../xrLC_Light/xrface.h"

#include "../../xrcore/xrSyncronize.h"
#include "../../xrcdb/xrcdb.h"


#define	GI_THREADS		2
const	u32				gi_num_photons		= 32;
const	float			gi_optimal_range	= 15.f;
const	float			gi_reflect			= 0.9f;
const	float			gi_clip				= 0.05f;
const	u32				gi_maxlevel			= 4;
//////////////////////////////////////////////////////////////////////////
static xr_vector<R_Light>*		task;
xrCriticalSection		task_cs
#ifdef PROFILE_CRITICAL_SECTIONS
	(MUTEX_PROFILE_ID(task_cs))
#endif // PROFILE_CRITICAL_SECTIONS
;
static u32						task_it;

//////////////////////////////////////////////////////////////////////////
static Fvector		GetPixel_7x7		(CDB::RESULT& rpinf)
{
	Fvector B,P,R={0,0,0};

	// Access to texture
	CDB::TRI& clT										= lc_global_data()->RCAST_Model()->get_tris()[rpinf.id];
	base_Face* F										= (base_Face*)(*((void**)&clT.dummy));
	if (0==F)											return R;
	const Shader_xrLC&	SH								= F->Shader();
	if (!SH.flags.bLIGHT_CastShadow)					return R;
	if (!F->flags.bOpaque)								return R;	// don't use transparency

	b_material& M	= pBuild->materials()			[F->dwMaterial];
	b_texture&	T	= pBuild->textures()			[M.surfidx];

#ifdef		DEBUG
	const b_BuildTexture	&build_texture  = pBuild->textures()			[M.surfidx];

	VERIFY( !!(build_texture.THM.HasSurface()) ==  !!(T.pSurface) );
#endif

	if (0==T.pSurface)									
				return R;

	// barycentric coords
	// note: W,U,V order
	B.set	(1.0f - rpinf.u - rpinf.v,rpinf.u,rpinf.v);

	// calc UV
	Fvector2*	cuv = F->getTC0					();
	Fvector2	uv;
	uv.x = cuv[0].x*B.x + cuv[1].x*B.y + cuv[2].x*B.z;
	uv.y = cuv[0].y*B.x + cuv[1].y*B.y + cuv[2].y*B.z;

	for (int _y=-3; _y<=3; _y++)	
	{
		for (int _x=-3; _x<=3; _x++)	
		{
			int U = iFloor(uv.x*float(T.dwWidth) + .5f) + _x;
			int V = iFloor(uv.y*float(T.dwHeight)+ .5f) + _y;
			U %= T.dwWidth;		if (U<0) U+=T.dwWidth;
			V %= T.dwHeight;	if (V<0) V+=T.dwHeight;
			u32 pixel		= T.pSurface[V*T.dwWidth+U];
			P.set(float(color_get_R(pixel)),float(color_get_G(pixel)),float(color_get_B(pixel)));
			R.mad(P,1.f/255.f);
		}
	}
	R.div	(49.f);
	//R.add	(1.f);	// make it appear more like white material
	//R.div	(2.f);
	return	R;
}

//////////////////////////////////////////////////////////////////////////
class CGI		: public CThread
{
public:
	CGI			(u32 ID)	: CThread(ID)	{	thMessages	= FALSE; }

	virtual void	Execute	()
	{
		CDB::COLLIDER		xrc;
		xrc.ray_options		(CDB::OPT_CULL|CDB::OPT_ONLYNEAREST);
		CDB::MODEL*	model	= lc_global_data()->RCAST_Model();
		CDB::TRI*	tris	= lc_global_data()->RCAST_Model()->get_tris();
		Fvector*	verts	= lc_global_data()->RCAST_Model()->get_verts();

		// full iteration
		for (;;)	
		{
			// get task
			R_Light				src,dst;
			task_cs.Enter		();
			if (task_it>=task->size())	{
				task_cs.Leave	();
				return;
			} else {
				src					= (*task)[task_it];
				if (0==src.level)	src.range	*= 1.5f;
				dst					= src;
				//if (LT_POINT==src.type)	(*task)[task_it].energy		= 0.f;
				dst.type			= LT_SECONDARY;
				dst.level			++;
				task_it				++;
				thProgress			= float(task_it)/float(task->size())/float(GI_THREADS);
			}
			task_cs.Leave				();
			if (dst.level>gi_maxlevel)	continue;

			// analyze
			CRandom				random;
			random.seed			(0x12071980);
			float	factor		=  _sqrt(src.range / gi_optimal_range);			// smaller lights get smaller amount of photons
					if (factor>1)	factor=1;
			if (LT_SECONDARY == src.type)	factor /= powf(2.f,float(src.level));// secondary lights get half the photons
					factor		*= _sqrt(src.energy);							// 2.f is optimal energy = baseline
					//factor	= _sqrt (factor);								// move towards 1.0 (one)
			int		count		= iCeil( factor * float(gi_num_photons) );
					//count		= gi_num_photons;
			float	_clip		= (_sqrt(src.energy)/10.f + gi_clip)/2.f;
			float	_scale		= 1.f / _sqrt(factor);
			//clMsg	("src_LER[%d/%f/%f] -> factor(%f), count(%d), clip(%f)",
			//	src.level, src.energy, src.range, factor, count, _clip
			//	);
			for (int it=0; it<count; it++)	{
				Fvector	dir,idir;		float	s=1.f;
				switch	(src.type)		{
					case LT_POINT		:	dir.random_dir(random).normalize();				break;
					case LT_SECONDARY	:	
						dir.random_dir	(src.direction,PI_DIV_2,random);					//. or PI ?
						s				= src.direction.dotproduct(dir.normalize());
						break;
					default:			continue;											// continue loop
				}
				xrc.ray_query		(model,src.position,dir,src.range);
				if					(!xrc.r_count()) continue;
				CDB::RESULT *R		= xrc.r_begin	();
				CDB::TRI&	T		= tris[R->id];
				Fvector		Tv[3]	= { verts[T.verts[0]],verts[T.verts[1]],verts[T.verts[2]] };
				Fvector		TN;		TN.mknormal		(Tv[0],Tv[1],Tv[2]);
				float		dot		= TN.dotproduct	(idir.invert(dir));

				dst.position.mad		(src.position,dir,R->range);
				dst.position.mad		(TN,0.01f);		// 1cm away from surface
				dst.direction.reflect	(dir,TN);
				dst.energy				= src.energy * dot * gi_reflect * (1-R->range/src.range) * _scale;
				if (dst.energy < _clip)	continue;

				// color bleeding
				dst.diffuse.mul			(src.diffuse,GetPixel_7x7(*R));
				dst.diffuse.mul			(dst.energy);
				{
					float			_e		=	(dst.diffuse.x+dst.diffuse.y+dst.diffuse.z)/3.f;
					Fvector			_c		=	{dst.diffuse.x,dst.diffuse.y,dst.diffuse.z};
					if (_abs(_e)>EPS_S)		_c.div	(_e);
					else					{ _c.set(0,0,0); _e=0; }
					dst.diffuse				= _c;
					dst.energy				= _e;
				}
				if (dst.energy < _clip)	continue;

				// scale range in proportion with energy
				float	_r1			= src.range * _sqrt(dst.energy / src.energy);
				float	_r2			= (dst.energy - _clip)/_clip;
				float	_r3			= src.range;
				dst.range			= 1 * ( (1.f*_r1 + 3.f*_r2 + 3.f*_r3)/7.f );	// empirical
				// clMsg			("submit: level[%d],type[%d], energy[%f]",dst.level,dst.type,dst.energy);

				// submit answer
				if (dst.energy > gi_clip/4)	
				{
					//clMsg	("dst_ER[%f/%f]", dst.energy, dst.range);
					task_cs.Enter		();
					task->push_back		(dst);
					task_cs.Leave		();
				}
			}
		}
	}
};

// test_radios
void	CBuild::xrPhase_Radiosity	()
{
	CThreadManager			gi;
	Status					("Working...");
	task					= &(pBuild->L_static().rgb);
	task_it					= 0;

	// calculate energy
	float	_energy_before	= 0;
	for (u32 l=0; l<task->size(); l++)
		if (task->at(l).type == LT_POINT)	_energy_before	+= task->at(l).energy;

	// perform all the work
	u32	setup_old			= task->size	();
	for (int t=0; t<GI_THREADS; t++)	{
		gi.start(xr_new<CGI>(t));
		Sleep	(10);
	}
	gi.wait					();
	u32 setup_new			= task->size	();

	// renormalize
	float	_energy_after	= 0;
	for (u32 l=0; l<task->size(); l++)
	{
		R_Light&	L = (*task)[l];
		//clMsg		("type[%d], energy[%f]",L.type,L.energy);
		if (LT_SECONDARY == L.type)	{
			if (L.energy > gi_clip/4)	_energy_after	+= L.energy;
			else						{ task->erase	(task->begin()+l); l--; }
		}
	}
	float	_scale			= 1.f*_energy_before / _energy_after;
	for (u32 l=0; l<task->size(); l++)
	{
		R_Light&	L = (*task)[l];
		if (LT_SECONDARY == L.type)		L.energy	*= _scale;
	}

	// info
	clMsg				("old setup [%d], new setup[%d]",setup_old,setup_new);
	clMsg				("old energy [%f], new energy[%f]",_energy_before,_energy_after);
	FlushLog			();
}
