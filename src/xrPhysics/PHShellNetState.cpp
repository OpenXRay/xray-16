#include "stdafx.h"
#include "physicsshell.h"
#include "phinterpolation.h"
#include "phobject.h"
#include "phworld.h"
#include "phshell.h"

void CPHShell::net_Import(NET_Packet& P)
{
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(;i!=e;++i)
	{
		(*i)->net_Import(P);
	}	
}

void CPHShell::net_Export(NET_Packet& P)
{
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(;i!=e;++i)
	{
		(*i)->net_Export(P);
	}	
}