#pragma once
#include "control_combase.h"

#include "anim_triple.h"
#include "control_jump.h"
#include "control_rotation_jump.h"
#include "control_melee_jump.h"


class CAnimationSequencer;
class CControlRotationJump;
class CControlRunAttack;
class CControlThreaten;
class CControlCriticalWound;
class CEntityAlive;

class CControlManagerCustom : public CControl_ComBase {
	typedef					CControl_ComBase	inherited;

	xr_vector<CObject*>		m_nearest;

	CAnimationSequencer		*m_sequencer;
	CAnimationTriple		*m_triple_anim;

	CControlRotationJump	*m_rotation_jump;
	CControlJump			*m_jump;
	CControlRunAttack		*m_run_attack;
	CControlThreaten		*m_threaten;
	CControlMeleeJump		*m_melee_jump;
	CControlCriticalWound	*m_critical_wound;

	DEFINE_VECTOR			(SControlRotationJumpData, ROT_JUMP_DATA_VEC, ROT_JUMP_DATA_VEC_IT);
	ROT_JUMP_DATA_VEC		m_rot_jump_data;
	
	SControlMeleeJumpData	m_melee_jump_data;

	LPCSTR					m_threaten_anim;
	float					m_threaten_time;

public:
					CControlManagerCustom	();
					~CControlManagerCustom	();

	virtual void	reinit					();
	virtual void	on_event				(ControlCom::EEventType, ControlCom::IEventData*);
	virtual void	on_start_control		(ControlCom::EControlType type);
	virtual void	on_stop_control			(ControlCom::EControlType type);
	virtual void	update_frame			();
	virtual void	update_schedule			();

			void	add_ability				(ControlCom::EControlType);

	//-------------------------------------------------------------------------------
	// Sequencer
	void		seq_init				();
	void		seq_add					(MotionID motion);
	void		seq_switch				();					// Перейти в следующее состояние, если такового не имеется - завершить
	void		seq_run					(MotionID motion);

	//-------------------------------------------------------------------------------
	// Triple Animation
	void		ta_activate				(const SAnimationTripleData &data);
	void		ta_pointbreak			();
	void		ta_fill_data			(SAnimationTripleData &data, LPCSTR s1, LPCSTR s2, LPCSTR s3, bool execute_once, bool skip_prep, u32 capture_type = ControlCom::eCaptureDir | ControlCom::eCapturePath | ControlCom::eCaptureMovement);
	bool		ta_is_active			();
	bool		ta_is_active			(const SAnimationTripleData &data);
	void		ta_deactivate			();
	
	//-------------------------------------------------------------------------------
	// Jump
	void		jump					(CObject *obj, const SControlJumpData &ta);
	bool		jump					(const SControlJumpData &ta);
	void		jump					(const Fvector &position);
	void		load_jump_data			(LPCSTR s1, LPCSTR s2, LPCSTR s3, LPCSTR s4, u32 vel_mask_prepare, u32 vel_mask_ground, u32 flags);
	bool		is_jumping				();

	bool		check_if_jump_possible	(Fvector const&		target, bool full_check);
	bool		jump_if_possible		(Fvector const&		target, 
										 CEntityAlive*		target_object,
										 bool 				use_target_direction,
										 bool				use_velocity_bounce	=	true,
										 bool				check_possibility	=	true);

	
	void		script_jump				(const Fvector &position, float factor);
	void		script_capture			(ControlCom::EControlType type);
	void		script_release			(ControlCom::EControlType type);
	//-------------------------------------------------------------------------------
	// Rotation Jump
	void		add_rotation_jump_data	(LPCSTR left1,LPCSTR left2,LPCSTR right1,LPCSTR right2, float angle, u32 flags = 0);
	void		add_melee_jump_data		(LPCSTR left,LPCSTR right);

	//-------------------------------------------------------------------------------
	// Threaten Animation
	void		set_threaten_data		(LPCSTR anim, float time) {m_threaten_anim = anim; m_threaten_time = time;}

	void		critical_wound			(LPCSTR anim);

	void		remove_links			(CObject * object);

	CControlJump*	get_jump_control	() { return m_jump; }
private:

	void		check_attack_jump		();
	void		check_jump_over_physics	();
	void		check_rotation_jump		();
	void		check_melee_jump		();
	void		check_run_attack		();
	void		check_threaten			();

	void		fill_rotation_data		(SControlRotationJumpData &data, LPCSTR left1,LPCSTR left2,LPCSTR right1,LPCSTR right2, float angle, u32 flags);
};

