#include "stdafx.h"
#include "xr_area.h"
#include "xr_collide_form.h"
#include "xr_object.h"
#include "cl_intersect.h"

#include "igame_level.h"
#include "x_ray.h"
#include "GameFont.h"

using namespace	collide;

//--------------------------------------------------------------------------------
// RayTest - Occluded/No
//--------------------------------------------------------------------------------
BOOL CObjectSpace::RayTest	( const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::ray_cache* cache, CObject* ignore_object)
{
	Lock.Enter		();
	BOOL	_ret	= _RayTest(start,dir,range,tgt,cache,ignore_object);
	r_spatial.clear	();
	Lock.Leave		();
	return			_ret;
}
BOOL CObjectSpace::_RayTest	( const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::ray_cache* cache, CObject* ignore_object)
{
	VERIFY					(_abs(dir.magnitude()-1)<EPS);
	r_temp.r_clear			();

	xrc.ray_options			(CDB::OPT_ONLYFIRST);
	collide::ray_defs	Q	(start,dir,range,CDB::OPT_ONLYFIRST,tgt);

	// dynamic test
	if (tgt&rqtDyn){
		u32			d_flags =	STYPE_COLLIDEABLE|((tgt&rqtObstacle)?STYPE_OBSTACLE:0)|((tgt&rqtShape)?STYPE_SHAPE:0);
		// traverse object database
		g_SpatialSpace->q_ray	(r_spatial,0,d_flags,start,dir,range);
		// Determine visibility for dynamic part of scene
		for (u32 o_it=0; o_it<r_spatial.size(); o_it++)
		{
			ISpatial*	spatial			= r_spatial[o_it];
			CObject*	collidable		= spatial->dcast_CObject	();
			if (collidable && (collidable!=ignore_object))	{
				ECollisionFormType tp	= collidable->collidable.model->Type();
				if ((tgt&(rqtObject|rqtObstacle))&&(tp==cftObject)&&collidable->collidable.model->_RayQuery(Q,r_temp))	return TRUE;
				if ((tgt&rqtShape)&&(tp==cftShape)&&collidable->collidable.model->_RayQuery(Q,r_temp))		return TRUE;
			}
		}
	}
	// static test
	if (tgt&rqtStatic){
		// If we get here - test static model
		if (cache) 
		{
			// 0. similar query???
			if (cache->similar(start,dir,range))	{
				return cache->result;
			}

			// 1. Check cached polygon
			float _u,_v,_range;
			if (CDB::TestRayTri(start,dir,cache->verts,_u,_v,_range,false)) 
			{
				if (_range>0 && _range<range) return TRUE;
			}
			
			// 2. Polygon doesn't pick - real database query
			xrc.ray_query	(&Static,start,dir,range);
			if (0==xrc.r_count()) {
				cache->set		(start,dir,range,FALSE);
				return FALSE;
			} else {
				// cache polygon
				cache->set		(start,dir,range,TRUE);
				CDB::RESULT*	R	= xrc.r_begin();
				CDB::TRI&		T	= Static.get_tris() [ R->id ];
				Fvector*		V	= Static.get_verts();
				cache->verts[0].set	(V[T.verts[0]]);
				cache->verts[1].set	(V[T.verts[1]]);
				cache->verts[2].set	(V[T.verts[2]]);
				return TRUE;
			}
		} else {
			xrc.ray_query		(&Static,start,dir,range);
			return xrc.r_count	();
		}
	}
	return FALSE;
}

