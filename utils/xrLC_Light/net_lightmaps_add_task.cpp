#include "stdafx.h"
#include "net_execution_lightmaps.h"
#include "net_execution_factory.h"
#include "lcnet_task_manager.h"
#include "lcnet_execution_tasks_add.h"
#include "xrlc_globaldata.h"
#include "xrThread.h"
#include "xrdeflector.h"

namespace lc_net
{

	void net_lightmaps_add_task( u32 from, u32 to )
	{
		tnet_execution_base< et_lightmaps > *el = lc_net::execution_factory.create<et_lightmaps>();
		el->implementation( ).construct( from, to );
		get_task_manager().add_task( el );
	}
/*
	void net_lightmaps_add_all_tasks(  )
	{
		//

		u32	stride			= u32(-1);
		
		u32 threads			= u32(-1);
		u32 rest			= u32(-1);


		u32 size = inlc_global_data()->g_deflectors().size();

		get_intervals( 20000, size, threads, stride, rest );

		for (u32 thID=0; thID<threads; thID++)
			net_lightmaps_add_task( thID*stride,thID*stride + stride  );
		if(rest > 0)
			net_lightmaps_add_task( threads*stride,threads*stride + rest );
	}

*/

	static const u32	num_tasks = 20000;
	static const float	f_num_tasks = float ( num_tasks );

	static u32 get_next(u32 from, float &f_weight_per_task )
	{
		u32 size = inlc_global_data()->g_deflectors().size();
		R_ASSERT( from < size );
		float weight = 0;
		
		for( u32 i = from; i < size ; ++i )
		{
			weight += float( inlc_global_data()->g_deflectors()[i]->weight() );
			if( weight > f_weight_per_task )
			{
				f_weight_per_task = weight;
				return i+1;
			}
		}

		f_weight_per_task = weight;
		return size;
	}


	void net_lightmaps_add_all_tasks(  )
	{
		
		u32	stride			= u32(-1);
		
		
		u32 rest			= u32(-1);
		u32 local_num_tasks = num_tasks;

		u32 size = inlc_global_data()->g_deflectors().size();

		get_intervals( num_tasks, size, local_num_tasks, stride, rest );
		
		if(rest!=0)
			++local_num_tasks;
		
		//
		u32 total_weight = 0;
		xr_vector<CDeflector*>::const_iterator i = inlc_global_data()->g_deflectors().begin(),
			e = inlc_global_data()->g_deflectors().end();
		for( ; i!=e ; ++i )
			total_weight += (*i)->weight();
		
		

		float f_weight	= float( total_weight );
		
		u32 from = 0;

		for(;;)
		{
			if( local_num_tasks == 0 )
						local_num_tasks = 1;
			float f_weight_per_task	= f_weight/float( local_num_tasks );
			u32 to =  get_next( from, f_weight_per_task );
			f_weight -= f_weight_per_task;
			local_num_tasks--;

			net_lightmaps_add_task( from, to );
			from = to ;
			if( to >= size )
				break;
		}

		
	}



}