#include "stdafx.h"
#include "xrLightVertex.h"

#include "xrface.h"
#include "xrLC_GlobalData.h"
#include "net_execution_vertex_light.h"

#include "net_execution_factory.h"
#include "lcnet_task_manager.h"
#include "net_exec_pool.h"

//#include "light_point.h"
bool GetTranslucency(const Vertex* V,float &v_trans );
u32		vertises_has_lighting = u32(-1);
u32 CalcAllTranslucency()
{
	vecVertex					&verts = lc_global_data()->g_vertices();
	u32 end_translucency = verts.size();
	for( u32 i = 0; i < end_translucency; ++i )
	{
		Vertex* V		= verts[i];
		float			v_trans		= 0.f;
		bool trans = GetTranslucency( V, v_trans );
		if(trans)
			V->C.t._w(v_trans);
		else
		{
			std::swap( verts[end_translucency-1], verts[i] );
			--end_translucency;
			--i;
		}
	}
	return end_translucency;
}

namespace lc_net{
void RunLightVertexNet()
{
	//u32 size = CalcAllTranslucency();
	R_ASSERT( vertises_has_lighting!=u32(-1) );

	u32 size = vertises_has_lighting;
	const u32	vertex_light_task_number = 2048;
	const u32	task_size = size/vertex_light_task_number;
	const u32	rest_size = size%vertex_light_task_number;
	if( task_size!=0 )
		for( u32 i = 0; i < vertex_light_task_number; ++i )
		{
			tnet_execution_base< et_vertex_light > *el = execution_factory.create<et_vertex_light>();
			el->implementation( ).construct( i*task_size, (i+1)*task_size );
			get_task_manager().add_task( el );
		}
	if( rest_size != 0 )
	{
		tnet_execution_base< et_vertex_light > *el = execution_factory.create<et_vertex_light>();
		el->implementation( ).construct( vertex_light_task_number*task_size, vertex_light_task_number*task_size+rest_size );
		get_task_manager().add_task( el );
	}

	exec_pool *pool = get_task_manager().run( "Net Vertex Lighting" );
	if(pool)
		pool->wait();
	
	//get_task_manager().wait_all();
}
}

