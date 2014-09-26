#include "stdafx.h"
#include "build.h"
#include "xrHemisphere.h"

// hemi
struct		hemi_data
{
	xr_vector<R_Light>*	dest;
	R_Light				T;
};
void		__stdcall	hemi_callback(float x, float y, float z, float E, LPVOID P)
{
	hemi_data*	H		= (hemi_data*)P;
	H->T.energy			= E;
	H->T.direction.set	(x,y,z);
	H->dest->push_back	(H->T);
}

// 
void CBuild::SoftenLights()
{
	Status	("Jittering lights...");
	b_light*		L = lights_lmaps.begin();
	R_Light			RL;
	CDB::COLLIDER	XRC;
				
	for (;L!=lights_lmaps.end(); L++)
	{
		Progress				(float(L-lights_lmaps.begin())/float(lights_lmaps.size()));

		// generic properties
		RL.diffuse.normalize_rgb	(L->diffuse);
		RL.position.set				(L->position);
		RL.direction.normalize_safe	(L->direction);
		RL.range				=	L->range*1.1f;
		RL.range2				=	RL.range*RL.range;
		RL.attenuation0			=	L->attenuation0;
		RL.attenuation1			=	L->attenuation1;
		RL.attenuation2			=	L->attenuation2;
		RL.falloff				=   1.0f/(RL.range*(RL.attenuation0 + RL.attenuation1*RL.range + RL.attenuation2*RL.range2))
		RL.energy				=	L->diffuse.magnitude_rgb();
		
		// select destination container
		xr_vector<R_Light>* dest	=	0;
		if (L->flags.bProcedural)	{
			// one of the procedural lights
			lights.push_back		( b_LightLayer() );
			lights.back().original	= L;
			dest					= &(lights.back().lights);		
			RL.diffuse.set			(1,1,1,1);
			RL.energy				= RL.diffuse.magnitude_rgb();
			RL.diffuse.normalize_rgb();
		} else {
			// ambient (fully-static)
			dest					= &(lights.front().lights);
		}

		if (L->type==D3DLIGHT_DIRECTIONAL) 
		{
			RL.type				= LT_DIRECT;
			R_Light	T			= RL;
			Fmatrix				rot_y;

			Fvector				v_top,v_right,v_dir;
			v_top.set			(0,1,0);
			v_dir.set			(RL.direction);
			v_right.crossproduct(v_top,v_dir);
			v_right.normalize	();

			// Build jittered light
			T.energy			= RL.energy/14.f;
			float angle			= deg2rad(g_params.area_dispersion);
			{
				// *** center
				dest->push_back	(T);

				// *** left
				rot_y.rotateY			(3*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				rot_y.rotateY			(2*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				rot_y.rotateY			(1*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				// *** right
				rot_y.rotateY			(-1*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				rot_y.rotateY			(-2*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				rot_y.rotateY			(-3*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);
				
				// *** top 
				rot_y.rotation			(v_right, 3*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				rot_y.rotation			(v_right, 2*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				rot_y.rotation			(v_right, 1*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				// *** bottom
				rot_y.rotation			(v_right,-1*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				rot_y.rotation			(v_right,-2*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);

				rot_y.rotation			(v_right,-3*angle/4);
				rot_y.transform_dir		(T.direction,RL.direction);
				dest->push_back	(T);
			}

			// Build area-lights
			if (g_params.area_quality)	
			{
				hemi_data				h_data;
				h_data.dest				= dest;
				h_data.T				= RL;
				h_data.T.diffuse.set	(g_params.area_color);
				xrHemisphereBuild		(g_params.area_quality,FALSE,0.5f,g_params.area_energy_summary,hemi_callback,&h_data);
			}
		} else {
			RL.type			= LT_POINT;
			if (g_params.fuzzy_enable)	{
				// Perform jittering
				R_Light	T		= RL;
				T.energy		= RL.energy/float(g_params.fuzzy_samples);
				
				XRC.ray_options	(CDB::OPT_ONLYNEAREST);
				for (int i=0; i<g_params.fuzzy_samples; i++)
				{
					// dir & dist
					Fvector			R;
					R.random_dir	();
					float dist		= ::Random.randF(g_params.fuzzy_min,g_params.fuzzy_max);
					
					// check collision
					XRC.ray_query	(&RCAST_Model,RL.position,R,dist);
					if (XRC.r_count())	dist = XRC.r_begin()->range;
					
					// calculate point
					T.position.mad	(RL.position,R,dist*.5f);
					dest->push_back	(T);
				}
			} else {
				dest->push_back(RL);
			}
		}
	}
	clMsg("* Total light-layers: %d",lights.size());
}
