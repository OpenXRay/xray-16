#include "pch_script.h"
#include "WeaponSVU.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponSVU::CWeaponSVU(void) {}
CWeaponSVU::~CWeaponSVU(void) {}
using namespace luabind;

SCRIPT_EXPORT(CWeaponSVU, (CGameObject),
    { module(luaState)[class_<CWeaponSVU, CGameObject>("CWeaponSVU").def(constructor<>())]; });
