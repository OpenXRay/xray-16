#include "pch_script.h"
#include "WeaponWalther.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponWalther::CWeaponWalther(void) {}
CWeaponWalther::~CWeaponWalther(void) {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponWalther, (CGameObject),
    { module(luaState)[class_<CWeaponWalther, CGameObject>("CWeaponWalther").def(constructor<>())]; });
