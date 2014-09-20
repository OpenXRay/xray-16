#include "stdafx.h"

#include "xrlight_implicitcalcglobs.h"
#include "xrLight_implicitdeflector.h"
#include "xrface.h"
#include "xrlc_globaldata.h"
void	ImplicitCalcGlobs::read				( INetReader	&r )
	{

		
		//bool create_rf = !read_faces;
		//if(!read_faces)
		//{
		//	inlc_global_data()->create_read_faces();
		//}

		R_ASSERT(read_faces);
		NetClear	( );
		Allocate( );
		defl	= xr_new<ImplicitDeflector>();
		ImplicitHash->read( r, *read_faces );
		defl->read( r );

		//if(create_rf)
			//inlc_global_data()->destroy_read_faces();
	}
	void	ImplicitCalcGlobs::NetClear	( )
	{
		Deallocate( );
		xr_delete( defl );
	}
	void	ImplicitCalcGlobs::write				( IWriter	&w ) const 
	{

		R_ASSERT( ImplicitHash );
		R_ASSERT(write_faces);
		ImplicitHash->write( w, *write_faces );
		defl->write( w );
	}
	void ImplicitCalcGlobs::Allocate()
	{
			ImplicitHash	= xr_new<IHASH>	();
	}
	void	ImplicitCalcGlobs::Deallocate()
	{
		xr_delete( ImplicitHash );
	}
	void	ImplicitCalcGlobs::Initialize( ImplicitDeflector &d )
	{
		defl = &d;
		R_ASSERT( defl );
		Fbox2 bounds;
		defl->Bounds_Summary			(bounds);
		Hash().initialize	(bounds,defl->faces.size());
		for (u32 fid=0; fid<defl->faces.size(); fid++)
		{
			Face* F				= defl->faces[fid];
			F->AddChannel		(F->tc[0].uv[0],F->tc[0].uv[1],F->tc[0].uv[2]); // make compatible format with LMAPs
			defl->Bounds			(fid,bounds);
			ImplicitHash->add	(bounds,F);
		}
	}