//--------------------------------------------------------------------------------
// RayPick
//--------------------------------------------------------------------------------
BOOL CObjectSpace::RayPick	( const Fvector &start, const Fvector &dir, float range, rq_target tgt, rq_result& R, CObject* ignore_object)
{
	Lock.Enter		();
	BOOL	_res	= _RayPick(start,dir,range,tgt,R,ignore_object);
	r_spatial.clear	();
	Lock.Leave		();
	return	_res;
}
BOOL CObjectSpace::_RayPick	( const Fvector &start, const Fvector &dir, float range, rq_target tgt, rq_result& R, CObject* ignore_object)
{
	r_temp.r_clear			();
	R.O		= 0; R.range = range; R.element = -1;
	// static test
	if (tgt&rqtStatic){ 
		xrc.ray_options		(CDB::OPT_ONLYNEAREST | CDB::OPT_CULL);
		xrc.ray_query		(&Static,start,dir,range);
		if (xrc.r_count())  R.set_if_less(xrc.r_begin());
	}
	// dynamic test
	if (tgt&rqtDyn){ 
		collide::ray_defs Q		(start,dir,R.range,CDB::OPT_ONLYNEAREST|CDB::OPT_CULL,tgt);
		// traverse object database
		u32			d_flags =	STYPE_COLLIDEABLE|((tgt&rqtObstacle)?STYPE_OBSTACLE:0)|((tgt&rqtShape)?STYPE_SHAPE:0);
		g_SpatialSpace->q_ray	(r_spatial,0,d_flags,start,dir,range);
		// Determine visibility for dynamic part of scene
		for (u32 o_it=0; o_it<r_spatial.size(); o_it++){
			ISpatial*	spatial			= r_spatial[o_it];
			CObject*	collidable		= spatial->dcast_CObject();
			if			(0==collidable)				continue;
			if			(collidable==ignore_object)	continue;
			ECollisionFormType tp		= collidable->collidable.model->Type();
			if (((tgt&(rqtObject|rqtObstacle))&&(tp==cftObject))||((tgt&rqtShape)&&(tp==cftShape))){
				u32		C	= D3DCOLOR_XRGB	(64,64,64);
				Q.range		= R.range;
				if (collidable->collidable.model->_RayQuery(Q,r_temp)){
					C				= D3DCOLOR_XRGB(128,128,196);
					R.set_if_less	(r_temp.r_begin());
				}
#ifdef DEBUG
				if (bDebug){
					Fsphere	S;		S.P = spatial->spatial.sphere.P; S.R = spatial->spatial.sphere.R;
					m_pRender->dbgAddSphere(S,C);
					//dbg_S.push_back	(mk_pair(S,C));
				}
#endif
			}
		}
	}
	return (R.element>=0);
}

//--------------------------------------------------------------------------------
// RayQuery
//--------------------------------------------------------------------------------
BOOL CObjectSpace::RayQuery		(collide::rq_results& dest, const collide::ray_defs& R, collide::rq_callback* CB, LPVOID user_data, collide::test_callback* tb, CObject* ignore_object)
{
	Lock.Enter					();
	BOOL						_res = _RayQuery2(dest,R,CB,user_data,tb,ignore_object);
	r_spatial.clear_not_free	();
	Lock.Leave					();
	return						(_res);
}
BOOL CObjectSpace::_RayQuery2	(collide::rq_results& r_dest, const collide::ray_defs& R, collide::rq_callback* CB, LPVOID user_data, collide::test_callback* tb, CObject* ignore_object)
{
	// initialize query
	r_dest.r_clear		();
	r_temp.r_clear		();

	rq_target	s_mask	=	rqtStatic;
	rq_target	d_mask	=	rq_target(	((R.tgt&rqtObject)	?rqtObject:rqtNone		)|
										((R.tgt&rqtObstacle)?rqtObstacle:rqtNone	)|
										((R.tgt&rqtShape)	?rqtShape:rqtNone)		);
	u32			d_flags =	STYPE_COLLIDEABLE|((R.tgt&rqtObstacle)?STYPE_OBSTACLE:0)|((R.tgt&rqtShape)?STYPE_SHAPE:0);

	// Test static
	if (R.tgt&s_mask){ 
		xrc.ray_options	(R.flags);
		xrc.ray_query	(&Static,R.start,R.dir,R.range);
		if (xrc.r_count()){	
			CDB::RESULT* _I	= xrc.r_begin();
			CDB::RESULT* _E = xrc.r_end	();
			for (; _I!=_E; _I++)
				r_temp.append_result(rq_result().set(0,_I->range,_I->id));
		}
	}
	// Test dynamic
	if (R.tgt&d_mask){ 
		// Traverse object database
		g_SpatialSpace->q_ray	(r_spatial,0,d_flags,R.start,R.dir,R.range);
		for (u32 o_it=0; o_it<r_spatial.size(); o_it++){
			CObject*	collidable		= r_spatial[o_it]->dcast_CObject();
			if			(0==collidable)				continue;
			if			(collidable==ignore_object)	continue;
			ICollisionForm*	cform		= collidable->collidable.model;
			ECollisionFormType tp		= collidable->collidable.model->Type();
			if (((R.tgt&(rqtObject|rqtObstacle))&&(tp==cftObject))||((R.tgt&rqtShape)&&(tp==cftShape))){
				if (tb&&!tb(R,collidable,user_data))continue;
				cform->_RayQuery(R,r_temp);
			}
		}
	}
	if (r_temp.r_count()){
		r_temp.r_sort		();
		collide::rq_result* _I = r_temp.r_begin	();
		collide::rq_result* _E = r_temp.r_end	();
		for (; _I!=_E; _I++){
			r_dest.append_result(*_I);
			if (!(CB?CB(*_I,user_data):TRUE))						return r_dest.r_count();
			if (R.flags&(CDB::OPT_ONLYNEAREST|CDB::OPT_ONLYFIRST))	return r_dest.r_count();
		}
	}
	return r_dest.r_count();
}

