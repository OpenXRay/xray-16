////////////////////////////////////////////////////////////////////////////
//	Created		: 19.05.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "gl_base_cl_data.h"


#include "xrlc_globaldata.h"
#include "file_compress.h"


namespace lc_net
{
	void	net_global_data_impl<gl_base_cl_data>::create_data_file( LPCSTR path  )
	{
		FPU::m64r			();
		Memory.mem_compact	();
		//std::random_shuffle	(inlc_global_data()->g_deflectors().begin(),inlc_global_data()->g_deflectors().end());
		clMsg( "create_base_global_data_write:  start" );
		IWriter * file = FS.w_open(path);
		inlc_global_data()->write_base( *file );
		FS.w_close(file);
		compress( path ); 
		clMsg( "create_base_global_data_write:  end" );
		//inlc_global_data()->create_read_faces();
		//inlc_global_data()->create_write_faces();
	}
	bool	net_global_data_impl<gl_base_cl_data>::create_data( LPCSTR path  )
	{
		decompress( path );
		INetReaderFile r_global( path );
		create_global_data();
		
		VERIFY( inlc_global_data() );
		inlc_global_data()->read_base( r_global );
		
		//unlink( fn );

		FPU::m64r		();
		Memory.mem_compact	();
		//inlc_global_data()->create_read_faces();
		//inlc_global_data()->create_write_faces();
		return true;
	}

}



