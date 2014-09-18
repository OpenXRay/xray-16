#pragma once

#include "ik_calculate_state.h"


class CIKLimb;
struct SCalculateData  {

	float	const		*m_angles			;
	CIKLimb				*m_limb				;
	Fmatrix	const		*m_obj				;

	bool				do_collide  		;

	calculate_state		state				;
	Fvector				cl_shift			;
	bool				apply				;
	float				l					;
	float				a					;

public:
	SCalculateData( ):
	state( ), m_limb( 0 ), m_obj( 0 ), 
	cl_shift( Fvector( ).set( 0, 0, 0 ) ), m_angles( 0 ), apply( false ), do_collide( false ),l( 0.f ), a( 0.f ) {}

	SCalculateData( CIKLimb& l, const Fmatrix &o );
public:
	IC	Fmatrix& goal( Fmatrix &g ) const;
};

//#define IK_DBG_STATE_SEQUENCE
#ifdef	IK_DBG_STATE_SEQUENCE
extern u32	sdbg_state_sequence_number;
#include	"ik_dbg_matrix.h"
#endif