BOOL CObjectSpace::_RayQuery3	(collide::rq_results& r_dest, const collide::ray_defs& R, collide::rq_callback* CB, LPVOID user_data, collide::test_callback* tb, CObject* ignore_object)
{
	// initialize query
	r_dest.r_clear			();

	ray_defs	d_rd		(R);
	ray_defs	s_rd		(R.start,R.dir,R.range,CDB::OPT_ONLYNEAREST|R.flags,R.tgt);
	rq_target	s_mask		=	rqtStatic;
	rq_target	d_mask		=	rq_target(	((R.tgt&rqtObject)	?rqtObject:rqtNone		)|
											((R.tgt&rqtObstacle)?rqtObstacle:rqtNone	)|
											((R.tgt&rqtShape)	?rqtShape:rqtNone)		);
	u32			d_flags		=	STYPE_COLLIDEABLE|((R.tgt&rqtObstacle)?STYPE_OBSTACLE:0)|((R.tgt&rqtShape)?STYPE_SHAPE:0);
	float		d_range		= 0.f;

	do{
		r_temp.r_clear		();
		if (R.tgt&s_mask){
			// static test allowed

			// test static
			xrc.ray_options		(s_rd.flags);
			xrc.ray_query		(&Static,s_rd.start,s_rd.dir,s_rd.range);

			if (xrc.r_count())	{	
				VERIFY			(xrc.r_count()==1);
				rq_result		s_res;
				s_res.set		(0,xrc.r_begin()->range,xrc.r_begin()->id);
				// update dynamic test range
				d_rd.range		= s_res.range;
				// set next static start & range
				s_rd.range		-= (s_res.range+EPS_L);
				s_rd.start.mad	(s_rd.dir,s_res.range+EPS_L);
				s_res.range		= R.range-s_rd.range-EPS_L;
				r_temp.append_result(s_res);
			}else{
				d_rd.range		= s_rd.range;
			}
		}
		// test dynamic
		if (R.tgt&d_mask)		{ 
			// Traverse object database
			g_SpatialSpace->q_ray	(r_spatial,0,d_flags,d_rd.start,d_rd.dir,d_rd.range);
			for (u32 o_it=0; o_it<r_spatial.size(); o_it++){
				CObject*	collidable		= r_spatial[o_it]->dcast_CObject();
				if			(0==collidable)				continue;
				if			(collidable==ignore_object)	continue;
				ICollisionForm*	cform		= collidable->collidable.model;
				ECollisionFormType tp		= collidable->collidable.model->Type();
				if (((R.tgt&(rqtObject|rqtObstacle))&&(tp==cftObject))||((R.tgt&rqtShape)&&(tp==cftShape))){
					if (tb&&!tb(d_rd,collidable,user_data))continue;
					u32 r_cnt				= r_temp.r_count();
					cform->_RayQuery		(d_rd,r_temp);
					for (int k=r_cnt; k<r_temp.r_count(); k++){
						rq_result& d_res	= *(r_temp.r_begin()+k);
						d_res.range			+= d_range;
					}
				}
			}
		}
		// set dynamic ray def
		d_rd.start			= s_rd.start;
		d_range				= R.range-s_rd.range;
		if (r_temp.r_count()){
			r_temp.r_sort		();
			collide::rq_result* _I = r_temp.r_begin	();
			collide::rq_result* _E = r_temp.r_end	();
			for (; _I!=_E; _I++){
				r_dest.append_result(*_I);
				if (!(CB?CB(*_I,user_data):TRUE))	return r_dest.r_count();
				if (R.flags&CDB::OPT_ONLYFIRST)		return r_dest.r_count();
			}
		}
		if ((R.flags&(CDB::OPT_ONLYNEAREST|CDB::OPT_ONLYFIRST)) && r_dest.r_count()) return r_dest.r_count();
	}while(r_temp.r_count());
	return r_dest.r_count()	;
}

