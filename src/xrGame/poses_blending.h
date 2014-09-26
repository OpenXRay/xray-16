#pragma once

class poses_interpolation
{
	Fvector		p0, p1;
	Fquaternion q0, q1;
public:
			poses_interpolation ( const Fmatrix &m0, const Fmatrix &m1 );
	void	pose				( Fmatrix &p, float factor ) const ;
};


class poses_blending
{
	poses_interpolation	interpolation;
	float				target_time;
public:
			poses_blending	( const Fmatrix &m0, const Fmatrix &m1, float target_time_ );

	void	pose			( Fmatrix &p, float time )									const ;
IC	bool	target_reached	( float time )												const 
			{ 
				return time >= target_time;
			}

};