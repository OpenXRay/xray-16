#pragma once

#include "interactive_motion.h"
#include "../Include/xrRender/Kinematics.h"
class imotion_position;
enum motion_history_state;
class imotion_position:
	public interactive_motion
{
private:
struct tracks_update: public IUpdateTracksCallback
	{
		tracks_update( ): motion( 0 ), update( false ) {}
		virtual	bool	operator () ( float dt, IKinematicsAnimated& k );
		imotion_position *motion;
		bool update;
	} update_callback;
	float					time_to_end;
	UpdateCallback			saved_visual_callback;
	CBlend					*blend;
	bool					shell_motion_has_history;
public:
	imotion_position();
private:
	typedef			interactive_motion inherited;
	virtual	void	move_update	(  );
			float	motion_collide		( float dt, IKinematicsAnimated& k );
			void	collide_not_move	( IKinematicsAnimated& KA );
			void	force_calculate_bones( IKinematicsAnimated& KA );
			float	advance_animation	( float dt, IKinematicsAnimated& k );
			float	collide_animation	( float dt, IKinematicsAnimated& k );
			float	move				( float dt, IKinematicsAnimated& k );
			void	disable_update		(bool v);
	virtual	void	collide		(  ){};
	virtual	void	state_end	(  );
	virtual	void	state_start (  );

static	void	_BCL	rootbone_callback	( CBoneInstance *BI );

		void	init_bones			();
		void	deinit_bones		();
		void	set_root_callback	();
		void	remove_root_callback();

			void	interactive_motion_diagnostic( LPCSTR message );
};