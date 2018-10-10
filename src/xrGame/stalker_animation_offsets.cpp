////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_offsets.cpp
//	Created 	: 19.12.2007
//	Author		: Alexander Dudin
//	Description : Animation offsets class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_animation_offsets.hpp"

SRotation const animation_offsets::offsets(shared_str const& animation_id) const
{
    AnimationOffsets::const_iterator found = m_animation_offsets.find(animation_id);
    if (found != m_animation_offsets.end())
        return found->second;

    return (SRotation());
}

void animation_offsets::load(LPCSTR section)
{
    CInifile::Sect& anim_section = pSettings->r_section(section);
    CInifile::SectIt_ i = anim_section.Data.begin();
    CInifile::SectIt_ e = anim_section.Data.end();
    for (; i != e; ++i)
    {
        string256 tmp;

        shared_str anim_id = i->first;
        LPCSTR data = i->second.c_str();
        float yaw = deg2rad((float)atof(_GetItem(data, 0, tmp)));
        float pitch = deg2rad((float)atof(_GetItem(data, 1, tmp)));

        m_animation_offsets.insert(std::make_pair(anim_id, SRotation(yaw, pitch, 0.f)));
    }
}
