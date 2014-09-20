#ifndef __RECALCULATION_PARAMS_H__
#define __RECALCULATION_PARAMS_H__

#include	"detailformat.h"

class CVirtualFileRW;
class recalculation
{
	const DetailHeader				&dtH;
	u8								*slots_flags;
	CVirtualFileRW					*dtFS;

	Frect	calculation_rect;
	bool	recalculate;
	bool	partial_calculate;
	bool	force_recalculate;
public:
			recalculation	(const DetailHeader	&_dtH): dtH(_dtH), calculation_rect( Frect().invalidate() ),
														recalculate( false ),partial_calculate( false ), force_recalculate( false ),
														dtFS( 0 ), slots_flags( 0 )   {}//
IC	bool	recalculating	() const 
	{
		return recalculate; 
	}

IC	bool	skip_slot		( int x, int z ) const
	{
		
		if(partial_calculate)
		{
			Frect srect;
			dtH.GetSlotRect( srect, x, z );
			if( !calculation_rect.intersected( srect ) )
				return true;
		}
		if( !recalculating	() || force_recalculate )
			return false;
		return !!slots_flags[dtH.slot_index( x, z )];

	}
IC void set_slot_calculated( int x, int z )
	{
		slots_flags[dtH.slot_index( x, z )] = u8(1);
	}
	void	load			( u32 check_sum );
	void	close			();
private:
	void	load_calculation_params			();
	void	setup_recalculationflags_file	( u32 check_sum );
	void	check_load						( u32 check_sum );
	void	check_files						( u32 check_sum );
};


#endif //__RECALCULATION_PARAMS_H__