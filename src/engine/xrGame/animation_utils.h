#pragma once
class CBoneInstance;
class IKinematics;
struct anim_bone_fix
{
	CBoneInstance *bone;
	CBoneInstance *parent;
	Fmatrix		  matrix;

						anim_bone_fix();
						~anim_bone_fix();
static	void	_BCL	callback	( CBoneInstance *BI );
		void			fix			( u16 bone_id, IKinematics &K );
		void			refix		();
		void			release		();
		void			deinit		();
};

bool find_in_parents( const u16 bone_to_find, const u16 from_bone, IKinematics &ca );