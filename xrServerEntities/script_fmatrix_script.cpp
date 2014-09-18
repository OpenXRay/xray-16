////////////////////////////////////////////////////////////////////////////
//	Module 		: script_fmatrix_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float matrix script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_fmatrix.h"

using namespace luabind;
void get_matrix_hpb(Fmatrix* self, float* h, float* p, float* b)
{
	self->getHPB	(*h, *p, *b);
}
void matrix_transform (Fmatrix* self, Fvector* v)
{
	self->transform (*v);
}

#pragma optimize("s",on)
void CScriptFmatrix::script_register(lua_State *L)
{
	module(L)
	[
		class_<Fmatrix>("matrix")
			.def_readwrite("i",					&Fmatrix::i)
			.def_readwrite("_14_",				&Fmatrix::_14_)
			.def_readwrite("j",					&Fmatrix::j)
			.def_readwrite("_24_",				&Fmatrix::_24_)
			.def_readwrite("k",					&Fmatrix::k)
			.def_readwrite("_34_",				&Fmatrix::_34_)
			.def_readwrite("c",					&Fmatrix::c)
			.def_readwrite("_44_",				&Fmatrix::_44_)
			.def(								constructor<>())
			.def("set",							(Fmatrix & (Fmatrix::*)(const Fmatrix &))(&Fmatrix::set),																return_reference_to(_1))
			.def("set",							(Fmatrix & (Fmatrix::*)(const Fvector &, const Fvector &, const Fvector &, const Fvector &))(&Fmatrix::set),				return_reference_to(_1))
			.def("identity",					&Fmatrix::identity,																										return_reference_to(_1))
			.def("mk_xform",					&Fmatrix::mk_xform,																										return_reference_to(_1))
			.def("mul",							(Fmatrix & (Fmatrix::*)(const Fmatrix &, const Fmatrix &))(&Fmatrix::mul),												return_reference_to(_1))
			.def("mul",							(Fmatrix & (Fmatrix::*)(const Fmatrix &, float))(&Fmatrix::mul),															return_reference_to(_1))
			.def("mul",							(Fmatrix & (Fmatrix::*)(float))(&Fmatrix::mul),																			return_reference_to(_1))
			.def("div",							(Fmatrix & (Fmatrix::*)(const Fmatrix &, float))(&Fmatrix::div),															return_reference_to(_1))
			.def("div",							(Fmatrix & (Fmatrix::*)(float))(&Fmatrix::div),																			return_reference_to(_1))
//			.def("invert",						(Fmatrix & (Fmatrix::*)())(&Fmatrix::invert),																			return_reference_to(_1))
//			.def("invert",						(Fmatrix & (Fmatrix::*)(const Fmatrix &))(&Fmatrix::invert),																return_reference_to(_1))
//			.def("transpose",					(Fmatrix & (Fmatrix::*)())(&Fmatrix::transpose),																			return_reference_to(_1))
//			.def("transpose",					(Fmatrix & (Fmatrix::*)(const Fmatrix &))(&Fmatrix::transpose),															return_reference_to(_1))
//			.def("translate",					(Fmatrix & (Fmatrix::*)(const Fvector &))(&Fmatrix::translate),															return_reference_to(_1))
//			.def("translate",					(Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::translate),														return_reference_to(_1))
//			.def("translate_over",				(Fmatrix & (Fmatrix::*)(const Fvector &))(&Fmatrix::translate_over),														return_reference_to(_1))
//			.def("translate_over",				(Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::translate_over),													return_reference_to(_1))
//			.def("translate_add",				&Fmatrix::translate_add,																								return_reference_to(_1))
//			.def("scale",						(Fmatrix & (Fmatrix::*)(const Fvector &))(&Fmatrix::scale),																return_reference_to(_1))
//			.def("scale",						(Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::scale),															return_reference_to(_1))
//			.def("rotateX",						&Fmatrix::rotateX,																										return_reference_to(_1))
//			.def("rotateY",						&Fmatrix::rotateY,																										return_reference_to(_1))
//			.def("rotateZ",						&Fmatrix::rotateZ,																										return_reference_to(_1))
//			.def("rotation",					(Fmatrix & (Fmatrix::*)(const Fvector &, const Fvector &))(&Fmatrix::rotation),											return_reference_to(_1))
//			.def("rotation",					(Fmatrix & (Fmatrix::*)(const Fvector &, float))(&Fmatrix::rotation),													return_reference_to(_1))
//			.def("rotation",					&Fmatrix::rotation,																										return_reference_to(_1))
/*
			.def("mapXYZ",						&Fmatrix::mapXYZ,																										return_reference_to(_1))
			.def("mapXZY",						&Fmatrix::mapXZY,																										return_reference_to(_1))
			.def("mapYXZ",						&Fmatrix::mapYXZ,																										return_reference_to(_1))
			.def("mapYZX",						&Fmatrix::mapYZX,																										return_reference_to(_1))
			.def("mapZXY",						&Fmatrix::mapZXY,																										return_reference_to(_1))
			.def("mapZYX",						&Fmatrix::mapZYX,																										return_reference_to(_1))
			.def("mirrorX",						&Fmatrix::mirrorX,																										return_reference_to(_1))
			.def("mirrorX_over",				&Fmatrix::mirrorX_over,																									return_reference_to(_1))
			.def("mirrorX_add ",				&Fmatrix::mirrorX_add,																									return_reference_to(_1))
			.def("mirrorY",						&Fmatrix::mirrorY,																										return_reference_to(_1))
			.def("mirrorY_over",				&Fmatrix::mirrorY_over,																									return_reference_to(_1))
			.def("mirrorY_add ",				&Fmatrix::mirrorY_add,																									return_reference_to(_1))
			.def("mirrorZ",						&Fmatrix::mirrorZ,																										return_reference_to(_1))
			.def("mirrorZ_over",				&Fmatrix::mirrorZ_over,																									return_reference_to(_1))
			.def("mirrorZ_add ",				&Fmatrix::mirrorZ_add,																									return_reference_to(_1))
*/
//			.def("build_projection",			&Fmatrix::build_projection,																								return_reference_to(_1))
//			.def("build_projection_HAT",		&Fmatrix::build_projection_HAT,																							return_reference_to(_1))
//			.def("build_projection_ortho",		&Fmatrix::build_projection_ortho,																						return_reference_to(_1))
//			.def("build_camera",				&Fmatrix::build_camera,																									return_reference_to(_1))
//			.def("build_camera_dir",			&Fmatrix::build_camera_dir,																								return_reference_to(_1))
//			.def("inertion",					&Fmatrix::inertion,																										return_reference_to(_1))
//			.def("transform_tiny32",			&Fmatrix::transform_tiny32)
//			.def("transform_tiny23",			&Fmatrix::transform_tiny23)
//			.def("transform_tiny",				(void	   (Fmatrix::*)(Fvector &) const)(&Fmatrix::transform_tiny),																					out_value(_2))
//			.def("transform_tiny",				(void	   (Fmatrix::*)(Fvector &, const Fvector &) const)(&Fmatrix::transform_tiny),																out_value(_2))
//			.def("transform_dir",				(void	   (Fmatrix::*)(Fvector &) const)(&Fmatrix::transform_dir),																					out_value(_2))
//			.def("transform_dir",				(void	   (Fmatrix::*)(Fvector &, const Fvector &) const)(&Fmatrix::transform_dir),																	out_value(_2))
//			.def("transform",					(void	   (Fmatrix::*)(Fvector &) const)(&Fmatrix::transform),																						out_value(_2))
//			.def("transform",					&matrix_transform)
			.def("setHPB",						&Fmatrix::setHPB,																										return_reference_to(_1))
//			.def("setXYZ",						(Fmatrix & (Fmatrix::*)(Fvector &))(&Fmatrix::setXYZ),																	return_reference_to(_1)	+	out_value(_2))
			.def("setXYZ",						(Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::setXYZ),															return_reference_to(_1))	 
//			.def("setXYZi",						(Fmatrix & (Fmatrix::*)(Fvector &))(&Fmatrix::setXYZi),																	return_reference_to(_1) +	out_value(_2))
			.def("setXYZi",						(Fmatrix & (Fmatrix::*)(float, float, float))(&Fmatrix::setXYZi),														return_reference_to(_1))
//			.def("getHPB",						(void	   (Fmatrix::*)(Fvector &) const)(&Fmatrix::getHPB),																							out_value(_2))
			.def("getHPB",						&get_matrix_hpb)
//			.def("getXYZ",						(void	   (Fmatrix::*)(Fvector &) const)(&Fmatrix::getXYZ),																							out_value(_2))
//			.def("getXYZ",						(void	   (Fmatrix::*)(float &, float &, float &) const)(&Fmatrix::getXYZ))
//			.def("getXYZi",						(void	   (Fmatrix::*)(Fvector &) const)(&Fmatrix::getXYZi),																						out_value(_2))
//			.def("getXYZi",						(void	   (Fmatrix::*)(float &, float &, float &) const)(&Fmatrix::getXYZi))
	];
}
