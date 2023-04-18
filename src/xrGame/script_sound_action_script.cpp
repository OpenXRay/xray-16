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

SCRIPT_EXPORT(CScriptSoundAction, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CScriptSoundAction>("sound")
            .enum_("type")
            [
                value("idle", int(MonsterSound::eMonsterSoundIdle)),
                value("eat", int(MonsterSound::eMonsterSoundEat)),
                value("attack", int(MonsterSound::eMonsterSoundAggressive)),
                value("attack_hit", int(MonsterSound::eMonsterSoundAttackHit)),
                value("take_damage", int(MonsterSound::eMonsterSoundTakeDamage)),
                value("die", int(MonsterSound::eMonsterSoundDie)),
                value("threaten", int(MonsterSound::eMonsterSoundThreaten)),
                value("steal", int(MonsterSound::eMonsterSoundSteal)),
                value("panic", int(MonsterSound::eMonsterSoundPanic))
            ]

            .def(constructor<>())
            .def(constructor<pcstr, pcstr>())
            .def(constructor<pcstr, pcstr, const Fvector&>())
            .def(constructor<pcstr, pcstr, const Fvector&, const Fvector&>())
            .def(constructor<pcstr, pcstr, const Fvector&, const Fvector&, bool>())
            .def(constructor<pcstr, Fvector*>())
            .def(constructor<pcstr, Fvector*, const Fvector&>())
            .def(constructor<pcstr, Fvector*, const Fvector&, bool>())
            .def(constructor<CScriptSound*, pcstr, const Fvector&>())
            .def(constructor<CScriptSound*, pcstr, const Fvector&, const Fvector&>())
            .def(constructor<CScriptSound*, pcstr, const Fvector&, const Fvector&, bool>())
            .def(constructor<CScriptSound*, Fvector*>())
            .def(constructor<CScriptSound*, Fvector*, const Fvector&>())
            .def(constructor<CScriptSound*, Fvector*, const Fvector&, bool>())
            // monster specific
            .def(constructor<MonsterSound::EType>())
            .def(constructor<MonsterSound::EType, int>())
            // trader specific
            .def(constructor<pcstr, pcstr, MonsterSpace::EMonsterHeadAnimType>())

            .def("set_sound", (void (CScriptSoundAction::*)(pcstr))(&CScriptSoundAction::SetSound))
            .def("set_sound",
                (void (CScriptSoundAction::*)(const CScriptSound&))(&CScriptSoundAction::SetSound))
            .def("set_sound_type", &CScriptSoundAction::SetSoundType)
            .def("set_bone", &CScriptSoundAction::SetBone)
            .def("set_position", &CScriptSoundAction::SetPosition)
            .def("set_angles", &CScriptSoundAction::SetAngles)
            .def("completed", &CScriptAbstractAction::completed)
    ];
});
