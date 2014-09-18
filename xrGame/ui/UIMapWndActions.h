#pragma once
#include "../action_planner.h"
#include "../property_evaluator_const.h"

class CUIMapWnd;
class CUICustomMap;
class CUILevelMap;


class CMapActionPlanner :public CActionPlanner<CUIMapWnd,true> {
private:
	typedef CActionPlanner<CUIMapWnd,true> inherited;

public:
					CMapActionPlanner	();
	virtual			~CMapActionPlanner	();
	virtual	void	setup				(CUIMapWnd *object);
	virtual	LPCSTR	object_name			() const;
};

