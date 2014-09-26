////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_sound_data_visitor.h
//	Created 	: 02.02.2005
//  Modified 	: 02.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker sound data visitor
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "sound_user_data_visitor.h"

class CAI_Stalker;
class CStalkerSoundData;

class CStalkerSoundDataVisitor : public CSound_UserDataVisitor {
private:
	CAI_Stalker			*m_object;

public:
	IC					CStalkerSoundDataVisitor	(CAI_Stalker *object);
	virtual				~CStalkerSoundDataVisitor	();
	virtual void		visit						(CStalkerSoundData *data);
	IC		CAI_Stalker	&object						() const;
};

#include "stalker_sound_data_visitor_inline.h"