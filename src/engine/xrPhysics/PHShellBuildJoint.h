#pragma once

#include "../xrengine/bone.h"

#include "PHJoint.h"
//#include "PHElement.h"

static const Fvector 	X 			= { 1, 0, 0 };
static const Fvector 	Y 			= { 0, 1, 0 };
static const Fvector 	Z 			= { 0, 0, 1 };
static const Fvector	basis[3]	= { X, Y, Z };	

IC void SetJoint( CPhysicsJoint	&J, const SJointIKData& joint_data )
{
	J.SetAnchorVsSecondElement	(0,0,0);
	J.SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
}


IC void SetJointLimit( CPhysicsJoint	&J, const IBoneData &bone_data, u8 limit_num, u8 axis_num )
{
	const SJointIKData& joint_data	=	bone_data.get_IK_data();
	const SJointLimit&	limit		=	joint_data.limits[limit_num];
	float lo = bone_data.lo_limit(limit_num);//limit.x;
	float hi = bone_data.hi_limit(limit_num);//limit.y;

	J.SetLimits(lo,hi,axis_num);
	J.SetAxisSDfactors(limit.spring_factor,limit.damping_factor,axis_num);
	
}

IC bool IsFreeRLimit( const IBoneData &bone_data, u8 limit_num )
{
	//const SJointIKData& joint_data	=	bone_data.get_IK_data();
	//const SJointLimit&	limit		=	joint_data.limits[limit_num];
	float lo = bone_data.lo_limit(limit_num);//limit.x;
	float hi = bone_data.hi_limit(limit_num);//limit.y;
	return !(hi-lo<M_PI*2.f);
}

IC void SetJointRLimit( CPhysicsJoint	&J, const IBoneData &bone_data, u8 limit_num, u8 axis_num )
{
	if(!IsFreeRLimit(bone_data,limit_num))
	{
		SetJointLimit( J, bone_data, limit_num, axis_num );
		//J->SetLimits(lo,hi,axis_num);
		//J->SetAxisSDfactors(limit.spring_factor,limit.damping_factor,axis_num);
	}
}





IC CPhysicsJoint	*CtreateHinge(const IBoneData &bone_data, u8 limit_num, CPhysicsElement* root_e, CPhysicsElement* E  )
{
	u8 axis_num =0;
	const Fvector axis = basis[ limit_num ];


	const SJointIKData& joint_data=bone_data.get_IK_data();
	CPhysicsJoint	* J = P_create_Joint(CPhysicsJoint::hinge,root_e,E);
	//J= P_create_Joint(CPhysicsJoint::hinge,root_e,E);

	SetJoint( *J, joint_data );

	J->SetAxisDirVsSecondElement ( axis.x, axis.y, axis.z, axis_num );


	SetJointLimit( *J, bone_data, limit_num, axis_num );
	return J;
}

IC CPhysicsJoint	*CtreateFullControl(const IBoneData &bone_data,  u8 limit_num[3], CPhysicsElement* root_e, CPhysicsElement* E  )
{



			const SJointIKData& joint_data=bone_data.get_IK_data();
			//CPhysicsJoint	* J = P_create_Joint(CPhysicsJoint::hinge,root_e,E);
			CPhysicsJoint	*J= P_create_Joint(CPhysicsJoint::full_control,root_e,E);
			SetJoint( *J, joint_data );
			//J->SetAnchorVsSecondElement	(0,0,0);
			//J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
			

			const bool set_axis[3] = { true, false, true };
			for( u8 i = 0; i < 3; ++i )
				if(set_axis[i])
					J->SetAxisDirVsSecondElement(basis[limit_num[i]],i);

			for( u8 i = 0; i < 3; ++i )
				SetJointLimit( *J, bone_data, limit_num[i], i );

			return J;


}

IC CPhysicsJoint	*BuildWheelJoint( const IBoneData &bone_data, CPhysicsElement* root_e, CPhysicsElement* E )				
	{
		const SJointIKData& joint_data=bone_data.get_IK_data();
		CPhysicsJoint	*J= P_create_Joint(CPhysicsJoint::hinge2,root_e,E);

		//J->SetAnchorVsSecondElement	(0,0,0);
		//J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
		SetJoint( *J, joint_data );


		J->SetAxisDirVsSecondElement(1,0,0,0);
		J->SetAxisDirVsSecondElement(0,0,1,1);

		//if(joint_data.limits[0].limit.y-joint_data.limits[0].limit.x<M_PI*2.f)
		//{
		//	J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,0);	
		//	J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,0);
		//}
		SetJointLimit( *J, bone_data, 0, 0 );
		return J;
	}
