#pragma once
#include "hxgrid/Interface/IAgent.h"
#include "hxgrid/Interface/hxgridinterface.h"
//interface IGridUser;
class INetReader;
class XRLC_LIGHT_API net_task_menager
{
	friend void  Finalize(IGenericStream* outStream);
	xr_vector<u32>	pool;
	CTimer			start_time;
	float			thProgress;
	void	send( IGridUser& user, u32 id  );
	void	receive( INetReader& r);
public:
			net_task_menager( );
	void	run();
};

XRLC_LIGHT_API net_task_menager * get_net_task_menager();
XRLC_LIGHT_API void				  create_net_task_menager();
XRLC_LIGHT_API void				  destroy_net_task_menager();