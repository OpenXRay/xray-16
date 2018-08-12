#include "pch_script.h"
#include "Torch.h"
#include "PDA.h"
#include "SimpleDetector.h"
#include "EliteDetector.h"
#include "AdvancedDetector.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CTorch, (CGameObject), {
    module(luaState)[class_<CTorch, CGameObject>("CTorch").def(constructor<>()),
        class_<CPda, CGameObject>("CPda").def(constructor<>()),
        class_<CScientificDetector, CGameObject>("CScientificDetector").def(constructor<>()),
        class_<CEliteDetector, CGameObject>("CEliteDetector").def(constructor<>()),
        class_<CAdvancedDetector, CGameObject>("CAdvancedDetector").def(constructor<>()),
        class_<CSimpleDetector, CGameObject>("CSimpleDetector").def(constructor<>())];
});
