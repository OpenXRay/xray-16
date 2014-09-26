#include "stdafx.h"

#include "LightThread.h"

#include "cl_intersect.h"
#include "base_lighting.h"
#include "global_calculation_data.h"
#include "../shader_xrlc.h"
enum
{
	LP_DEFAULT			= 0,
	LP_UseFaceDisable	= (1<<0),
	LP_dont_rgb			= (1<<1),
	LP_dont_hemi		= (1<<2),
	LP_dont_sun			= (1<<3),
};

float	color_intensity	(Fcolor& c)
{
	float	ntsc		= c.r * 0.2125f + c.g * 0.7154f + c.b * 0.0721f;
	float	absolute	= c.magnitude_rgb() / 1.7320508075688772935274463415059f;
	return	ntsc*0.5f + absolute*0.5f;
}


class base_color
{
public:
	Fvector					rgb;		// - all static lighting
	float					hemi;		// - hemisphere
	float					sun;		// - sun
	float					_tmp_;		// ???
	base_color()			{ rgb.set(0,0,0); hemi=0; sun=0; _tmp_=0;	}

	void					mul			(float s)									{	rgb.mul(s);	hemi*=s; sun*=s;				};
	void					add			(float s)									{	rgb.add(s);	hemi+=s; sun+=s;				};
	void					add			(base_color& s)								{	rgb.add(s.rgb);	hemi+=s.hemi; sun+=s.sun;	};
	void					scale		(int samples)								{	mul	(1.f/float(samples));					};
	void					max			(base_color& s)								{ 	rgb.max(s.rgb); hemi=_max(hemi,s.hemi); sun=_max(sun,s.sun); };
	void					lerp		(base_color& A, base_color& B, float s)		{ 	rgb.lerp(A.rgb,B.rgb,s); float is=1-s;  hemi=is*A.hemi+s*B.hemi; sun=is*A.sun+s*B.sun; };
};

IC	u8	u8_clr				(float a)	{ s32 _a = iFloor(a*255.f); clamp(_a,0,255); return u8(_a);		};


//-----------------------------------------------------------------------------------------------------------------
const int	LIGHT_Count				=	7;

//-----------------------------------------------------------------
__declspec(thread)		u64			t_start	= 0;
__declspec(thread)		u64			t_time	= 0;
__declspec(thread)		u64			t_count	= 0;



IC bool RayPick(CDB::COLLIDER& DB, Fvector& P, Fvector& D, float r, R_Light& L)
{
	// 1. Check cached polygon
	float _u,_v,range;
	bool res = CDB::TestRayTri(P,D,L.tri,_u,_v,range,true);
	if (res) {
		if (range>0 && range<r) return true;
	}

	// 2. Polygon doesn't pick - real database query
	t_start			= CPU::GetCLK();
	DB.ray_query	( &gl_data.RCAST_Model, P, D,r );
	t_time			+=	CPU::GetCLK()-t_start-CPU::clk_overhead;
	t_count			+=	1;
	
	// 3. Analyze
	if (0==DB.r_count()) {
		return false;
	} else {
		// cache polygon
		CDB::RESULT&	rpinf	= *DB.r_begin();
		CDB::TRI&		T		= gl_data.RCAST_Model.get_tris()[rpinf.id];
		L.tri[0].set	(rpinf.verts[0]);
		L.tri[1].set	(rpinf.verts[1]);
		L.tri[2].set	(rpinf.verts[2]);
		return true;
	}
}

float getLastRP_Scale(CDB::COLLIDER* DB, R_Light& L)//, Face* skip)
{
	u32	tris_count		= DB->r_count();
	float	scale		= 1.f;
	Fvector B;

//	X_TRY 
	{
		for (u32 I=0; I<tris_count; I++)
		{
			CDB::RESULT& rpinf = DB->r_begin()[I];
			// Access to texture
			CDB::TRI& clT								= gl_data.RCAST_Model.get_tris()[rpinf.id];
			b_rc_face& F								= gl_data.g_rc_faces[rpinf.id];
//			if (0==F)									continue;
//			if (skip==F)								continue;

			b_material& M	= gl_data.g_materials				[F.dwMaterial];
			b_texture&	T	= gl_data.g_textures				[M.surfidx];

		
			const Shader_xrLC& SH	= shader( F.dwMaterial, *(gl_data.g_shaders_xrlc), gl_data.g_materials );
//			Shader_xrLCVec&	LIB = 		gl_data.g_shaders_xrlc->Library	();
//			if (M.shader_xrlc>=LIB.size()) return		0;		//. hack - vy gonite rebyata - eto ne hack - eto sledy zamesti - shader_xrlc - index ne togo masiva !!
//			Shader_xrLC& SH	= LIB						[M.shader_xrlc];

			if (!SH.flags.bLIGHT_CastShadow)			continue;

			if (0==T.pSurface)	T.bHasAlpha = FALSE;
			if (!T.bHasAlpha)	{
				// Opaque poly - cache it
				L.tri[0].set	(rpinf.verts[0]);
				L.tri[1].set	(rpinf.verts[1]);
				L.tri[2].set	(rpinf.verts[2]);
				return 0;
			}

			// barycentric coords
			// note: W,U,V order
			B.set	(1.0f - rpinf.u - rpinf.v,rpinf.u,rpinf.v);

			// calc UV
			Fvector2*	cuv = F.t;
			Fvector2	uv;
			uv.x = cuv[0].x*B.x + cuv[1].x*B.y + cuv[2].x*B.z;
			uv.y = cuv[0].y*B.x + cuv[1].y*B.y + cuv[2].y*B.z;

			int U = iFloor(uv.x*float(T.dwWidth) + .5f);
			int V = iFloor(uv.y*float(T.dwHeight)+ .5f);
			U %= T.dwWidth;		if (U<0) U+=T.dwWidth;
			V %= T.dwHeight;	if (V<0) V+=T.dwHeight;

			u32 pixel		= T.pSurface[V*T.dwWidth+U];
			u32 pixel_a		= color_get_A(pixel);
			float opac		= 1.f - float(pixel_a)/255.f;
			scale			*= opac;
		}
	} 
//	X_CATCH
//	{
//		clMsg("* ERROR: getLastRP_Scale");
//	}

	return scale;
}

