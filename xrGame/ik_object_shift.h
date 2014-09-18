#pragma once
namespace extrapolation{class points;}
class object_shift
{
	float	current;
	float	taget;
	float	taget_time;
	float	current_time;
	float	speed;
	float	accel;
	float	aaccel;
	bool	b_freeze;
public:
			object_shift		( ): current( 0.f ), taget( 0.f ),current_time( 0 ), taget_time( 0 ), speed( 0.f ), accel( 0.f ), aaccel( 0 ), b_freeze( false ){}
	void	set_taget			( float taget, float time );
	float	shift				( ) const ;
	void	freeze				( bool v ){ b_freeze = v; }
private:
	float	shift				( float time_global )const; 
	float	delta_shift			( float delta_time ) const;
#ifdef	DEBUG
public:
	void	dbg_draw			( const Fmatrix	&current_pos, const extrapolation::points &predict , const Fvector& start ) const;
private:
#endif
};