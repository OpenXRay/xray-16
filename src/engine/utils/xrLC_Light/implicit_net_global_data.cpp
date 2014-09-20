#include "stdafx.h"
#include "implicit_net_global_data.h"
#include "xrlight_implicitcalcglobs.h"
#include "xrLC_GlobalData.h"
#include "xrface.h"
#include "file_compress.h"
extern ImplicitCalcGlobs cl_globs;
namespace lc_net{



	void	net_global_data_impl<gl_implicit_cl_data>::init()		
	{
			data_init( );//init as new data
	}


	void	net_global_data_impl<gl_implicit_cl_data>::create_data_file( LPCSTR path  )
	{
		//FPU::m64r			();
		//Memory.mem_compact	();
		//if(!write_faces)
			//inlc_global_data()->create_write_faces();
		clMsg( "create_implicit_data_write:  start" );
		R_ASSERT(write_faces);

		
		IWriter * file = FS.w_open(path);
		cl_globs.write( *file );
		FS.w_close(file);
		
		compress( path ); 
		clMsg( "create_implicit_data_write:  end" );

	}
	bool	net_global_data_impl<gl_implicit_cl_data>::create_data( LPCSTR path  )
	{
		decompress( path );
		INetReaderFile r_global( path );
	
		cl_globs.read(r_global);
		FPU::m64r			();
		Memory.mem_compact	();
		return true;
	}
	void	net_global_data_impl<gl_implicit_cl_data>::	destroy_data( )
	{
		cl_globs.NetClear();
	}
}