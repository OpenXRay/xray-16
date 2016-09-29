#include "stdafx.h"
#pragma hdrstop

#include "actor.h"
#include "../xrEngine/CameraBase.h"
#include "ActorEffector.h"
#include "CharacterPhysicsSupport.h"
#include "holder_custom.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "Car.h"

bool CActor::use_HolderEx(CHolderCustom* object, bool bForce)
{
	if (m_holder)
	{
		CCar* car = smart_cast<CCar*>(m_holder);
		if (car)
		{
			detach_Vehicle();
			return true;
		}
		if (!m_holder->ExitLocked())
		{
			if (!object || (m_holder == object)){
				m_holder->detach_Actor();

				CGameObject* go = smart_cast<CGameObject*>(m_holder);
				if (go)
					this->callback(GameObject::eDetachVehicle)(go->lua_game_object());

				character_physics_support()->movement()->CreateCharacter();
				m_holder = NULL;
			}
		}
		return true;
	} 
	else
	{
		CCar* car = smart_cast<CCar*>(m_holder);
		if (car)
		{
			attach_Vehicle(object);
			return true;
		}
		if (object && !object->EnterLocked())
		{
			Fvector center;	Center(center);
			if (object->Use(Device.vCameraPosition, Device.vCameraDirection, center)){
				if (object->attach_Actor(this)){
					// destroy actor character
					character_physics_support()->movement()->DestroyCharacter();

					m_holder = object;
					if (pCamBobbing){
						Cameras().RemoveCamEffector(eCEBobbing);
						pCamBobbing = NULL;
					}

					CGameObject* go = smart_cast<CGameObject*>(m_holder);
					if (go)
						this->callback(GameObject::eAttachVehicle)(go->lua_game_object());
					return true;
				}
			}
		}
	}
	return false;
}