#include "pch_script.h"

#include "physics_joint_scripted.h"
#include "physics_element_scripted.h"
using namespace luabind;


cphysics_element_scripted*	cphysics_joint_scripted::	PFirst_element()
{
	CPhysicsElement* E = physics_impl().PFirst_element();
	if(!E)
		return 0;
	return get_script_wrapper<cphysics_element_scripted>(*E);
	
}

cphysics_element_scripted*	cphysics_joint_scripted::	PSecond_element	()
{
	CPhysicsElement* E = physics_impl().PSecond_element();
	if(!E)
		return 0;
	return get_script_wrapper<cphysics_element_scripted>(*E);
}

void cphysics_joint_scripted::script_register(lua_State *L)
{

	module(L)
		[
			class_<cphysics_joint_scripted>("physics_joint")
			.def("get_bone_id",							&cphysics_joint_scripted::BoneID)
			.def("get_first_element",					&cphysics_joint_scripted::PFirst_element)
			.def("get_stcond_element",					&cphysics_joint_scripted::PSecond_element)
			.def("set_anchor_global",					(void(cphysics_joint_scripted::*)(const float,const float,const float))(&cphysics_joint_scripted::SetAnchor))
			.def("set_anchor_vs_first_element",			(void(cphysics_joint_scripted::*)(const float,const float,const float))(&cphysics_joint_scripted::SetAnchorVsFirstElement))
			.def("set_anchor_vs_second_element",		(void(cphysics_joint_scripted::*)(const float,const float,const float))(&cphysics_joint_scripted::SetAnchorVsSecondElement))
			.def("get_axes_number",						&cphysics_joint_scripted::GetAxesNumber)
			.def("set_axis_spring_dumping_factors",		&cphysics_joint_scripted::SetAxisSDfactors)
			.def("set_joint_spring_dumping_factors",	&cphysics_joint_scripted::SetJointSDfactors)
			.def("set_axis_dir_global",					(void(cphysics_joint_scripted::*)(const float,const float,const float,const int ))(&cphysics_joint_scripted::SetAxisDir))
			.def("set_axis_dir_vs_first_element",		(void(cphysics_joint_scripted::*)(const float,const float,const float,const int ))(&cphysics_joint_scripted::SetAxisDirVsFirstElement))
			.def("set_axis_dir_vs_second_element",		(void(cphysics_joint_scripted::*)(const float,const float,const float,const int ))(&cphysics_joint_scripted::SetAxisDirVsSecondElement))
			.def("set_limits",							&cphysics_joint_scripted::SetLimits)
			.def("set_max_force_and_velocity",			&cphysics_joint_scripted::SetForceAndVelocity)
			.def("get_max_force_and_velocity",			&cphysics_joint_scripted::GetMaxForceAndVelocity)
			.def("get_axis_angle",						&cphysics_joint_scripted::GetAxisAngle)
			.def("get_limits",							&cphysics_joint_scripted::GetLimits,out_value(_2) + out_value(_3))
			.def("get_axis_dir",						&cphysics_joint_scripted::GetAxisDirDynamic)
			.def("get_anchor",							&cphysics_joint_scripted::GetAnchorDynamic)
			.def("is_breakable",						&cphysics_joint_scripted::isBreakable)
		];
}