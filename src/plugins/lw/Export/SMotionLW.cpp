#include "StdAfx.h"
#include "SMotionLW.hpp"
#include "Globals.hpp"
#include "Util.hpp"

// XXX: locate and include
extern void ReplaceSpaceAndLowerCase(shared_str& s);

void SMotionLW::Save(IWriter& writer)
{
    ReplaceSpaceAndLowerCase(name);
    CSMotion::Save(writer);
}

void SMotionLW::ParseBoneMotion(LWItemID bone)
{
    LWChanGroupID group, group_goal, group_goal_parent;
    LWChannelID chan, chan_goal, chan_goal_parent;
    LWItemID goal, goal_parent;

    bone_mots.push_back(st_BoneMotion());
    st_BoneMotion&	bm = bone_mots.back();

    group = g_iteminfo->chanGroup(bone);
    chan = g_chinfo->nextChannel(group, NULL);
    bm.SetName(g_iteminfo->name(bone));

    goal = g_iteminfo->goal(bone);
    LPCSTR goal_name = g_iteminfo->name(goal);

    goal_parent = g_iteminfo->parent(goal);
    bool bParent = !(goal_parent == LWITEM_NULL);
    bool bGoalOrient = !!(g_iteminfo->flags(bone)&LWITEMF_GOAL_ORIENT);
    if (bGoalOrient)
    {
        group_goal = g_iteminfo->chanGroup(goal);
        chan_goal = g_chinfo->nextChannel(group_goal, NULL);
        // flag
        bm.m_Flags.set(st_BoneMotion::flWorldOrient, TRUE);
    }

    while (chan)
    {
        EChannelType t = GetChannelType(chan);
        if (t != ctUnsupported)
        {
            if (!bGoalOrient || (bGoalOrient && ((t == ctPositionX) || (t == ctPositionY) || (t == ctPositionZ))))
            {
                CEnvelope* env = CreateEnvelope(chan);
                bm.envs[t] = env;
            }
        }
        chan = g_chinfo->nextChannel(group, chan);
    }
    // goal orientation
    if (bGoalOrient)
    {
        while (chan_goal)
        {
            EChannelType t = GetChannelType(chan_goal);
            if (t != ctUnsupported)
            {
                if ((t == ctRotationH) || (t == ctRotationP) || (t == ctRotationB))
                {
                    // parent (if exist)
                    if (bParent)
                    {
                        group_goal_parent = g_iteminfo->chanGroup(goal_parent);
                        chan_goal_parent = g_chinfo->nextChannel(group_goal_parent, NULL);
                        while (chan_goal_parent)
                        {
                            if (t == GetChannelType(chan_goal_parent)) break;
                            chan_goal_parent = g_chinfo->nextChannel(group_goal_parent, chan_goal_parent);
                        }
                    }
                    CEnvelope* env = CreateEnvelope(chan_goal, bParent ? &chan_goal_parent : 0);
                    bm.envs[t] = env;
                }
            }
            chan_goal = g_chinfo->nextChannel(group_goal, chan_goal);
        }
    }
}
