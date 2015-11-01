#include "pch_script.h"
#include "weaponsvd.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponSVD::CWeaponSVD(void)
{}

CWeaponSVD::~CWeaponSVD(void)
{}

void CWeaponSVD::switch2_Fire	()
{
	m_bFireSingleShot			= true;
	bWorking					= false;
	SetPending					(TRUE);
	m_iShotNum					= 0;
	m_bStopedAfterQueueFired	= false;

}

void CWeaponSVD::OnAnimationEnd(u32 state) 
{
	switch(state) 
	{
	case eFire:	{
		SetPending			(FALSE);
		}break;	// End of reload animation
	}
	inherited::OnAnimationEnd(state);
}

using namespace luabind;

SCRIPT_EXPORT(CWeaponSVD, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponSVD,CGameObject>("CWeaponSVD")
			.def(constructor<>())
	];
});
