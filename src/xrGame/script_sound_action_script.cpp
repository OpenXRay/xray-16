////////////////////////////////////////////////////////////////////////////
//	Module 		: script_sound_action_script.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script sound action class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_sound_action.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CScriptSoundAction, (), {
    module(luaState)[class_<CScriptSoundAction>("sound")
                         .enum_("type")[value("idle", int(MonsterSound::eMonsterSoundIdle)),
                             value("eat", int(MonsterSound::eMonsterSoundEat)),
                             value("attack", int(MonsterSound::eMonsterSoundAggressive)),
                             value("attack_hit", int(MonsterSound::eMonsterSoundAttackHit)),
                             value("take_damage", int(MonsterSound::eMonsterSoundTakeDamage)),
                             value("die", int(MonsterSound::eMonsterSoundDie)),
                             value("threaten", int(MonsterSound::eMonsterSoundThreaten)),
                             value("steal", int(MonsterSound::eMonsterSoundSteal)),
                             value("panic", int(MonsterSound::eMonsterSoundPanic))]

                         .def(constructor<>())
                         .def(constructor<const char*, const char*>())
                         .def(constructor<const char*, const char*, const Fvector&>())
                         .def(constructor<const char*, const char*, const Fvector&, const Fvector&>())
                         .def(constructor<const char*, const char*, const Fvector&, const Fvector&, bool>())
                         .def(constructor<const char*, Fvector*>())
                         .def(constructor<const char*, Fvector*, const Fvector&>())
                         .def(constructor<const char*, Fvector*, const Fvector&, bool>())
                         .def(constructor<CScriptSound*, const char*, const Fvector&>())
                         .def(constructor<CScriptSound*, const char*, const Fvector&, const Fvector&>())
                         .def(constructor<CScriptSound*, const char*, const Fvector&, const Fvector&, bool>())
                         .def(constructor<CScriptSound*, Fvector*>())
                         .def(constructor<CScriptSound*, Fvector*, const Fvector&>())
                         .def(constructor<CScriptSound*, Fvector*, const Fvector&, bool>())
                         // monster specific
                         .def(constructor<MonsterSound::EType>())
                         .def(constructor<MonsterSound::EType, int>())
                         // trader specific
                         .def(constructor<const char*, const char*, MonsterSpace::EMonsterHeadAnimType>())

                         .def("set_sound", (void (CScriptSoundAction::*)(const char*))(&CScriptSoundAction::SetSound))
                         .def("set_sound",
                             (void (CScriptSoundAction::*)(const CScriptSound&))(&CScriptSoundAction::SetSound))
                         .def("set_sound_type", &CScriptSoundAction::SetSoundType)
                         .def("set_bone", &CScriptSoundAction::SetBone)
                         .def("set_position", &CScriptSoundAction::SetPosition)
                         .def("set_angles", &CScriptSoundAction::SetAngles)
                         .def("completed", (bool (CScriptSoundAction::*)())(&CScriptSoundAction::completed))];
});
