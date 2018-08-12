////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_sound_data.cpp
//	Created 	: 02.02.2005
//  Modified 	: 02.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker sound data
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_sound_data.h"
#include "sound_user_data_visitor.h"
#include "ai/stalker/ai_stalker.h"

CStalkerSoundData::~CStalkerSoundData() {}
void CStalkerSoundData::accept(CSound_UserDataVisitor* visitor)
{
    if (!m_object || m_object->getDestroy())
        return;

    visitor->visit(this);
}

void CStalkerSoundData::invalidate() { m_object = 0; }