float rayTrace	(CDB::COLLIDER* DB, R_Light& L, Fvector& P, Fvector& D, float R)//, Face* skip)
{
	R_ASSERT	(DB);

	// 1. Check cached polygon
	float _u,_v,range;
	bool res = CDB::TestRayTri(P,D,L.tri,_u,_v,range,false);
	if (res) {
		if (range>0 && range<R) return 0;
	}

	// 2. Polygon doesn't pick - real database query
	DB->ray_query	(&gl_data.RCAST_Model,P,D,R);

	// 3. Analyze polygons and cache nearest if possible
	if (0==DB->r_count()) {
		return 1;
	} else {
		return getLastRP_Scale(DB,L);//,skip);
	}
	return 0;
}

void LightPoint(CDB::COLLIDER* DB, base_color &C, Fvector &P, Fvector &N, base_lighting& lights, u32 flags)
{
	Fvector		Ldir,Pnew;
	Pnew.mad	(P,N,0.01f);

	if (0==(flags&LP_dont_rgb))
	{
		R_Light	*L	= &*lights.rgb.begin(), *E = &*lights.rgb.end();
		for (;L!=E; L++)
		{
			if (L->type==LT_DIRECT) {
				// Cos
				Ldir.invert	(L->direction);
				float D		= Ldir.dotproduct( N );
				if( D <=0 ) continue;

				// Trace Light
				float scale	=	D*L->energy*rayTrace(DB,*L,Pnew,Ldir,1000.f);
				C.rgb.x		+=	scale * L->diffuse.x; 
				C.rgb.y		+=	scale * L->diffuse.y;
				C.rgb.z		+=	scale * L->diffuse.z;
			} else {
				// Distance
				float sqD	=	P.distance_to_sqr	(L->position);
				if (sqD > L->range2) continue;

				// Dir
				Ldir.sub	(L->position,P);
				Ldir.normalize_safe();
				float D		= Ldir.dotproduct( N );
				if( D <=0 ) continue;

				// Trace Light
				float R		= _sqrt(sqD);
				float scale = D*L->energy*rayTrace(DB,*L,Pnew,Ldir,R);
				float A		= scale / (L->attenuation0 + L->attenuation1*R + L->attenuation2*sqD);

				C.rgb.x += A * L->diffuse.x;
				C.rgb.y += A * L->diffuse.y;
				C.rgb.z += A * L->diffuse.z;
			}
		}
	}
	if (0==(flags&LP_dont_sun))
	{
		R_Light	*L	= &*(lights.sun.begin()), *E = &*(lights.sun.end());
		for (;L!=E; L++)
		{
			if (L->type==LT_DIRECT) {
				// Cos
				Ldir.invert	(L->direction);
				float D		= Ldir.dotproduct( N );
				if( D <=0 ) continue;

				// Trace Light
				float scale	=	L->energy*rayTrace(DB,*L,Pnew,Ldir,1000.f);
				C.sun		+=	scale;
			} else {
				// Distance
				float sqD	=	P.distance_to_sqr(L->position);
				if (sqD > L->range2) continue;

				// Dir
				Ldir.sub			(L->position,P);
				Ldir.normalize_safe	();
				float D				= Ldir.dotproduct( N );
				if( D <=0 )			continue;

				// Trace Light
				float R		=	_sqrt(sqD);
				float scale =	D*L->energy*rayTrace(DB,*L,Pnew,Ldir,R);
				float A		=	scale / (L->attenuation0 + L->attenuation1*R + L->attenuation2*sqD);

				C.sun		+=	A;
			}
		}
	}
	if (0==(flags&LP_dont_hemi))
	{
		R_Light	*L	= &*lights.hemi.begin(), *E = &*lights.hemi.end();
		for (;L!=E; L++)
		{
			if (L->type==LT_DIRECT) {
				// Cos
				Ldir.invert	(L->direction);
				float D		= Ldir.dotproduct( N );
				if( D <=0 ) continue;

				// Trace Light
				Fvector		PMoved;	PMoved.mad	(Pnew,Ldir,0.001f);
				float scale	=	L->energy*rayTrace(DB,*L,PMoved,Ldir,1000.f);
				C.hemi		+=	scale;
			} else {
				// Distance
				float sqD	=	P.distance_to_sqr(L->position);
				if (sqD > L->range2) continue;

				// Dir
				Ldir.sub			(L->position,P);
				Ldir.normalize_safe	();
				float D		=	Ldir.dotproduct( N );
				if( D <=0 ) continue;

				// Trace Light
				float R		=	_sqrt(sqD);
				float scale =	D*L->energy*rayTrace(DB,*L,Pnew,Ldir,R);
				float A		=	scale / (L->attenuation0 + L->attenuation1*R + L->attenuation2*sqD);

				C.hemi		+=	A;
			}
		}
	}
}

