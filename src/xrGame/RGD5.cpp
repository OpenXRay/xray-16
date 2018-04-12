#include "pch_script.h"
#include "rgd5.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CRGD5::CRGD5(void) {}
CRGD5::~CRGD5(void) {}
using namespace luabind;

SCRIPT_EXPORT(CRGD5, (CGameObject, CExplosive),
    { module(luaState)[class_<CRGD5, bases<CGameObject, CExplosive>>("CRGD5").def(constructor<>())]; });
