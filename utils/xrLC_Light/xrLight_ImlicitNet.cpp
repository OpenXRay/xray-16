#include "stdafx.h"
#include "xrlight_implicitrun.h"
#include "xrLight_Implicit.h"
#include "xrlight_implicitdeflector.h"

#include "implicit_net_global_data.h"


#include "net_execution_factory.h"
#include "lcnet_task_manager.h"
#include "net_execution_implicit_light.h"
#include "net_exec_pool.h"
#include "mu_model_light.h"	
#include "xrLC_GlobalData.h"
extern ImplicitCalcGlobs cl_globs;
namespace lc_net{
	static void AddImpImplicitNetTask( u32 from, u32 to )
	{
				//tmanager.start		(xr_new<ImplicitThread> (thID,&defl,thID*stride,thID*stride+stride));
				if( from == to )
					return;
				R_ASSERT( from < to );
				ImplicitExecute	exec( from, to );

				tnet_execution_base< et_implicit_light > *el = lc_net::execution_factory.create<et_implicit_light>();
				el->implementation( ).construct(exec);
				get_task_manager().add_task( el );
	}
	void RunImplicitnet(ImplicitDeflector& defl,  const xr_vector<u32> &exept )
	{
			
			globals().get<gl_implicit_cl_data>().init();

			
	
			
			//WaitNetCompileDataPrepare( );
			WaitMuModelsLocalCalcLightening();
			inlc_global_data()->clear_build_textures_surface(exept);
			

			//u32	num_tasks			= 1003;
			//u32	stride				= defl.Height()/num_tasks;
			//if( stride == 0 )
			//{
			//	num_tasks	= defl.Height();
			//	stride		= 1;
			//}

			const u32	num_tasks	= defl.Height();
			const u32	stride		= 1;
			for (u32 thID=0; thID<num_tasks; thID++)
			{
				AddImpImplicitNetTask(thID*stride,thID*stride+stride);
			}
			AddImpImplicitNetTask(num_tasks*stride,defl.Height());
			
			exec_pool *pool = get_task_manager().run( "Net Implicit Lighting" );
			if(pool)
				pool->wait();
			
			//get_task_manager().wait_all();
			globals().get<gl_implicit_cl_data>().cleanup( );

	}
}