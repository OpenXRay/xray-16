#include "stdafx.h"
#include "ref_model_net_global_data.h"

#include "xrlc_globaldata.h"
#include "file_compress.h"

namespace lc_net
{
	void	net_global_data_impl<gl_ref_model_data>::create_data_file( LPCSTR path  )
	{
		FPU::m64r			();
		Memory.mem_compact	();
		clMsg( "create_ref_model_data_write:  start" );
		IWriter * file = FS.w_open(path);
		inlc_global_data()->write_mu_model_refs( *file );
		inlc_global_data()->write_modes_color( *file );
		FS.w_close(file);
		compress( path ); 
		clMsg( "create_ref_model_data_write:  end" );
	}
	bool	net_global_data_impl<gl_ref_model_data>::create_data( LPCSTR path  )
	{
		{
		 R_ASSERT( inlc_global_data() );
		 decompress( path );
		 INetReaderFile r_global( path );
		 inlc_global_data()->read_mu_model_refs( r_global );
		 inlc_global_data()->read_modes_color( r_global );
		}
	 //unlink( fn );
		FPU::m64r		();
		Memory.mem_compact	();
		return true;
	}

}


