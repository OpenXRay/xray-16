#pragma	once
struct	SHit;
class	CPhysicsShell;
class	character_shell_control
{
	
public:
	character_shell_control				();
	float 								curr_skin_friction_in_death	()					{ return m_curr_skin_friction_in_death; }
	void								set_kill_hit				( SHit &H ) const;
	void								set_fatal_impulse			( SHit &H ) const;
	void								set_start_shell_params		( CPhysicsShell	* sh ) const;
	void								apply_start_velocity_factor	(CObject* who, Fvector &velocity ) const;
	void								Load						( LPCSTR section );
	void								TestForWounded				( const Fmatrix& xform, IKinematics* CKA );
	void								UpdateFrictionAndJointResistanse( CPhysicsShell	* sh );
	void								CalculateTimeDelta			();
private:
	//skeleton modell(!share?)
	float								skel_airr_lin_factor																																;
	float								skel_airr_ang_factor																																;
	float								hinge_force_factor1																																	;
	float								skel_fatal_impulse_factor																															;
	float								skel_ddelay																																			;
	float								skel_remain_time																																	;	
/////////////////////////////////////////////////
	//bool								b_death_anim_on																																		;
	//bool								b_skeleton_in_shell																																	;
///////////////////////////////////////////////////////////////////////////
	float								m_shot_up_factor																																	;
	float								m_after_death_velocity_factor																														;
	
	//gray_wolf>Переменные для поддержки изменяющегося трения у персонажей во время смерти
	float								skeleton_skin_ddelay;
	float								skeleton_skin_remain_time;
	float								skeleton_skin_friction_start;
	float								skeleton_skin_friction_end;
	float								skeleton_skin_ddelay_after_wound;
	float								skeleton_skin_remain_time_after_wound;

	float								m_Pred_Time;//Для вычисления дельта времени между пересчётами сопротивления в джоинтах и коэффициента NPC
	float								m_time_delta;
	float								pelvis_factor_low_pose_detect;
	BOOL								character_have_wounded_state;
	bool								m_was_wounded;
	//gray_wolf<

	//gray_wolf>
	float								m_curr_skin_friction_in_death;
	//gray_wolf<
};