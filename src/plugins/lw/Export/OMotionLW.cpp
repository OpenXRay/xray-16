#include "OMotionLW.hpp"
#include "Globals.hpp"
#include "StdAfx.h"
#include "Util.hpp"

void OMotionLW::ParseObjectMotion(LWItemID object)
{
    LWChanGroupID group;
    LWChannelID chan;
    group = g_iteminfo->chanGroup(object);
    chan = g_chinfo->nextChannel(group, NULL);
    while (chan)
    {
        EChannelType t = GetChannelType(chan);
        if (t != ctUnsupported) {
            CEnvelope* env = CreateEnvelope(chan);
            envs[t] = env;
        }
        chan = g_chinfo->nextChannel(group, chan);
    }
}
