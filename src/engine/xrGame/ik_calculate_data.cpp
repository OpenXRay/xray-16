#include	"stdafx.h"

#include	"ik_calculate_data.h"

#include	"ik/iklimb.h"

SCalculateData::SCalculateData( CIKLimb& l, const Fmatrix &o ):
	state( )								,
	m_limb( &l )							,
	m_obj( &o )								,
	cl_shift( Fvector( ).set( 0, 0, 0 ) )	, 
	m_angles( 0 )							, 
	apply( false )							,
	do_collide( false ) 
{

}