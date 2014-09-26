#pragma once

#include "encyclopedia_article_defs.h"
#include "GameTaskDefs.h"
#include "script_export_space.h"
#include "../../sdk/include/luabind/functor.hpp"

class CGameTaskManager;
class CMapLocation;
class CGameTask;

typedef xr_vector<luabind::functor<bool> > task_state_functors;

class SScriptTaskHelper: public IPureSerializeObject<IReader,IWriter>
{
public:
	xr_vector<shared_str>					m_s_complete_lua_functions;
	xr_vector<shared_str>					m_s_fail_lua_functions;

	xr_vector<shared_str>					m_s_lua_functions_on_complete;
	xr_vector<shared_str>					m_s_lua_functions_on_fail;
public:
	bool					not_empty		()	{return m_s_complete_lua_functions.size()	||
														m_s_fail_lua_functions.size()		||
														m_s_lua_functions_on_complete.size()||
														m_s_lua_functions_on_fail.size() ;}

	virtual void			save			(IWriter &stream);
	virtual void			load			(IReader &stream);
			
			void			init_functors	(xr_vector<shared_str>& v_src, task_state_functors& v_dest);
};

class CGameTask
{
private:
	ETaskState								m_task_state;
	ETaskType								m_task_type;
	SScriptTaskHelper						m_pScriptHelper;
public:
	shared_str								m_icon_texture_name;
	// map
	shared_str								m_map_hint;
	shared_str								m_map_location;
	u16										m_map_object_id;

private:
	//infos
	xr_vector<shared_str>					m_completeInfos;
	xr_vector<shared_str>					m_failInfos;
	xr_vector<shared_str>					m_infos_on_complete;
	xr_vector<shared_str>					m_infos_on_fail;

	// functions
	task_state_functors						m_fail_lua_functions;
	task_state_functors						m_complete_lua_functions;

	task_state_functors						m_lua_functions_on_complete;
	task_state_functors						m_lua_functions_on_fail;
	
	CMapLocation*							m_linked_map_location;

	void					SendInfo		(const xr_vector<shared_str>&);
	bool					CheckInfo		(const xr_vector<shared_str>&) const;
	void					CallAllFuncs	(const task_state_functors& v);
	bool					CheckFunctions	(const task_state_functors& v) const;

	void					CreateMapLocation(bool on_load);


							CGameTask		(const CGameTask&);
public:
							CGameTask		();

	void 					save_task		(IWriter &stream);
	void 					load_task		(IReader &stream);


	shared_str				m_ID;
	shared_str				m_Title;
	shared_str				m_Description;
	ALife::_TIME_ID			m_ReceiveTime;
	ALife::_TIME_ID			m_FinishTime;
	ALife::_TIME_ID			m_TimeToComplete;
	ALife::_TIME_ID			m_timer_finish;
	u32						m_priority;
	bool					m_read;

	void					OnArrived				();
	void					RemoveMapLocations		(bool notify);
	void					ChangeMapLocation		(LPCSTR new_map_location, u16 new_map_object_id);

	void					ChangeStateCallback		();
	void					SetTaskState			(ETaskState state);
	ETaskState				GetTaskState			() const					{return m_task_state;};
	ETaskType				GetTaskType				() const					{return m_task_type;}

	ETaskState				UpdateState				();
	IC CMapLocation*		LinkedMapLocation		() {return m_linked_map_location;}

// for scripting access
	void					SetTitle_script			(LPCSTR _title)				{m_Title = _title;}
	LPCSTR					GetTitle_script			()							{return m_Title.c_str();}
	void					SetPriority_script		(int _prio)					{m_priority	= _prio;}
	int						GetPriority_script		()							{return m_priority;}
	void					SetType_script			(int t)						{m_task_type = (ETaskType)t;}

	LPCSTR					GetID_script			()							{return m_ID.c_str();}
	void					SetID_script			(LPCSTR _id)				{m_ID = _id;}
	void					SetDescription_script	(LPCSTR _desc)				{m_Description = _desc;}
	void					SetIconName_script		(LPCSTR _tex)				{m_icon_texture_name = _tex;}
	LPCSTR					GetIconName_script		()							{return m_icon_texture_name.c_str();}
	void					SetMapHint_script		(LPCSTR _hint)				{m_map_hint = _hint;}

	void					SetMapLocation_script	(LPCSTR _mls)				{m_map_location = _mls;}
	void					SetMapObjectID_script	(int _id)					{m_map_object_id = (u16)_id;}

	void 					AddCompleteInfo_script	(LPCSTR _str);
	void 					AddFailInfo_script		(LPCSTR _str);
	void 					AddOnCompleteInfo_script(LPCSTR _str);
	void 					AddOnFailInfo_script	(LPCSTR _str);
	void 					AddCompleteFunc_script	(LPCSTR _str);
	void 					AddFailFunc_script		(LPCSTR _str);
	void 					AddOnCompleteFunc_script(LPCSTR _str);
	void 					AddOnFailFunc_script	(LPCSTR _str);

	void					CommitScriptHelperContents();
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CGameTask)
#undef script_type_list
#define script_type_list save_type_list(CGameTask)
