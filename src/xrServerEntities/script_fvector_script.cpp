////////////////////////////////////////////////////////////////////////////
//	Module 		: script_fvector_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float vector script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(Fvector, (),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<Fvector>("vector")
            .def_readwrite("x", &Fvector::x)
            .def_readwrite("y", &Fvector::y)
            .def_readwrite("z", &Fvector::z)
            .def(constructor<>())
            .def("set", (Fvector & (Fvector::*)(float, float, float))(&Fvector::set), return_reference_to<1>())
            .def("set", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::set), return_reference_to<1>())
            .def("add", (Fvector & (Fvector::*)(float))(&Fvector::add), return_reference_to<1>())
            .def("add", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::add), return_reference_to<1>())
            .def("add", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::add),
                return_reference_to<1>())
            .def("add", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::add), return_reference_to<1>())
            .def("sub", (Fvector & (Fvector::*)(float))(&Fvector::sub), return_reference_to<1>())
            .def("sub", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::sub), return_reference_to<1>())
            .def("sub", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::sub),
                return_reference_to<1>())
            .def("sub", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::sub), return_reference_to<1>())
            .def("mul", (Fvector & (Fvector::*)(float))(&Fvector::mul), return_reference_to<1>())
            .def("mul", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::mul), return_reference_to<1>())
            .def("mul", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::mul),
                return_reference_to<1>())
            .def("mul", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::mul), return_reference_to<1>())
            .def("div", (Fvector & (Fvector::*)(float))(&Fvector::div), return_reference_to<1>())
            .def("div", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::div), return_reference_to<1>())
            .def("div", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::div),
                return_reference_to<1>())
            .def("div", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::div), return_reference_to<1>())
            .def("invert", (Fvector & (Fvector::*)())(&Fvector::invert), return_reference_to<1>())
            .def("invert", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::invert), return_reference_to<1>())
            .def("min", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::min), return_reference_to<1>())
            .def("min", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::min),
                return_reference_to<1>())
            .def("max", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::max), return_reference_to<1>())
            .def("max", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::max),
                return_reference_to<1>())
            .def("abs", &Fvector::abs, return_reference_to<1>())
            .def("similar", &Fvector::similar)
            .def("set_length", &Fvector::set_length, return_reference_to<1>())
            .def("align", &Fvector::align, return_reference_to<1>())
            //			.def("squeeze",						&Fvector::squeeze,
            // return_reference_to<1>())
            .def("clamp", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::clamp), return_reference_to<1>())
            .def("clamp", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::clamp),
                return_reference_to<1>())
            .def("inertion", &Fvector::inertion, return_reference_to<1>())
            .def("average", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::average), return_reference_to<1>())
            .def("average", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::average),
                return_reference_to<1>())
            .def("lerp", &Fvector::lerp, return_reference_to<1>())
            .def("mad", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::mad), return_reference_to<1>())
            .def("mad", (Fvector & (Fvector::*)(const Fvector&, const Fvector&, float))(&Fvector::mad),
                return_reference_to<1>())
            .def("mad", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::mad),
                return_reference_to<1>())
            .def("mad", (Fvector & (Fvector::*)(const Fvector&, const Fvector&, const Fvector&))(&Fvector::mad),
                return_reference_to<1>())
            //			.def("square_magnitude",			&Fvector::square_magnitude)
            .def("magnitude", &Fvector::magnitude)
            //			.def("normalize_magnitude",			&Fvector::normalize_magn)
            .def("normalize", (Fvector & (Fvector::*)())(&Fvector::normalize_safe), return_reference_to<1>())
            .def("normalize", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::normalize_safe),
                return_reference_to<1>())
            .def("normalize_safe", (Fvector & (Fvector::*)())(&Fvector::normalize_safe), return_reference_to<1>())
            .def("normalize_safe", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::normalize_safe),
                return_reference_to<1>())
            //			.def("random_dir",					(Fvector & (Fvector::*)())(&Fvector::random_dir),
            // return_reference_to<1>())
            //			.def("random_dir",					(Fvector & (Fvector::*)(const Fvector &,
            // float))(&Fvector::random_dir), return_reference_to<1>())
            //			.def("random_point",				(Fvector & (Fvector::*)(const Fvector
            //&))(&Fvector::random_point), return_reference_to<1>())
            //			.def("random_point",				(Fvector & (Fvector::*)(float))(&Fvector::random_point),
            // return_reference_to<1>())
            .def("dotproduct", &Fvector::dotproduct)
            .def("crossproduct", &Fvector::crossproduct, return_reference_to<1>())
            .def("distance_to_xz", &Fvector::distance_to_xz)
            .def("distance_to_sqr", &Fvector::distance_to_sqr)
            .def("distance_to", &Fvector::distance_to)
            //			.def("from_bary",					(Fvector & (Fvector::*)(const Fvector &, const Fvector
            //&, const Fvector &, float, float, float))(&Fvector::from_bary),	return_reference_to<1>())
            //			.def("from_bary",					(Fvector & (Fvector::*)(const Fvector &, const Fvector
            //&, const Fvector &, const Fvector &))(&Fvector::from_bary),		return_reference_to<1>())
            //			.def("from_bary4",					&Fvector::from_bary4,
            // return_reference_to<1>())
            //			.def("mknormal_non_normalized",		&Fvector::mknormal_non_normalized,
            // return_reference_to<1>())
            //			.def("mknormal",					&Fvector::mknormal,
            // return_reference_to<1>())
            .def("setHP", &Fvector::setHP, return_reference_to<1>())
            //			.def("getHP",						&Fvector::getHP, policy_list<out_value<2>,
            // out_value<3>>())
            .def("getH", &Fvector::getH)
            .def("getP", &Fvector::getP)

            .def("reflect", &Fvector::reflect, return_reference_to<1>())
            .def("slide", &Fvector::slide, return_reference_to<1>())
            //			.def("generate_orthonormal_basis",	&Fvector::generate_orthonormal_basis),
    ];
});

SCRIPT_EXPORT(Fvector2, (),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<Fvector2>("vector2")
            .def_readwrite("x", &Fvector2::x)
            .def_readwrite("y", &Fvector2::y)
            .def(constructor<>())
            .def("set", (Fvector2 & (Fvector2::*)(float, float))(&Fvector2::set), return_reference_to<1>())
            .def("set", (Fvector2 & (Fvector2::*)(const Fvector2&))(&Fvector2::set),
                return_reference_to<1>())
    ];
});

SCRIPT_EXPORT(Fbox, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<Fbox>("Fbox")
            .def_readwrite("min", &Fbox::vMin)
            .def_readwrite("max", &Fbox::vMax)
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(Frect, (),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<Frect>("Frect")
            .def(constructor<>())
            .def("set", (Frect& (Frect::*)(float, float, float, float))(&Frect::set), return_reference_to<1>())
            .def_readwrite("lt", &Frect::lt)
            .def_readwrite("rb", &Frect::rb)
            .def_readwrite("x1", &Frect::x1)
            .def_readwrite("x2", &Frect::x2)
            .def_readwrite("y1", &Frect::y1)
            .def_readwrite("y2", &Frect::y2)
    ];
});
