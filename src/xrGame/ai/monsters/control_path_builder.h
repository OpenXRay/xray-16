#pragma once

#include "control_combase.h"
#include "../../movement_manager.h"

class CCustomMonster;
class CControl_Manager;

struct SControlPathBuilderData : public ControlCom::IComData {
	bool		use_dest_orientation;
	Fvector		dest_orientation;
	
	bool		try_min_time;

	Fvector		target_position;
	u32			target_node;

	bool		enable;
	bool		extrapolate;

	u32			velocity_mask;
	u32			desirable_mask;

	bool		reset_actuality;
	
	MovementManager::EPathType path_type;
	u32			game_graph_target_vertex;
};


class CControlPathBuilder : 
	public CControl_ComPure<SControlPathBuilderData>,
	public CMovementManager
{
	typedef CMovementManager							inherited;
	typedef CControl_ComPure<SControlPathBuilderData>	inherited_com;

	friend	class CControl_Manager;

public:
					CControlPathBuilder		(CCustomMonster *monster);
	virtual			~CControlPathBuilder	();

	virtual void	load					(LPCSTR section);
	virtual void	reinit					();
	virtual void	update_schedule			();

	virtual	void	on_travel_point_change	(const u32 &previous_travel_point_index);
	virtual void	on_build_path			();
	virtual	bool	can_use_distributed_computations (u32 option) const;

			// services
			bool	is_path_end				(float dist_to_end);
			bool	valid_destination		(const Fvector &pos, u32 node);
			bool	valid_and_accessible	(Fvector &pos, u32 node);			// validate with a small correction
			bool	is_moving_on_path		();

			bool	get_node_in_radius		(u32 src_node, float min_radius, float max_radius, u32 attempts, u32 &dest_node);
			void	fix_position			(const Fvector &pos, u32 node, Fvector &res_pos);
			
	static	u32		find_nearest_vertex		(const u32 &level_vertex_id, const Fvector &target_position, const float &range);

			bool	is_path_built			();

private:
			bool	build_special			(const Fvector &target, u32 node, u32 vel_mask);
			void	make_inactual			();	
};
