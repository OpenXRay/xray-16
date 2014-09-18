////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_action.h
//	Created 	: 27.12.2003
//  Modified 	: 03.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Sight action
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "sight_manager_space.h"
#include "control_action.h"

namespace MemorySpace {
	struct CMemoryInfo;
};

class CAI_Stalker;
class CGameObject;

class CSightAction : public CControlAction {
public:
	typedef SightManager::ESightType	ESightType;
	typedef MemorySpace::CMemoryInfo	CMemoryInfo;

protected:
	xr_vector<CObject*>	objects;
	ESightType			m_sight_type;
	bool				m_torso_look;
	bool				m_path;
	bool				m_no_pitch;
	Fvector				m_vector3d;
	const CGameObject	*m_object_to_look;
	const CMemoryInfo	*m_memory_object;
	u32					m_start_time;

protected:
	u32					m_time;
	u32					m_internal_state;
	u32					m_start_state_time;
	u32					m_stop_state_time;
	float				m_cover_yaw;

protected:
	Fvector				m_holder_start_position;
	Fvector				m_object_start_position;
	u32					m_state_fire_object; // 0 : standard object navigation, 1 : vis_point navigation
	u32					m_state_fire_switch_time;
	bool				m_already_switched;
	bool				m_initialized;

private:
			void		predict_object_position		(bool use_exact_position);

protected:
			void		execute_current_direction	();
			void		execute_path_direction		();
			void		execute_direction			();
			void		execute_position			(Fvector const& look_position);
			void		execute_object				();
			void		execute_cover				();
			void		execute_search				();
			void		initialize_cover_look_over	();
			void		execute_cover_look_over		();
			void		initialize_fire_object		();
			void		execute_fire_object			();
			void		execute_animation_direction	();

public:
	IC					CSightAction				();
	IC					CSightAction				(const ESightType &sight_type, bool	torso_look = false, bool path = false);
	IC					CSightAction				(const ESightType &sight_type, const Fvector &vector3d, bool torso_look = false);
	IC					CSightAction				(const ESightType &sight_type, const Fvector *vector3d);
	IC					CSightAction				(const CGameObject *object_to_look, bool torso_look = false, bool fire_object = false, bool no_pitch = false);
	IC					CSightAction				(const CMemoryInfo *memory_object, bool	torso_look = false);
			void		initialize					();
			void		execute						();
			void		finalize					();
			void		on_frame					();
	IC		void		set_vector3d				(const Fvector &position);
	IC		void		set_object_to_look			(const CGameObject *object_to_look);
	IC		void		set_memory_object			(const CMemoryInfo *object_to_look);
	IC		bool		operator==					(const CSightAction &sight_action) const;
			void		remove_links				(CObject *object);
			bool		change_body_speed			() const;
			float		body_speed					() const;
			bool		change_head_speed			() const;
			float		head_speed					() const;
	IC		ESightType	sight_type					() const;
	IC		const CGameObject	*object_to_look		() const;
	IC		const Fvector		&vector3d			() const;
			bool		target_reached				();
	IC		u32 const&	state_fire_object			() const;
};

#include "sight_action_inline.h"