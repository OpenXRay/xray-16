#pragma once

struct dbg_matrix
{
	Fmatrix				b2goal_gl;
	Fmatrix				b3goal_gl;

	Fmatrix				b2goal_lc;
	Fmatrix				b3goal_lc;
	Fmatrix				b3tob3_goal;

	Fmatrix				b2start_gl;
	Fmatrix				b3start_gl;

	Fmatrix				b2start_lc;
	Fmatrix				b3start_lc;
	Fmatrix				b3tob3_start;
	
	Fmatrix				OBJ;
	Fmatrix				OBJ_END;
	Fmatrix				GOAL;
	Fmatrix				dbg_goal;
	u16					ref_bone;
	dbg_matrix()
	{
		b2goal_gl			=Fidentity;
		b3goal_gl			=Fidentity;

		b2goal_lc			=Fidentity;
		b3goal_lc			=Fidentity;
		b3tob3_goal			=Fidentity;

		b2start_gl			=Fidentity;
		b3start_gl			=Fidentity;

		b2start_lc			=Fidentity;
		b3start_lc			=Fidentity;
		b3tob3_start		=Fidentity;

		OBJ					=Fidentity;
		OBJ_END				=Fidentity;
		dbg_goal  = GOAL	=Fidentity;
		ref_bone			= u16(-1);
	}
};
struct SCalculateData;
struct dbg_matrises
{
	dbg_matrix				dbg_m;
	xr_vector<dbg_matrix>	old_dbg_m;
	void					next_state				( SCalculateData &cd );
	void					next_goal				(const SCalculateData &cd );
};