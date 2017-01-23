#include "StdAfx.h"
#include "Util.hpp"
#include "Globals.hpp"

EChannelType GetChannelType(LWChannelID chan)
{
    const char* chname = g_chinfo->channelName(chan);
    if (xr_strcmp(chname, "Position.X") == 0)
        return ctPositionX;
    if (xr_strcmp(chname, "Position.Y") == 0)
        return ctPositionY;
    if (xr_strcmp(chname, "Position.Z") == 0)
        return ctPositionZ;
    if (xr_strcmp(chname, "Rotation.H") == 0)
        return ctRotationH;
    if (xr_strcmp(chname, "Rotation.P") == 0)
        return ctRotationP;
    if (xr_strcmp(chname, "Rotation.B") == 0)
        return ctRotationB;
    return ctUnsupported;
}

CEnvelope* CreateEnvelope(LWChannelID chan, LWChannelID* chan_parent /*= nullptr*/)
{
    LWChanGroupID group, group_parent;
    LWEnvelopeID lwenv, lwenv_parent;
    LWEnvKeyframeID lwkey, lwkey_parent;
    CEnvelope *env;
    st_Key *key, *tail = NULL;
    double val;
    env = new CEnvelope();
    group = g_chinfo->channelParent(chan);
    lwenv = g_chinfo->channelEnvelope(chan);
    lwkey = NULL;
    if (chan_parent)
    {
        group_parent = g_chinfo->channelParent(*chan_parent);
        lwenv_parent = g_chinfo->channelEnvelope(*chan_parent);
        lwkey_parent = NULL;
    }
    g_envf->egGet(lwenv, group, LWENVTAG_PREBEHAVE, &env->behavior[0]);
    g_envf->egGet(lwenv, group, LWENVTAG_POSTBEHAVE, &env->behavior[1]);
    float val_parent = 0;
    if (chan_parent)
    {
        lwkey_parent = g_envf->nextKey(lwenv_parent, lwkey_parent);
        if (lwkey_parent)
        {
            g_envf->keyGet(lwenv_parent, lwkey_parent, LWKEY_VALUE, &val);
            val_parent = (float)val;
        }
    }
    while (lwkey = g_envf->nextKey(lwenv, lwkey))
    {
        key = new st_Key();
        env->keys.push_back(key);
        g_envf->keyGet(lwenv, lwkey, LWKEY_SHAPE, &key->shape);
        g_envf->keyGet(lwenv, lwkey, LWKEY_VALUE, &val);		key->value = (float)val + val_parent;
        g_envf->keyGet(lwenv, lwkey, LWKEY_TIME, &val);		key->time = (float)val;
        g_envf->keyGet(lwenv, lwkey, LWKEY_TENSION, &val);	key->tension = (float)val;
        g_envf->keyGet(lwenv, lwkey, LWKEY_CONTINUITY, &val);	key->continuity = (float)val;
        g_envf->keyGet(lwenv, lwkey, LWKEY_BIAS, &val);		key->bias = (float)val;
        g_envf->keyGet(lwenv, lwkey, LWKEY_PARAM_0, &val);	key->param[0] = (float)val;
        g_envf->keyGet(lwenv, lwkey, LWKEY_PARAM_1, &val);	key->param[1] = (float)val;
        g_envf->keyGet(lwenv, lwkey, LWKEY_PARAM_2, &val);	key->param[2] = (float)val;
        g_envf->keyGet(lwenv, lwkey, LWKEY_PARAM_3, &val);	key->param[3] = (float)val;
    }
    return env;
}
