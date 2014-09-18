#include "stdafx.h"

#include "global_slots_data.h"

void	global_slots_data::	Load			( )
{
	// Load .details
	// copy
	//if()
	
	IReader*	R		= FS.r_open	( "$level$", "build.details" );
	R->r_chunk			( 0, &dtH );
	R->seek				( 0 );
	u32 check_sum		= crc32( R-> pointer(), R->length());

	recalculation_data.load( check_sum );
	if( !recalculation_data.recalculating() )
	{
		IWriter*	W		= FS.w_open	( "$level$", "level.details" );
		W->w				( R->pointer(), R->length() );
		FS.w_close			( W );
	}

	FS.r_close			( R );
	
	// re-open
	string_path			N;
	FS.update_path		( N, "$level$", "level.details" );
	dtFS				= xr_new<CVirtualFileRW> ( N );

	R_ASSERT			( dtH.version()==DETAIL_VERSION );

	dtFS->find_chunk	( 2 );
	dtS		= (DetailSlot*)dtFS->pointer();
}

void	global_slots_data::Free			()
{
	if ( dtFS )	
		xr_delete( dtFS );
	recalculation_data.close();
}