IC CPhysicsJoint	*BuildSliderJoint( const IBoneData &bone_data, CPhysicsElement* root_e, CPhysicsElement* E )	
{
		const SJointIKData& joint_data=bone_data.get_IK_data();
		CPhysicsJoint	*J= P_create_Joint(CPhysicsJoint::slider,root_e,E);
		/////////////////////////////////////////////////////////////////////////////////////
		//J->SetAnchorVsSecondElement	(0,0,0);
		//J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
		SetJoint( *J, joint_data );

		J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,0);
		J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,0);

		//if(joint_data.limits[1].limit.y-joint_data.limits[1].limit.x<M_PI*2.f)
		//{
		//	J->SetLimits(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y,1);
		//	J->SetAxisSDfactors(joint_data.limits[1].spring_factor,joint_data.limits[1].damping_factor,1);
		//}
		SetJointLimit( *J, bone_data, 1, 1 );
		return J;
}

IC CPhysicsJoint	*BuildBallJoint( const IBoneData &bone_data, CPhysicsElement* root_e, CPhysicsElement* E )
{
	const SJointIKData& joint_data=bone_data.get_IK_data();
	CPhysicsJoint	*J= P_create_Joint(CPhysicsJoint::ball,root_e,E);
	SetJoint( *J, joint_data );
	//J->SetAnchorVsSecondElement	(0,0,0);
	//J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
	return J;
}

IC CPhysicsJoint	*BuildGenericJoint( const IBoneData &bone_data, CPhysicsElement* root_e, CPhysicsElement* E )
{
	const SJointIKData& joint_data=bone_data.get_IK_data();

	
		bool	eqx=!!fsimilar(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y),
				eqy=!!fsimilar(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y),
				eqz=!!fsimilar(joint_data.limits[2].limit.x,joint_data.limits[2].limit.y);

		if(eqx)
		{
			if(eqy)
			{

				return CtreateHinge( bone_data, 2, root_e, E );
			}
			if(eqz)
			{

				return CtreateHinge( bone_data, 1, root_e, E );

			}


			u8 axis_limits[3]	= { 2, 0, 1 };
			return CtreateFullControl( bone_data,  axis_limits , root_e, E );
		}

		if(eqy)
		{
			if(eqz)
			{

				return CtreateHinge( bone_data, 0, root_e, E );
			}


			u8 axis_limits[3]	= { 2, 1, 0 };
			return CtreateFullControl( bone_data, axis_limits , root_e, E );

		}

		if(eqz)
		{

			u8 axis_limits[3]	= { 0, 2, 1 };
			return CtreateFullControl( bone_data,  axis_limits , root_e, E );

		}


		u8 axis_limits[3]	= { 2, 0, 1 };
		return CtreateFullControl( bone_data,  axis_limits , root_e, E );
	
	//return J;
}
IC CPhysicsJoint	*BuildJoint( const IBoneData &bone_data, CPhysicsElement* root_e, CPhysicsElement* E )
{
	CPhysicsJoint	*J= 0;
	const SJointIKData& joint_data=bone_data.get_IK_data();
		switch(joint_data.type) 
		{
		case jtSlider: 
			J =BuildSliderJoint( bone_data, root_e,E );
			VERIFY(J);
			break;
		case jtCloth: 
			J =BuildBallJoint( bone_data, root_e,E );
			VERIFY(J);
			break;
		case jtJoint:
			J = BuildGenericJoint( bone_data, root_e,E );
			VERIFY(J);
			break;
		case jtWheel:
			J = BuildWheelJoint( bone_data, root_e, E );
			VERIFY(J);
			break;
		case jtNone: break;

		default: NODEFAULT;
		}

		if(J)			
			J->SetForceAndVelocity(joint_data.friction);//joint_data.friction

		

	return J;
}