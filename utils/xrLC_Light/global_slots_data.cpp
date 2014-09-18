#include "stdafx.h"

#include "global_slots_data.h"
#include "serialize.h"

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
	
//DetailHeader					dtH;
//DetailSlot						*dtS;
//CVirtualFileRW					*dtFS;
//recalculation					recalculation_data;

void global_slots_data::write( IWriter	&w ) const
{
	
	w_pod( w, dtH );
	const u32 buffer_size = sizeof( DetailSlot ) * dtH.slots_count();
	w.w( dtS, buffer_size );
	recalculation_data.write( w );
}

void global_slots_data::read( INetReader &r  )
{
	
	r_pod( r, dtH );
	R_ASSERT( !dtS );
	const u32 buffer_size = sizeof( DetailSlot ) * dtH.slots_count();
	dtS = ( DetailSlot* )xr_malloc( buffer_size );
	R_ASSERT(dtS);
	r.r( dtS, buffer_size );
	recalculation_data.read( r );
}

void	global_slots_data::FreeOnAgent		()
{
	xr_delete( dtS );
}

void global_slots_data::process_all_pallete()
{
	R_ASSERT( header( ).version() != u32(-1) );
	R_ASSERT( header( ).x_size() != u32(-1) );
	R_ASSERT( header( ).z_size() != u32(-1) );
	for ( u32 i=0; i < header().slots_count(); ++i )
	{
		
		DetailSlot& DS = get_slot( i );
		process_pallete( DS );
		
		if( is_empty( DS ) )
			recalculation_data.set_slot_calculated( i );
	}
}