#include "pch_script.h"
#include "UsableScriptObject.h"
#include "GameObject.h"
#include "script_game_object.h"
#include "script_callback_ex.h"
#include "game_object_space.h"
#include "doors_door.h"
#include "doors.h"

using namespace luabind;

CUsableScriptObject::CUsableScriptObject()
{
	m_bNonscriptUsable = true;
	set_tip_text_default();
}

CUsableScriptObject::~CUsableScriptObject()
{
}

bool CUsableScriptObject::use(CGameObject* who_use)
{
	VERIFY(who_use);
	CGameObject* pThis = smart_cast<CGameObject*>(this); VERIFY(pThis);
	
	if ( pThis->lua_game_object() && pThis->lua_game_object()->m_door && (pThis->lua_game_object()->m_door->is_blocked(doors::door_state_open) || pThis->lua_game_object()->m_door->is_blocked(doors::door_state_closed)) )
		return false;

	pThis->callback(GameObject::eUseObject)(pThis->lua_game_object(),who_use->lua_game_object());

	return true;
}

LPCSTR CUsableScriptObject::tip_text	()
{
	return *m_sTipText;
}
void CUsableScriptObject::set_tip_text	(LPCSTR new_text) 
{
	m_sTipText = new_text;
}
void CUsableScriptObject::set_tip_text_default () 
{
	m_sTipText = NULL;
}

bool CUsableScriptObject::nonscript_usable		()
{
	return m_bNonscriptUsable;
}
void CUsableScriptObject::set_nonscript_usable	(bool usable)
{
	m_bNonscriptUsable = usable;
}
