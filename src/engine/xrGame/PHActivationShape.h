#ifndef PH_ACTIVATION_SHAPE
#define PH_ACTIVATION_SHAPE
#endif
#include "MathUtils.h"
#include "phvalidevalues.h"
class CPHActivationShape : public CPHObject
{
dBodyID					m_body																																							;
dGeomID					m_geom																																							;
Flags16					m_flags																																							;
CSafeFixedRotationState m_safe_state																																					;			

#ifdef		DEBUG
virtual		CPhysicsShellHolder	*ref_object					();
#endif

public:
enum					EType
{
					etBox,
					etCylinder,
					etSphere
};

enum		{
					flFixedRotation		=1<<0,
					flFixedPosition		=1<<1,
					flStaticEnvironment	=1<<2,
					flGravity			=1<<3
			};
							CPHActivationShape					()																															;
							~CPHActivationShape					()																															;
			void			Create								(const Fvector start_pos,const Fvector start_size,CPhysicsShellHolder* ref_obj,EType type=etBox,u16	flags=0)				;
			void			Destroy								()																															;
			bool			Activate							(const Fvector need_size,u16 steps,float max_displacement,float max_rotation,bool	un_freeze_later	=false)					;															
const		Fvector			&Position							()																															;
			void			Size								(Fvector &size)																												;
			dBodyID			ODEBody								()																											{return m_body	;}
			void			set_rotation						(const	Fmatrix	&rot)																											;
private:
virtual		void			PhDataUpdate						(dReal step)																												;
virtual		void			PhTune								(dReal step)																												;
virtual		void			CutVelocity							(float l_limit,float a_limit)																								;						
virtual		void			InitContact							(dContact* c,bool& do_collide,u16 ,u16 )																		;
virtual		dGeomID			dSpacedGeom							()																															;
virtual		void			get_spatial_params					()																															;
virtual		u16				get_elements_number					()																															{return 0;}
virtual		CPHSynchronize	*get_element_sync					(u16 element)																												{return NULL;}		

};