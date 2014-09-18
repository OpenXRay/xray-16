#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "ai/monsters/bloodsucker/bloodsucker.h"
#include "ai/monsters/poltergeist/poltergeist.h"
#include "ai/monsters/burer/burer.h"
#include "ai/monsters/zombie/zombie.h"
#include "script_sound_info.h"
#include "script_monster_hit_info.h"
#include "ai/monsters/monster_home.h"
#include "ai/monsters/control_animation_base.h"

//////////////////////////////////////////////////////////////////////////
// Burer

void   CScriptGameObject::set_force_anti_aim (bool force)
{
	CBaseMonster* monster = smart_cast<CBaseMonster*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,
			"object is not CBaseMonster to call set_force_anti_aim");
		return;
	}

	monster->set_force_anti_aim(force);
}

bool   CScriptGameObject::get_force_anti_aim ()
{
	CBaseMonster* monster = smart_cast<CBaseMonster*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,
			"object is not CBaseMonster to call get_force_anti_aim");
		return false;
	}

	return	monster->get_force_anti_aim();
}

void   CScriptGameObject::burer_set_force_gravi_attack (bool force)
{
	CBurer *monster = smart_cast<CBurer*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,
			"object is not CBurer to call burer_set_force_gravi_attack");
		return;
	}

	monster->set_force_gravi_attack (force);
}

bool   CScriptGameObject::burer_get_force_gravi_attack ()
{
	CBurer *monster = smart_cast<CBurer*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,
			"object is not CBurer to call burer_set_force_gravi_attack");
		return false;
	}

	return	monster->get_force_gravi_attack ();
}

//////////////////////////////////////////////////////////////////////////
// Poltergeist

void   CScriptGameObject::poltergeist_set_actor_ignore (bool ignore)
{
	CPoltergeist *monster = smart_cast<CPoltergeist*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,
			"object is not Poltergeist to call poltergeist_set_actor_ignore");
		return;
	}

	monster->set_actor_ignore(ignore);
}

bool   CScriptGameObject::poltergeist_get_actor_ignore ()
{
	CPoltergeist *monster = smart_cast<CPoltergeist*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,
			"object is not Poltergeist to call poltergeist_get_actor_ignore");
		return false;
	}

	return	monster->get_actor_ignore();
}

//////////////////////////////////////////////////////////////////////////
//CAI_Bloodsucker

void   CScriptGameObject::force_visibility_state (int state)
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member force_visibility_state!");
		return;
	}

	monster->force_visibility_state(state);
}

int   CScriptGameObject::get_visibility_state ()
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member get_visibility_state!");
		return CAI_Bloodsucker::full_visibility;
	}

	return monster->get_visibility_state();
}

void   CScriptGameObject::set_override_animation (pcstr anim_name)
{
	CBaseMonster* monster	=	smart_cast<CBaseMonster*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"object is not of CBaseMonster class!");
		return;
	}

	monster->anim().set_override_animation(anim_name);
}

void   CScriptGameObject::clear_override_animation ()
{
	CBaseMonster* monster	=	smart_cast<CBaseMonster*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"object is not of CBaseMonster class!");
		return;
	}

	monster->anim().clear_override_animation();
}

void   CScriptGameObject::force_stand_sleep_animation (u32 index)
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member force_stand_sleep_animation!");
		return;
	}

	monster->force_stand_sleep_animation(index);
}

void   CScriptGameObject::release_stand_sleep_animation ()
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member release_stand_sleep_animation!");
		return;
	}

	monster->release_stand_sleep_animation();
}

void CScriptGameObject::set_invisible(bool val)
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member set_invisible!");
		return;
	}

	val ? monster->manual_activate() : monster->manual_deactivate();
}

void CScriptGameObject::set_manual_invisibility(bool val)
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member set_manual_invisible!");
		return;
	}
	val ? monster->set_manual_control(true) : monster->set_manual_control(false);
	
}

void CScriptGameObject::bloodsucker_drag_jump(CScriptGameObject* e, LPCSTR e_str, const Fvector &position, float factor)
{

	CAI_Bloodsucker	*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"CScriptGameObject : cannot process drag, anim, jump for CAI_Bloodsucker!");
		return;
	}

		
	CGameObject *game_object = &e->object();
	CEntityAlive *entity_alive = smart_cast<CEntityAlive*>(game_object);

	
	monster->set_drag_jump(entity_alive, e_str, position, factor);
}

