////////////////////////////////////////////////////////////////////////////
//	Module 		: script_sound_script.cpp
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script sound class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_sound.h"
#include "script_game_object.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptSound::script_register(lua_State *L)
{
	module(L)
	[
		class_<CSound_params>("sound_params")
			.def_readwrite("position",			&CSound_params::position)
			.def_readwrite("volume",			&CSound_params::volume)
			.def_readwrite("frequency",			&CSound_params::freq)
			.def_readwrite("min_distance",		&CSound_params::min_distance)
			.def_readwrite("max_distance",		&CSound_params::max_distance),
			
		class_<CScriptSound>("sound_object")
			.enum_("sound_play_type")
			[
				value("looped",					sm_Looped),
				value("s2d",					sm_2D),
				value("s3d",					0)
			]
			.property("frequency",				&CScriptSound::GetFrequency,	&CScriptSound::SetFrequency)
			.property("min_distance",			&CScriptSound::GetMinDistance,	&CScriptSound::SetMinDistance)
			.property("max_distance",			&CScriptSound::GetMaxDistance,	&CScriptSound::SetMaxDistance)
			.property("volume",					&CScriptSound::GetVolume,		&CScriptSound::SetVolume)
			.def(								constructor<LPCSTR>())
			.def(								constructor<LPCSTR,ESoundTypes>())
			.def("get_position",				&CScriptSound::GetPosition)
			.def("set_position",				&CScriptSound::SetPosition)
			.def("play",						(void (CScriptSound::*)(CScriptGameObject*))(&CScriptSound::Play))
			.def("play",						(void (CScriptSound::*)(CScriptGameObject*,float))(&CScriptSound::Play))
			.def("play",						(void (CScriptSound::*)(CScriptGameObject*,float,int))(&CScriptSound::Play))
			.def("play_at_pos",					(void (CScriptSound::*)(CScriptGameObject*,const Fvector &))(&CScriptSound::PlayAtPos))
			.def("play_at_pos",					(void (CScriptSound::*)(CScriptGameObject*,const Fvector &,float))(&CScriptSound::PlayAtPos))
			.def("play_at_pos",					(void (CScriptSound::*)(CScriptGameObject*,const Fvector &,float, int))(&CScriptSound::PlayAtPos))
			.def("play_no_feedback",			&CScriptSound::PlayNoFeedback)
			.def("stop",						&CScriptSound::Stop)
			.def("stop_deffered",				&CScriptSound::StopDeffered)
			.def("playing",						&CScriptSound::IsPlaying)
			.def("length",						&CScriptSound::Length)
			.def("attach_tail",					&CScriptSound::AttachTail)
	];
}
