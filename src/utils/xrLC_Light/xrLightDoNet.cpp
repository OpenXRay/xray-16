////////////////////////////////////////////////////////////////////////////
//	Created		: 27.03.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrLightDoNet.h"
#include "global_calculation_data.h"
#include "detail_slot_calculate.h"
#include "net_execution_detail_light.h"
#include "detail_net_global_data.h"
#include "net_execution_factory.h"
#include "lcnet_task_manager.h"
//#include "net_exec_pool.h"
namespace lc_net
{

void	xrNetDOLight()
{
		
	get_task_manager().startup();
/////////////////////////////////////////////////////////////////////
	gl_data.slots_data.process_all_pallete();
	lc_net::globals().get<lc_net::gl_detail_cl_data>().init();
//////////////////////////////////////////////////////////////////////
	//for ( u32 _z=0; _z<gl_data.slots_data.size_z(); _z++ )
	//{
	//	for (u32 _x=0; _x<gl_data.slots_data.size_x(); _x++)
	//	{
	//		DetailSlot&	DS = gl_data.slots_data.get_slot( _x, _z );
	//		if( !detail_slot_process(  _x, _z, DS ) )
	//			continue;
	//		tnet_execution_base< et_detail_light > *el = lc_net::execution_factory.create<et_detail_light>();
	//		el->implementation( ).construct( _x, _z );
	//		get_task_manager().add_task( el );
	//	}
	//}
	
	u32 start = 0;
	u32 end = 0;
	u32 task_slots_number = 0;
	const u32 max_task_slots_number = 100;
	const u32 slots_count = gl_data.slots_data.header().slots_count();
	do{
		
		int x, z;
		gl_data.slots_data.header().slot_x_z( end, x, z );
		++end;
		if( gl_data.slots_data.calculate_ignore( x, z ) )
			continue;
		++task_slots_number;
		if( task_slots_number == max_task_slots_number || end == slots_count )
		{
			tnet_execution_base< et_detail_light > *el = lc_net::execution_factory.create<et_detail_light>();
			el->implementation( ).construct( start, end );
			get_task_manager().add_task( el );
			start = end;
			task_slots_number = 0;
		}
	}while( end != slots_count );

//////////////////////////////////////////////////////////////////////////
	get_task_manager().run("Net Detail Lighting");
	get_task_manager().wait_all();
	get_task_manager().release();
}

}