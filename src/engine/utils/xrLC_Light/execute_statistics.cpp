////////////////////////////////////////////////////////////////////////////
//	Created		: 04.06.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "execute_statistics.h"
#include "serialize.h"

LPCSTR make_time	( string64 &buf, float fsec );
#ifdef	COLLECT_EXECUTION_STATS

void	execute_time_statistics::read				( INetReader	&r )
{
	r_pod(r,*this);
}
void	execute_time_statistics::write				( IWriter	&w ) const 
{
	w_pod(w,*this);
}
void	execute_time_statistics::log					()const
{
	string64 buf;
	Msg( "calc time: %s ", make_time( buf, m_time ) );
}

void	execute_statistics::read				( INetReader	&r )
{
	r_pod( r, dir ); 
	time_stats.read( r );
}
void	execute_statistics::write				( IWriter	&w ) const
{
	w_pod( w, dir );
	time_stats.write( w );
}
void	execute_statistics::log					()const
{
	Msg( "agent: %s", dir ); 
	time_stats.log();
}

#endif