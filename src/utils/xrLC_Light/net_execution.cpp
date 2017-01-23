#include "stdafx.h"
#include "net_execution.h"

namespace lc_net
{
/*
	bool net_execution::run( IAgent* agent, DWORD sessionId, IGenericStream* inStream, IGenericStream* outStream )
	{
		if( receive_task( agent, sessionId,  inStream ) &&
			execute() )
		{
			send_result( outStream );
			return true;
		}
		return false;
	}
*/
	void net_execution::send_task( IGridUser& user, IGenericStream* outStream, u32  id )
	{
		_id = id;
	}
}