#ifndef	_NET_TASK_CALLBACK_H_
#define	_NET_TASK_CALLBACK_H_
#include "hxgrid/Interface/IAgent.h"

class net_task_callback
{
	static const u16	_break_connection_times = 1;
	IAgent&				_agent		;
	DWORD				_session	;
	u16					_beak_count	;
public:
	net_task_callback( IAgent *agent, DWORD session ):
	_agent(*agent),
	_session(session),
	_beak_count( _break_connection_times ){}
public:
	IC	bool	break_all			( ) const	{ return _beak_count==0; } 
		bool	test_connection		( )			;
const	IAgent	&agent				( )const	{ return _agent; }
		IAgent	&agent				( )			{ return _agent; }
		DWORD	session				( )const	{ return _session; }
};

#endif