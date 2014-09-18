#pragma once
#include "control_com_defs.h"

#include "control_animation.h"
#include "control_direction.h"
#include "control_path_builder.h"
#include "control_movement.h"

// Lain: added
#ifdef DEBUG
namespace debug { class text_tree; }
#endif

class CBaseMonster;
class CControl_Com;

class CControl_Manager {
	CBaseMonster			*m_object;
	xr_vector<CObject*>		m_nearest;

	DEFINE_VECTOR			(CControl_Com *, CONTROLLERS_VECTOR, CONTROLLERS_VECTOR_IT);
	DEFINE_MAP				(ControlCom::EEventType, CONTROLLERS_VECTOR, LISTENERS_MAP, LISTENERS_MAP_IT);	
	LISTENERS_MAP			m_listeners;

	// contains all available controllers
	DEFINE_MAP				(ControlCom::EControlType, CControl_Com*, CONTROLLERS_MAP, CONTROLLERS_MAP_IT);
	CONTROLLERS_MAP			m_control_elems;
	CONTROLLERS_MAP			m_base_elems;
	
	DEFINE_VECTOR			(CControl_Com*, COM_VEC, COM_VEC_IT);
	COM_VEC					m_active_elems;

	CControlAnimation		*m_animation;
	CControlDirection		*m_direction;
	CControlMovement		*m_movement;
	CControlPathBuilder		*m_path;

public:
							CControl_Manager			(CBaseMonster *);
							~CControl_Manager			();

					// common routines
					void	init_external				();
					void	load						(LPCSTR section);
					void	reinit						();
					void	reload						(LPCSTR section);
					void	update_schedule				();
					void	update_frame				();

					// event handling routines
					void	notify						(ControlCom::EEventType, ControlCom::IEventData*);
					void	subscribe					(CControl_Com*, ControlCom::EEventType);
					void	unsubscribe					(CControl_Com*, ControlCom::EEventType);

					// add new control block
					void	add							(CControl_Com*, ControlCom::EControlType);
					void	set_base_controller			(CControl_Com*, ControlCom::EControlType);
	
					// capturing/releasing
					void	capture						(CControl_Com*, ControlCom::EControlType); // who, type
					void	release						(CControl_Com*, ControlCom::EControlType); // who, type
					bool	check_capturer				(CControl_Com *com, ControlCom::EControlType type);

					// Lain: added
			        CControl_Com* get_capturer          (ControlCom::EControlType type);
					ControlCom::EControlType com_type   (CControl_Com*);


					void	capture_pure				(CControl_Com*);
					void	release_pure				(CControl_Com*);

					void	activate					(ControlCom::EControlType type);
					void	deactivate					(ControlCom::EControlType type);
					void	deactivate					(CControl_Com*);

					void	lock						(CControl_Com*, ControlCom::EControlType);
					void	unlock						(CControl_Com*, ControlCom::EControlType);

	ControlCom::IComData	*data						(CControl_Com*, ControlCom::EControlType);

		CControlAnimation	&animation					() {return (*m_animation);}
		CControlDirection	&direction					() {return (*m_direction);}
		CControlPathBuilder	&path_builder				() {return (*m_path);}
		CControlMovement	&movement					() {return (*m_movement);}

					void	install_path_manager		(CControlPathBuilder *);

					bool	is_captured					(ControlCom::EControlType);
					bool	is_captured_pure			();

					// utilities

					void	path_stop					(CControl_Com*);
					void	move_stop					(CControl_Com*);
					void	dir_stop					(CControl_Com*);

					bool	check_start_conditions		(ControlCom::EControlType);

					// path buidler specials
					bool	build_path_line				(CControl_Com*, const Fvector &target, u32 node, u32 vel_mask);

					// Lain: added
					#ifdef DEBUG
					void    add_debug_info              (debug::text_tree& root_s);
					#endif
		
private:

		bool				is_pure						(CControl_Com*);
		bool				is_base						(CControl_Com*);
		bool				is_locked					(CControl_Com*);
		void				check_active_com			(CControl_Com *com, bool b_add);
};