BOOL CObjectSpace::_RayQuery	(collide::rq_results& r_dest, const collide::ray_defs& R, collide::rq_callback* CB, LPVOID user_data, collide::test_callback* tb, CObject* ignore_object)
{
#ifdef DEBUG
	if (R.range<EPS || !_valid(R.range))
		Debug.fatal			(DEBUG_INFO,"Invalid RayQuery range passed: %f.",R.range);
#endif
	// initialize query
	r_dest.r_clear			();
	r_temp.r_clear			();

	Flags32		sd_test;	sd_test.assign	(R.tgt);
	rq_target	next_test	= R.tgt;

	rq_result	s_res;
	ray_defs	s_rd		(R.start,R.dir,R.range,CDB::OPT_ONLYNEAREST|R.flags,R.tgt);
	ray_defs	d_rd		(R.start,R.dir,R.range,CDB::OPT_ONLYNEAREST|R.flags,R.tgt);
	rq_target	s_mask	=	rqtStatic;
	rq_target	d_mask	=	rq_target(	((R.tgt&rqtObject)	?rqtObject:rqtNone		)|
										((R.tgt&rqtObstacle)?rqtObstacle:rqtNone	)|
										((R.tgt&rqtShape)	?rqtShape:rqtNone)		);
	u32			d_flags =	STYPE_COLLIDEABLE|((R.tgt&rqtObstacle)?STYPE_OBSTACLE:0)|((R.tgt&rqtShape)?STYPE_SHAPE:0);

	s_res.set				(0,s_rd.range,-1);
	do{
		if ((R.tgt&s_mask)&&sd_test.is(s_mask)&&(next_test&s_mask)){ 
			s_res.set		(0,s_rd.range,-1);
			// Test static model
			if (s_rd.range>EPS){
				xrc.ray_options	(s_rd.flags);
				xrc.ray_query	(&Static,s_rd.start,s_rd.dir,s_rd.range);
				if (xrc.r_count()){	
					if (s_res.set_if_less(xrc.r_begin())){
						// set new static start & range
						s_rd.range	-=	(s_res.range+EPS_L);
						s_rd.start.mad	(s_rd.dir,s_res.range+EPS_L);
						s_res.range	= R.range-s_rd.range-EPS_L;
#ifdef DEBUG
						if (!(fis_zero(s_res.range,EPS) || s_res.range>=0.f))
							Debug.fatal(DEBUG_INFO,"Invalid RayQuery static range: %f (%f). /#1/",s_res.range,s_rd.range);
#endif
					}
				}
			}
			if (!s_res.valid())	sd_test.set(s_mask,FALSE);
		}
		if ((R.tgt&d_mask)&&sd_test.is_any(d_mask)&&(next_test&d_mask)){ 
			r_temp.r_clear	();

			if (d_rd.range>EPS){
				// Traverse object database
				g_SpatialSpace->q_ray		(r_spatial,0,d_flags,d_rd.start,d_rd.dir,d_rd.range);
				// Determine visibility for dynamic part of scene
				for (u32 o_it=0; o_it<r_spatial.size(); o_it++){
					CObject*	collidable		= r_spatial[o_it]->dcast_CObject();
					if			(0==collidable)				continue;
					if			(collidable==ignore_object)	continue;
					ICollisionForm*	cform		= collidable->collidable.model;
					ECollisionFormType tp		= collidable->collidable.model->Type();
					if (((R.tgt&(rqtObject|rqtObstacle))&&(tp==cftObject))||((R.tgt&rqtShape)&&(tp==cftShape))){
						if (tb&&!tb(d_rd,collidable,user_data))continue;
						cform->_RayQuery(d_rd,r_temp);
					}
#ifdef DEBUG
					if (!((0==r_temp.r_count()) || (r_temp.r_count()&&(fis_zero(r_temp.r_begin()->range, EPS)||(r_temp.r_begin()->range>=0.f)))))
						Debug.fatal(DEBUG_INFO,"Invalid RayQuery dynamic range: %f (%f). /#2/",r_temp.r_begin()->range,d_rd.range);
#endif
				}
			}
			if (r_temp.r_count()){
				// set new dynamic start & range
				rq_result& d_res = *r_temp.r_begin();
				d_rd.range	-= (d_res.range+EPS_L);
				d_rd.start.mad(d_rd.dir,d_res.range+EPS_L);
				d_res.range	= R.range-d_rd.range-EPS_L;
#ifdef DEBUG
				if (!(fis_zero(d_res.range,EPS) || d_res.range>=0.f))
					Debug.fatal(DEBUG_INFO,"Invalid RayQuery dynamic range: %f (%f). /#3/",d_res.range,d_rd.range);
#endif
			}else{
				sd_test.set(d_mask,FALSE);
			}
		}
		if (s_res.valid()&&r_temp.r_count()){
			// all test return result
			if	(s_res.range<r_temp.r_begin()->range){
				// static nearer
				BOOL need_calc			= CB?CB(s_res,user_data):TRUE;
				next_test				= need_calc?s_mask:rqtNone; 
				r_dest.append_result	(s_res);
			}else{
				// dynamic nearer
				BOOL need_calc			= CB?CB(*r_temp.r_begin(),user_data):TRUE;
				next_test				= need_calc?d_mask:rqtNone;	
				r_dest.append_result	(*r_temp.r_begin());
			}
		}else if (s_res.valid())	{
			// only static return result
			BOOL need_calc				= CB?CB(s_res,user_data):TRUE;
			next_test					= need_calc?s_mask:rqtNone;
			r_dest.append_result		(s_res);
		}else if (r_temp.r_count())	{
			// only dynamic return result
			BOOL need_calc				= CB?CB(*r_temp.r_begin(),user_data):TRUE;
			next_test					= need_calc?d_mask:rqtNone;
			r_dest.append_result		(*r_temp.r_begin());
		}else{
			// nothing selected
			next_test			= rqtNone;
		}
		if ((R.flags&CDB::OPT_ONLYFIRST)||(R.flags&CDB::OPT_ONLYNEAREST)) break;
	} while (next_test!=rqtNone)		;
	return r_dest.r_count	()	;
}

BOOL CObjectSpace::RayQuery	(collide::rq_results& r_dest, ICollisionForm* target, const collide::ray_defs& R)
{
	VERIFY					(target);
	r_dest.r_clear			();
	return target->_RayQuery(R,r_dest);
}
