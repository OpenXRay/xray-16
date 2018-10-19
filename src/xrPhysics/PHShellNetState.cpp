#include "StdAfx.h"
#include "PhysicsShell.h"
#include "PHInterpolation.h"
#include "PHObject.h"
#include "PHWorld.h"
#include "PHShell.h"

void CPHShell::net_Import(NET_Packet& P)
{
    auto i = elements.begin(), e = elements.end();
    for (; i != e; ++i)
    {
        (*i)->net_Import(P);
    }
}

void CPHShell::net_Export(NET_Packet& P)
{
    auto i = elements.begin(), e = elements.end();
    for (; i != e; ++i)
    {
        (*i)->net_Export(P);
    }
}
