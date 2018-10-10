#include "pch_script.h"
#include "script_monster_hit_info.h"
#include "script_game_object.h"
#include "ai_monster_space.h"
#include "ai/monsters/monster_sound_defs.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

struct CMonsterSpace
{
};

SCRIPT_EXPORT(CScriptMonsterHitInfo, (), {
    module(luaState)[class_<CScriptMonsterHitInfo>("MonsterHitInfo")
                         .def_readwrite("who", &CScriptMonsterHitInfo::who)
                         .def_readwrite("direction", &CScriptMonsterHitInfo::direction)
                         .def_readwrite("time", &CScriptMonsterHitInfo::time),

        class_<CMonsterSpace>("MonsterSpace")
            .enum_("sounds")[value("sound_script", MonsterSound::eMonsterSoundScript)]

            .enum_("head_anim")[value("head_anim_normal", MonsterSpace::eHeadAnimNormal),
                value("head_anim_angry", MonsterSpace::eHeadAnimAngry),
                value("head_anim_glad", MonsterSpace::eHeadAnimGlad),
                value("head_anim_kind", MonsterSpace::eHeadAnimKind)]];
});
