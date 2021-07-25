#include "stdafx.h"

// XXX: remove this file

// *****************************************************************************************
// Error handling

//----------------------------- FLAGS
static struct _DF
{
    pcstr name;
    u32 mask;
} DF[] = {{"rsClearBB", rsClearBB}, {"rsVSync", rsVSync}, {"rsWireframe", rsWireframe},
    {nullptr, 0}};

void CRenderDevice::DumpFlags()
{
    Log("- Dumping device flags");
    _DF* p = DF;
    while (p->name)
    {
        Msg("* %20s %s", p->name, psDeviceFlags.test(p->mask) ? "on" : "off");
        p++;
    }
}
