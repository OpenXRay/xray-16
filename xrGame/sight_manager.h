////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_manager.h
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Sight manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "setup_manager.h"
#include "sight_control_action.h"
#include "xrServer_Space.h"

namespace LevelGraph {
	class CVertex;
};

class CAI_Stalker;

class CSightManager : public CSetupManager<CSightControlAction,CAI_Stalker,u32> {
public:
	typedef CSetupManager<CSightControlAction,CAI_Stalker,u32> inherited;

public:
	enum aiming_type {
		aiming_none				= u32(0),
		aiming_weapon 			= u32(1),
		aiming_head				= u32(2),
	}; // enum
	enum animation_frame_type {
		animation_frame_none	= u32(0),
		animation_frame_start 	= u32(1),
		animation_frame_end		= u32(2),
	}; // enum

private:
	struct bone_parameters_base {
		Fmatrix					m_rotation;
	}; // struct bone_parameters_base

	struct bone_parameters : bone_parameters_base {
		Fmatrix					m_rotation;
		float					m_factor;
	}; // struct bone_parameters

	struct parameters {
		bone_parameters			m_spine;
		bone_parameters			m_shoulder;
		bone_parameters			m_head;
	}; // struct parameters

	struct parameters_base {
		bone_parameters_base	m_spine;
		bone_parameters_base	m_shoulder;
		bone_parameters_base	m_head;
	}; // struct parameters

private:
	parameters					m_current;
	parameters_base				m_target;
	shared_str					m_animation_id;
	aiming_type					m_aiming_type;
	animation_frame_type		m_animation_frame;

private:
	float						m_max_left_angle;
	float						m_max_right_angle;
	bool						m_enabled;
	bool						m_turning_in_place;

private:
			bool	aim_target							(Fvector &my_position, Fvector &aim_target, const CGameObject *object) const;
			void	process_action						(float const time_delta );

public:
					CSightManager						(CAI_Stalker *object);
	virtual	void	Load								(LPCSTR section);
	virtual	void	reinit								();
	virtual	void	reload								(LPCSTR section);
			void	remove_links						(CObject *object);
			void	Exec_Look							(float dt);
			void	SetPointLookAngles					(const Fvector &tPosition, float &yaw, float &pitch, Fvector const& look_position, const CGameObject *object = 0);
			void	SetFirePointLookAngles				(const Fvector &tPosition, float &yaw, float &pitch, Fvector const& look_position, const CGameObject *object = 0);
			void	SetDirectionLook					();
			void	SetLessCoverLook					(const LevelGraph::CVertex *tpNode, bool bDifferenceLook);
			void	SetLessCoverLook					(const LevelGraph::CVertex *tpNode, float fMaxHeadTurnAngle, bool bDifferenceLook);
			void	vfValidateAngleDependency			(float x1, float &x2, float x3);
	IC		bool	GetDirectionAnglesByPrevPositions	(float &yaw, float &pitch);
			bool	GetDirectionAngles					(float &yaw, float &pitch);
	IC		bool	use_torso_look						() const;
	template <typename T1, typename T2, typename T3>
	IC		void	setup								(T1 _1, T2 _2, T3 _3);
	template <typename T1, typename T2>
	IC		void	setup								(T1 _1, T2 _2);
	template <typename T1>
	IC		void	setup								(T1 _1);
			void	setup								(const CSightAction &sight_action);
	virtual void	update								();
	IC		bool	turning_in_place					() const;
	IC		bool	enabled								() const;
			void	enable								(bool value);

private:
			void	adjust_orientation					();
			void	slerp_rotations						(float time_delta, float angular_speed);
			void	compute_aiming						(float time_delta, float angular_speed);

public:
			Fvector	aiming_position						() const;
			Fvector	object_position						() const;

public:
	IC		Fmatrix	const&	current_spine_rotation		() const;
	IC		Fmatrix	const&	current_shoulder_rotation	() const;
	IC		Fmatrix	const&	current_head_rotation		() const;

public:
	IC		void			bone_aiming					();
	IC		void			bone_aiming					(
								shared_str const& animation_id,
								animation_frame_type const animation_frame,
								aiming_type const aiming_type
							);
};

#include "sight_manager_inline.h"