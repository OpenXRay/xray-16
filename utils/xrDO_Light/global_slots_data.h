#ifndef __GLOBAL_SLOTS_DATA_H__
#define __GLOBAL_SLOTS_DATA_H__

#include "detailformat.h"
#include "recalculation.h"

class global_slots_data
{
private:
	DetailHeader					dtH;
	DetailSlot						*dtS;
	CVirtualFileRW					*dtFS;
	recalculation					recalculation_data;
public:
	global_slots_data(): dtS( 0 ), dtFS( 0 ), recalculation_data( dtH )	{}

	void				Load			();
	void				Free			();


	IC u32 size_x() const
	{
		return dtH.x_size();
	}

	IC u32 size_z() const
	{
		return dtH.z_size();
	}
	IC void set_slot_calculated( int _x, int _z )
	{
		recalculation_data.set_slot_calculated( _x, _z );
	}
	IC bool calculate_ignore( int _x, int _z ) const
	{
		return recalculation_data.skip_slot( _x, _z );
	}

	IC bool skip_slot( int _x, int _z ) const
	{
		return	is_empty( get_slot( _x, _z ) ) ||
				calculate_ignore( _x, _z ) ;
	}

	IC DetailSlot&	get_slot( int _x, int _z )
	{
		return dtS[ dtH.slot_index( _x, _z ) ];
	}

	IC const DetailSlot&	get_slot( int _x, int _z ) const
	{
		return dtS[ dtH.slot_index( _x, _z ) ];
	}



	IC Fvector& get_slot_box_min( Fvector &min, int _x, int _z ) const
	{
		const DetailSlot& DS =  get_slot( _x, _z );
		min.set( dtH.slot_min_x( _x ),
				 DS.r_ybase()	,
				 dtH.slot_min_z( _z ) 
				);
		return min;
	}


	IC Fvector& get_slot_box_max( Fvector &max, int _x, int _z ) const
	{
		Fvector min, diameter;
		get_slot_box_min( min, _x, _z );
		get_slot_diameter( diameter, get_slot( _x, _z ) );
		max.add( min, diameter );
		return max;
	}

	IC void get_slot_box( Fbox &BB, int _x, int _z ) const
	{
		get_slot_box_min(  BB.min, _x, _z ) ;

		Fvector diameter;
		get_slot_diameter( diameter, get_slot( _x, _z ) );

		BB.max.add	( BB.min, diameter );
		BB.grow		( 0.05f );
	}

};





#endif //__GLOBAL_SLOTS_DATA_H__