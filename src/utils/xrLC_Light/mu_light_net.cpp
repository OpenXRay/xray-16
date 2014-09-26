#include "stdafx.h"

//#include "xrface.h"
//#include "xrMU_Model.h"
//#include "xrMU_Model_Reference.h"

#include "xrlc_globaldata.h"

#include "net_execution_factory.h"
#include "lcnet_task_manager.h"

#include "net_execution_mu_ref.h"
#include "net_execution_mu_base.h"
#include "net_exec_pool.h"
#include "net_cl_data_prepare.h"


//void get_intervals( u32 max_threads, u32 num_items, u32 &threads, u32 &stride, u32 &rest );
void SetMuModelsLocalCalcLighteningCompleted();
namespace lc_net{
	exec_pool *ref_models_pool = 0;
	exec_pool *base_models_pool = 0;
	void RunRefModelsNet( )
	{
			

			SetRefModelLightDataInitialized();

			const u32	num_tasks	= inlc_global_data()->mu_refs().size();
			if( num_tasks == 0 )
			{
				SetMuModelsLocalCalcLighteningCompleted();
				return;
			}
			for (u32 thID=0; thID<num_tasks; thID++)
			{

				// Light references
				//u32	stride				= 0;
				//u32	last				= 0;
				//u32 tasks				= 0;
				//const u32 max_tasks		= 32;
				//get_intervals( max_tasks, inlc_global_data()->mu_refs().size(), threads, stride, last );

				tnet_execution_base< et_mu_ref_light > *el = lc_net::execution_factory.create<et_mu_ref_light>();
				el->implementation( ).construct(thID);
				get_task_manager().add_task( el );
				
			}
			
			ref_models_pool = get_task_manager().run( "Net Models Lighting" );
			SetMuModelsLocalCalcLighteningCompleted();

			
	}
	void WaitRefModelsNet()
	{
		if( !ref_models_pool )
				return;
		R_ASSERT(ref_models_pool);
		ref_models_pool->wait();
	}

	void RunBaseModelsNet( )
	{
			WaitNetBaseCompileDataPrepare( );

			const u32 num = inlc_global_data()->mu_models().size();
			if( num == 0 )
					return;
			for (u32 i=0; i<num; i++)
			{
				tnet_execution_base< et_mu_base_light > *el = lc_net::execution_factory.create<et_mu_base_light>();
				el->implementation( ).construct(i);
				get_task_manager().add_task( el );
			}
			
			base_models_pool = get_task_manager().run( "Net Base Models Lighting" );
	}
	
	void WaitBaseModelsNet( )
	{
		if( !base_models_pool )
				return;
		R_ASSERT(base_models_pool);
		base_models_pool->wait();
	}

}