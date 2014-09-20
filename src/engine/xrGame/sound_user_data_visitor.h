////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_user_data_visitor.h
//	Created 	: 27.01.2005
//  Modified 	: 27.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Sound user data visitor
////////////////////////////////////////////////////////////////////////////

#pragma once

class CStalkerSoundData;

class CSound_UserDataVisitor {
public:
	virtual			~CSound_UserDataVisitor	()							{};
	virtual void	visit					(CSound_UserData *data)		{};
	virtual void	visit					(CStalkerSoundData *data)	{};
};
