////////////////////////////////////////////////////////////////////////////
//	Created		: 27.03.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "detail_net_global_data.h"
#include "serialize.h"
#include "file_compress.h"
#include "global_calculation_data.h"
namespace lc_net{



	void	net_global_data_impl<gl_detail_cl_data>::init()		
	{
			data_init( );//init as new data
	}


	void	net_global_data_impl<gl_detail_cl_data>::create_data_file( LPCSTR path  )
	{
		//FPU::m64r			();
		//Memory.mem_compact	();
		//if(!write_faces)
			//inlc_global_data()->create_write_faces();
		clMsg( "gl_detail_cl_data:  start" );
		
		IWriter * file = FS.w_open(path);
		gl_data.write( *file );
		FS.w_close(file);
		compress( path ); 
		clMsg( "gl_detail_cl_data:  end" );

	}
	bool	net_global_data_impl<gl_detail_cl_data>::create_data( LPCSTR path  )
	{
		decompress( path );
		INetReaderFile r_global( path );
	
		gl_data.read(r_global);
		//FPU::m64r			();
		//Memory.mem_compact	();
		return true;
	}
	void	net_global_data_impl<gl_detail_cl_data>::destroy_data( )
	{
		//cl_globs.NetClear();
	}
}

