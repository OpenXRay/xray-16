#pragma once
#include <boost/noncopyable.hpp>

#include "../Include/xrRender/KinematicsAnimated.h"
#include "poses_blending.h"

//class IKinematicsAnimated;
class poses_blending;
class CBlend;

class animation_movement_controller : 
	public  IBlendDestroyCallback, 
	private boost::noncopyable
{
	Fmatrix&			m_pObjXForm;
	Fmatrix				m_startObjXForm;
	poses_blending		m_poses_blending;
public:
	void				DBG_verify_position_not_chaged() const;
private:
#ifdef					DEBUG
	Fmatrix				DBG_previous_position;
#endif	
	//Fmatrix			m_startAnimPose;
	IKinematics			*m_pKinematicsC;
	IKinematicsAnimated *m_pKinematicsA;
	CBlend*				m_control_blend;
	bool				inital_position_blending;
	bool				stopped;
	float				blend_linear_speed;
	float				blend_angular_speed;
	static void	_BCL	RootBoneCallback				( CBoneInstance* B );
	void				deinitialize					();
	void				BlendDestroy					( CBlend& blend );
public:		
			animation_movement_controller		( Fmatrix	*_pObjXForm, const Fmatrix &inital_pose,  IKinematics *_pKinematicsC,CBlend *b );
virtual		~animation_movement_controller		( );
			void	ObjStartXform				( Fmatrix &m )const { m.set( m_startObjXForm ) ;}
			CBlend*	ControlBlend				( ) const { return m_control_blend; }
			void	NewBlend					( CBlend* B, const Fmatrix &new_matrix, bool local_animation );
			bool	IsActive					( ) const ;
			void	OnFrame						( );
private:
			void	GetInitalPositionBlenSpeed	( );
			void	animation_root_position		( Fmatrix &pos  );
			void	InitalPositionBlending		( const Fmatrix &to );
			void	SetPosesBlending			( );
public:
			bool	IsBlending					( ) const;
	inline	Fmatrix const& start_transform		( ) const
	{
		return		(m_startObjXForm);
	}

			void	stop						( );
};