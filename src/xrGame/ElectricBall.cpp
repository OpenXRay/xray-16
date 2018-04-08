///////////////////////////////////////////////////////////////
// ElectricBall.cpp
// ElectricBall - артефакт электрический шар
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ElectricBall.h"
#include "xrPhysics/PhysicsShell.h"

CElectricBall::CElectricBall(void) {}
CElectricBall::~CElectricBall(void) {}
void CElectricBall::Load(LPCSTR section) { inherited::Load(section); }
void CElectricBall::UpdateCLChild()
{
    inherited::UpdateCLChild();

    if (H_Parent())
        XFORM().set(H_Parent()->XFORM());
}

void CElectricBall::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);
    //P.w_float_q8(GetCondition(), 0.0f, 1.0f);
}

void CElectricBall::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);
    /*float _cond;
    P.r_float_q8(_cond, 0.0f, 1.0f);
    SetCondition(_cond);*/
}
