////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_monsters_anims.h
//	Created 	: 23.05.2003
//  Modified 	: 23.05.2003
//	Author		: Serge Zhem
//	Description : Animation templates for all of the monsters
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../Include/xrRender/KinematicsAnimated.h"
#include "../ai_debug.h"

DEFINE_VECTOR	(MotionID,ANIM_VECTOR, ANIM_IT);

class CAniVector {
public:
	ANIM_VECTOR		A;

			void	Load	(IKinematicsAnimated *tpKinematics, LPCSTR caBaseName);
};

template <LPCSTR caBaseNames[]> class CAniFVector {
public:
	ANIM_VECTOR		A;

	IC	void		Load(IKinematicsAnimated *tpKinematics, LPCSTR caBaseName)
	{
		A.clear			();
		string256		S;
		for (int j=0; caBaseNames[j]; ++j);
		A.resize		(j);
		for (int i=0; i<j; ++i) 
		{
			strconcat	(sizeof(S),S,caBaseName,caBaseNames[i]);
			A[i]		= tpKinematics->ID_Cycle_Safe(S);
#ifdef DEBUG
			if (A[i] && psAI_Flags.test(aiAnimation))
				Msg		("* Loaded animation %s",S);
#endif
		}
	}
};

template <class TYPE_NAME, LPCSTR caBaseNames[]> class CAniCollection {
public:
	xr_vector<TYPE_NAME>	A;

	IC	void		Load(IKinematicsAnimated *tpKinematics, LPCSTR caBaseName)
	{
		A.clear		();
		string256	S;
		for (int j=0; caBaseNames[j]; ++j);
		A.resize	(j);
		for (int i=0; i<j; ++i)
			A[i].Load	(tpKinematics,strconcat(sizeof(S),S,caBaseName,caBaseNames[i]));
	}
};