void	LightThread::	Execute()
	{
//		DetailSlot::verify	();
		CDB::COLLIDER		DB;
		DB.ray_options		( CDB::OPT_CULL	);
		DB.box_options		( CDB::OPT_FULL_TEST );

		base_lighting		Selected;

		for (u32 _z=Nstart; _z<Nend; _z++)
		{
			for (u32 _x=0; _x<gl_data.slots_data.size_x(); _x++)
			{
				DetailSlot&	DS = gl_data.slots_data.get_slot( _x, _z );
				process_pallete( DS );
				if ( gl_data.slots_data.skip_slot ( _x, _z ) )
										continue;
				// Build slot BB & sphere
				Fbox	BB;
				gl_data.slots_data.get_slot_box( BB, _x, _z );

				Fsphere		S;
				BB.getsphere( S.P, S.R );
				
				// Select polygons
				Fvector				bbC,bbD;
				BB.get_CD			( bbC, bbD );	bbD.add( 0.01f );
				DB.box_query		( &gl_data.RCAST_Model, bbC, bbD );

				box_result.clear	();
				for (CDB::RESULT* I=DB.r_begin(); I!=DB.r_end(); I++) box_result.push_back(I->id);
				if (box_result.empty())	continue;

				CDB::TRI*	tris	= gl_data.RCAST_Model.get_tris();
				Fvector*	verts	= gl_data.RCAST_Model.get_verts();
				
				// select lights
				Selected.select		( gl_data.g_lights, S.P, S.R );
				
				// lighting itself
				base_color		amount;
				u32				count	= 0;
				float coeff		= DETAIL_SLOT_SIZE_2/float(LIGHT_Count);
				FPU::m64r		();
				for (int x=-LIGHT_Count; x<=LIGHT_Count; x++) 
				{
					Fvector		P;
					P.x			= bbC.x + coeff*float(x);

					for (int z=-LIGHT_Count; z<=LIGHT_Count; z++) 
					{
						// compute position
						Fvector t_n;	t_n.set(0,1,0);
						P.z				= bbC.z + coeff*float(z);
						P.y				= BB.min.y-5;
						Fvector	dir;	dir.set		(0,-1,0);
						Fvector start;	start.set	(P.x,BB.max.y+EPS,P.z);
						
						float		r_u,r_v,r_range;
						for (DWORDIt tit=box_result.begin(); tit!=box_result.end(); tit++)
						{
							CDB::TRI&	T		= tris	[*tit];
							Fvector		V[3]	= { verts[T.verts[0]], verts[T.verts[1]], verts[T.verts[2]] };
							if (CDB::TestRayTri(start,dir,V,r_u,r_v,r_range,TRUE))
							{
								if (r_range>=0.f)	{
									float y_test	= start.y - r_range;
									if (y_test>P.y)	{
										P.y			= y_test+EPS;
										t_n.mknormal(V[0],V[1],V[2]);
									}
								}
							}
						}
						if (P.y<BB.min.y) continue;
						
						// light point
						LightPoint		(&DB,amount,P,t_n,Selected,0);
						count			+= 1;
					}
				}
				
				// calculation of luminocity
				amount.scale		(count);
				amount.mul			(.5f);
				DS.c_dir			= DS.w_qclr	(amount.sun,15);
				DS.c_hemi			= DS.w_qclr	(amount.hemi,15);
				DS.c_r				= DS.w_qclr	(amount.rgb.x,15);
				DS.c_g				= DS.w_qclr	(amount.rgb.y,15);
				DS.c_b				= DS.w_qclr	(amount.rgb.z,15);

				gl_data.slots_data.set_slot_calculated( _x, _z );

				thProgress			= float(_z-Nstart)/float(Nend-Nstart);
				thPerformance		= float(double(t_count)/double(t_time*CPU::clk_to_seconds))/1000.f;
			}
		}
	}