#include "stdafx.h"

#include "lm_net_global_data.h"

#include "xrlc_globaldata.h"
#include "file_compress.h"
#include "xrdeflector.h"
namespace lc_net
{
	bool cmp_weight( CDeflector* d0, CDeflector* d1)
	{
		R_ASSERT( d0 );
		R_ASSERT( d1 );
		return d0->weight() > d1->weight();
	}
	
	void	net_global_data_impl<gl_lm_data>::create_data_file( LPCSTR path  )
	{
		FPU::m64r			();
		Memory.mem_compact	();
		//std::random_shuffle	(inlc_global_data()->g_deflectors().begin(),inlc_global_data()->g_deflectors().end());
		std::sort( inlc_global_data()->g_deflectors().begin(),inlc_global_data()->g_deflectors().end(),cmp_weight );
		clMsg( "create_lm_data_write:  start" );
		IWriter * file = FS.w_open(path);
		inlc_global_data()->write_lm_data( *file );
		FS.w_close(file);
		compress( path ); 
		clMsg( "create_lm_data_write:  end" );
		//inlc_global_data()->create_read_faces();
		//inlc_global_data()->create_write_faces();
	}
	bool	net_global_data_impl<gl_lm_data>::create_data( LPCSTR path  )
	{
		{
		 R_ASSERT( inlc_global_data() );
		 decompress( path );
		 INetReaderFile r_global( path );
		 inlc_global_data()->read_lm_data( r_global );
		}
	 //unlink( fn );
	//	inlc_global_data()->create_read_faces();
	//	inlc_global_data()->create_write_faces();
		FPU::m64r		();
		Memory.mem_compact	();
		return true;
	}

}

