///////////////////////////////////////////////////////////////
// Silencer.cpp
// Silencer - апгрейд оружия глушитель
///////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Silencer.h"

CSilencer::CSilencer() {}
CSilencer::~CSilencer() {}
bool CSilencer::net_Spawn(CSE_Abstract* DC) { return (inherited::net_Spawn(DC)); }
void CSilencer::Load(LPCSTR section) { inherited::Load(section); }
void CSilencer::net_Destroy() { inherited::net_Destroy(); }
void CSilencer::UpdateCL() { inherited::UpdateCL(); }
void CSilencer::OnH_A_Chield() { inherited::OnH_A_Chield(); }
void CSilencer::OnH_B_Independent(bool just_before_destroy) { inherited::OnH_B_Independent(just_before_destroy); }
