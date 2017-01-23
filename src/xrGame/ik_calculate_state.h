#pragma once


class ik_goal_matrix
{
	public:
	enum e_collide_state
	{
		cl_free,
		cl_rotational,
		cl_translational,
		cl_mixed,
		cl_aligned,
		cl_undefined
	};

	private:
		e_collide_state	cl_state;
		Fmatrix			m;
	public:
						ik_goal_matrix	(): m( Fidentity ), cl_state( cl_undefined ){}
	IC	const Fmatrix	&get			()const										{ return m; }
	IC			void	set				( const Fmatrix &m_, e_collide_state cl )	{ m.set( m_ ); cl_state = cl; }
	IC	e_collide_state collide_state	()const										{ return cl_state; }
};

struct calculate_state
{
	u32				calc_time;
	u32				unstuck_time;
	ik_goal_matrix 	goal;
	ik_goal_matrix 	blend_to;
	Fmatrix 		anim_pos;
	ik_goal_matrix	collide_pos;
	Fmatrix			b2tob3;
	Fvector 		pick;
	float			speed_blend_l;
	float			speed_blend_a;
	bool			foot_step;
	bool			idle;
	bool			blending;
	u16				ref_bone;
#ifdef DEBUG
	int		count;
#endif
	calculate_state( ) : ref_bone( u16(-1) ),
						calc_time(0), unstuck_time(u32(-1)), foot_step(false), idle(false), blending(false),
						anim_pos(Fidentity),
						b2tob3( Fidentity ), speed_blend_l(0), speed_blend_a(0), pick(Fvector().set(0,-1,0))
#ifdef DEBUG
						, count(-1)
#endif
	{}
};