
//	Module 		: ai_monsters_misc.cpp
//	Created 	: 23.07.2002
//  Modified 	: 23.07.2002
//	Author		: Dmitriy Iassenev
//	Description : Miscellanious routines for monsters
////////////////////////////////////////////////////////////////////////////

#pragma once

class CBaseFunction;
class CEntity;
class CEntityAlive;

namespace GroupHierarchyHolder {
	typedef xr_vector<CEntity*> MEMBER_REGISTRY;
};
		   
#define WRITE_LOG

#ifndef DEBUG
#undef WRITE_LOG
#endif

#ifdef WRITE_LOG
	#define WRITE_TO_LOG(S) {\
		Msg("%s,%s,%d,p[%.2f,%.2f,%.2f],%.2f,h[%.2f,%.2f],t[%.2f,%.2f]",*cName(),S,Device.dwTimeGlobal,Position().x,Position().y,Position().z,m_fCurSpeed,m_head.current.yaw,m_head.target.yaw,m_body.current.yaw,m_body.target.yaw);\
		vfUpdateDynamicObjects();\
		m_bStopThinking = true;\
	}
	#define WRITE_QUERY_TO_LOG(S) ;//Msg(S);
		//	Msg("%d",Level().Teams[g_Team()].Squads[g_Squad()].Groups[g_Group()].m_tpaSuspiciousNodes.size());\

#else
	#define WRITE_QUERY_TO_LOG(S)
	#define WRITE_TO_LOG(S) {\
		vfUpdateDynamicObjects();\
		m_bStopThinking = true;\
	}
#endif

#define GO_TO_NEW_STATE(a) {\
	m_tStateStack.top() = m_eCurrentState = a;\
	return;\
}

#define GO_TO_PREV_STATE {\
	m_tStateStack.pop();\
	m_eCurrentState = m_tStateStack.top();\
	return;\
}

#define SWITCH_TO_NEW_STATE(a) {\
	m_tStateStack.push(a);\
	GO_TO_NEW_STATE(a);\
}

#define CHECK_IF_SWITCH_TO_NEW_STATE(a,b) {\
	if (a)\
		SWITCH_TO_NEW_STATE(b);\
}

#define CHECK_IF_GO_TO_PREV_STATE(a) {\
	if (a)\
		GO_TO_PREV_STATE;\
}

#define CHECK_IF_GO_TO_NEW_STATE(a,b) {\
	if (a)\
		GO_TO_NEW_STATE(b);\
}

//////////////////////////////////////////////////////////////////////////
#define GO_TO_NEW_STATE_THIS_UPDATE(a) {\
	m_bStopThinking = false;\
	GO_TO_NEW_STATE(a);\
}

#define GO_TO_PREV_STATE_THIS_UPDATE {\
	m_bStopThinking = false;\
	GO_TO_PREV_STATE;\
}

#define SWITCH_TO_NEW_STATE_THIS_UPDATE(a) {\
	m_tStateStack.push(m_eCurrentState = a);\
	GO_TO_NEW_STATE_THIS_UPDATE(a);\
}

#define CHECK_IF_SWITCH_TO_NEW_STATE_THIS_UPDATE(a,b) {\
	if (a)\
		SWITCH_TO_NEW_STATE_THIS_UPDATE(b);\
}

#define CHECK_IF_GO_TO_PREV_STATE_THIS_UPDATE(a) {\
	if (a)\
		GO_TO_PREV_STATE_THIS_UPDATE;\
}

#define CHECK_IF_GO_TO_NEW_STATE_THIS_UPDATE(a,b) \
	if (a)\
		GO_TO_NEW_STATE_THIS_UPDATE(b);


extern bool			bfGetActionSuccessProbability	(GroupHierarchyHolder::MEMBER_REGISTRY &Members, const xr_set<const CEntityAlive *> &VisibleEnemies, float fMinProbability, CBaseFunction &fSuccessProbabilityFunction);
extern u32			dwfChooseAction					(u32 dwActionRefreshRate, float fMinProbability0, float fMinProbability1, float fMinProbability2, float fMinProbability3, u32 dwTeam, u32 dwSquad, u32 dwGroup, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4, CEntity *tpEntity=0, float fGroupDistance = 100.f);
