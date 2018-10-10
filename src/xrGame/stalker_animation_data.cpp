////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_data.cpp
//	Created 	: 13.10.2005
//  Modified 	: 13.10.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation data
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_animation_data.h"

CStalkerAnimationData::CStalkerAnimationData(IKinematicsAnimated* skeleton_animated)
{
    m_part_animations.Load(skeleton_animated, "");
    m_head_animations.Load(skeleton_animated, "");
    m_global_animations.Load(skeleton_animated, "item_");
}
