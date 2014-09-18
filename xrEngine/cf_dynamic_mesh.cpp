#include	"stdafx.h"

#include	"cf_dynamic_mesh.h"

#include	"xr_object.h"

#include "../Include/xrRender/RenderVisual.h"
#include "../Include/xrRender/Kinematics.h"

#ifdef DEBUG
#include	"iphdebug.h"
#endif

BOOL	CCF_DynamicMesh::_RayQuery( const collide::ray_defs& Q, collide::rq_results& R)
{
	int s_count = R.r_count();
	BOOL res = inherited::_RayQuery( Q, R );
	if( !res )
		return FALSE;

	VERIFY( owner );
	VERIFY( owner->Visual() );
	IKinematics *K = owner->Visual()->dcast_PKinematics();
	
	struct spick
	{
		 const collide::ray_defs& Q;
		 const  CObject			& obj;
		 IKinematics			& K;
		 
		 spick(  const collide::ray_defs& Q_, const CObject &obj_, IKinematics &K_ ): Q( Q_ ), obj( obj_ ), K( K_ )
		 {

		 }
		 
		 bool operator() ( collide::rq_result &r )
		{
			IKinematics::pick_result br;
			VERIFY( r.O == &obj );
			bool  res = K.PickBone( obj.XFORM(), br, Q.range, Q.start, Q.dir, (u16) r.element ) ;
			if(res)
			{
				r.range = br.dist;
			}
#if	0
			if(res)
			{
				ph_debug_render->open_cashed_draw();
				ph_debug_render->draw_tri(  br.tri[0], br.tri[1], br.tri[2], D3DCOLOR_ARGB(50, 255,0,0), 0 );
				ph_debug_render->close_cashed_draw(50000);
			}
#endif
			return !res;
		}
		private:
			spick& operator = (spick& ){ NODEFAULT;  return *this; }
	} pick( (collide::ray_defs&) (Q), (const CObject &)(*owner),(IKinematics&) (*K) );
	
	R.r_results().erase( std::remove_if( R.r_results().begin() + s_count, R.r_results().end() , pick), R.r_results().end() );
	/*
	for( collide::rq_result* i = R.r_begin() + s_count; i < R.r_end(); ++i )
	{
		IKinematics::pick_result r;
		if( K->PickBone( owner->XFORM(), r, Q.range, Q.start, Q.dir, (u16) i->element ) )
			return TRUE;
	}
	*/
	VERIFY( R.r_count() >= s_count );
	return R.r_count() > s_count;
}