void CScriptGameObject::set_enemy(CScriptGameObject* e)
{

	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member set_enemy!");
		return;
	}
	CGameObject *game_object = &e->object();
	CEntityAlive *entity_alive = smart_cast<CEntityAlive*>(game_object);
	monster->SetEnemy(entity_alive);
}

void CScriptGameObject::set_vis_state(float val)
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member set_vis_state!");
		return;
	}
	if(val==1){
		monster->set_vis();
	}
	if(val==-1){
		monster->set_invis();
	}
}

void CScriptGameObject::off_collision(bool val)
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member set_vis_state!");
		return;
	}
	monster->set_collision_off(val);
}

void CScriptGameObject::set_alien_control(bool val)
{
	CAI_Bloodsucker		*monster = smart_cast<CAI_Bloodsucker*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CAI_Bloodsucker : cannot access class member alien_control_activate!");
		return;
	}

	monster->set_alien_control(val);
}

CScriptSoundInfo CScriptGameObject::GetSoundInfo()
{
	CScriptSoundInfo	ret_val;

	CBaseMonster *l_tpMonster = smart_cast<CBaseMonster *>(&object());
	if (l_tpMonster) {
		if (l_tpMonster->SoundMemory.IsRememberSound()) {
			SoundElem se; 
			bool bDangerous;
			l_tpMonster->SoundMemory.GetSound(se, bDangerous);

			const CGameObject *pO = smart_cast<const CGameObject *>(se.who);
			ret_val.set((pO && !pO->getDestroy()) ?  pO->lua_game_object() : 0, bDangerous, se.position, se.power, int(se.time));
		}
	} else {
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,"CScriptGameObject : cannot access class member GetSoundInfo!");
	}
	return			(ret_val);
}

CScriptMonsterHitInfo CScriptGameObject::GetMonsterHitInfo()
{
	CScriptMonsterHitInfo	ret_val;

	CBaseMonster *l_tpMonster = smart_cast<CBaseMonster *>(&object());
	if (l_tpMonster) {
		if (l_tpMonster->HitMemory.is_hit()) {
			CGameObject *pO = smart_cast<CGameObject *>(l_tpMonster->HitMemory.get_last_hit_object());
			ret_val.set((pO && !pO->getDestroy()) ?  pO->lua_game_object() : 0, l_tpMonster->HitMemory.get_last_hit_dir(), l_tpMonster->HitMemory.get_last_hit_time());
		}
	} else {
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,"CScriptGameObject : cannot access class member GetMonsterHitInfo!");
	}
	return			(ret_val);
}

//////////////////////////////////////////////////////////////////////////
// CBaseMonster
void CScriptGameObject::skip_transfer_enemy(bool val)
{
	CBaseMonster *monster = smart_cast<CBaseMonster *>(&object());
	if (monster) monster->skip_transfer_enemy(val);
}

void CScriptGameObject::set_home(LPCSTR name, float r_min, float r_max, bool aggressive, float r_mid)
{
	CBaseMonster *monster = smart_cast<CBaseMonster *>(&object());
	if (monster) monster->Home->setup(name,r_min,r_max,aggressive, r_mid);
}
void CScriptGameObject::set_home(u32 lv_ID, float r_min, float r_max, bool aggressive, float r_mid)
{
	CBaseMonster *monster = smart_cast<CBaseMonster *>(&object());
	if (monster) monster->Home->setup(lv_ID,r_min,r_max,aggressive, r_mid);
}
void CScriptGameObject::remove_home()
{
	CBaseMonster *monster = smart_cast<CBaseMonster *>(&object());
	if (monster) monster->Home->remove_home();
}

bool CScriptGameObject::fake_death_fall_down()
{
	CZombie	*monster = smart_cast<CZombie*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CZombie : cannot access class member fake_death_fall_down!");
		return false;
	}

	return monster->fake_death_fall_down();
}
void CScriptGameObject::fake_death_stand_up()
{
	CZombie	*monster = smart_cast<CZombie*>(&object());
	if (!monster) {
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CZombie : cannot access class member fake_death_fall_down!");
		return;
	}

	monster->fake_death_stand_up();
}

void CScriptGameObject::berserk()
{
	CBaseMonster *monster = smart_cast<CBaseMonster *>(&object());
	if (monster) monster->set_berserk();
}

void CScriptGameObject::set_custom_panic_threshold(float value)
{
	CBaseMonster *monster = smart_cast<CBaseMonster *>(&object());
	if (monster) monster->set_custom_panic_threshold(value);
}

void CScriptGameObject::set_default_panic_threshold()
{
	CBaseMonster *monster = smart_cast<CBaseMonster *>(&object());
	if (monster) monster->set_default_panic_threshold();
}


