#pragma once
#include "hxgrid/Interface/IAgent.h"
#include "hxgrid/Interface/hxgridinterface.h"
//interface IGridUser;
class INetReader;
class XRLC_LIGHT_API net_task_manager
{
	friend void  Finalize(IGenericStream* outStream);
	xr_vector<u32>	pool;
	CTimer			start_time;
	float			thProgress;
	void	send( IGridUser& user, u32 id  );
	void	receive( INetReader& r);
public:
			net_task_manager( );
	void	run();
	void	create_global_data_write( LPCSTR save_path );
};



XRLC_LIGHT_API net_task_manager * get_net_task_manager();
XRLC_LIGHT_API void				  create_net_task_manager();
XRLC_LIGHT_API void				  destroy_net_task_manager();