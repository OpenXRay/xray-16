///////////////////////////////////////////////////////////////
// GrenadeLauncher.cpp
// GrenadeLauncher - апгрейд оружия поствольный гранатомет
///////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "GrenadeLauncher.h"

CGrenadeLauncher::CGrenadeLauncher() { m_fGrenadeVel = 0.f; }
CGrenadeLauncher::~CGrenadeLauncher() {}
BOOL CGrenadeLauncher::net_Spawn(CSE_Abstract* DC) { return (inherited::net_Spawn(DC)); }
void CGrenadeLauncher::Load(LPCSTR section)
{
    m_fGrenadeVel = pSettings->r_float(section, "grenade_vel");
    inherited::Load(section);
}

void CGrenadeLauncher::net_Destroy() { inherited::net_Destroy(); }
void CGrenadeLauncher::UpdateCL() { inherited::UpdateCL(); }
void CGrenadeLauncher::OnH_A_Chield() { inherited::OnH_A_Chield(); }
void CGrenadeLauncher::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);
}

void CGrenadeLauncher::renderable_Render() { inherited::renderable_Render(); }
