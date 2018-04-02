#include "stdafx.h"
#pragma hdrstop

#include "actor.h"
#include "xrEngine/CameraBase.h"
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
            if (!object || m_holder == object)
            {
                SetWeaponHideState(INV_STATE_CAR, false);

                CGameObject* go = smart_cast<CGameObject*>(m_holder);
                if (go)
                    this->callback(GameObject::eDetachVehicle)(go->lua_game_object());

                m_holder->detach_Actor();

                character_physics_support()->movement()->CreateCharacter();
                character_physics_support()->movement()->SetPosition(m_holder->ExitPosition());
                character_physics_support()->movement()->SetVelocity(m_holder->ExitVelocity());

                r_model_yaw = -m_holder->Camera()->yaw;
                r_torso.yaw = r_model_yaw;
                r_model_yaw_dest = r_model_yaw;

                SetCallbacks();

                m_holder = nullptr;
                m_holderID = u16(-1);
            }
        }
        return true;
    }
    CCar* car = smart_cast<CCar*>(object);
    if (car)
    {
        attach_Vehicle(object);
        return true;
    }
    if (object && !object->EnterLocked())
    {
        Fvector center;
        Center(center);
        if (object->Use(Device.vCameraPosition, Device.vCameraDirection, center) && object->attach_Actor(this))
        {
            SetWeaponHideState(INV_STATE_CAR, true);

            // destroy actor character
            character_physics_support()->movement()->DestroyCharacter();

            m_holder = object;
            IGameObject* oHolder = smart_cast<IGameObject*>(object);
            m_holderID = oHolder->ID();

            if (pCamBobbing)
            {
                Cameras().RemoveCamEffector(eCEBobbing);
                pCamBobbing = nullptr;
            }

            CGameObject* go = smart_cast<CGameObject*>(object);
            if (go)
                this->callback(GameObject::eAttachVehicle)(go->lua_game_object());
            return true;
        }
    }
    return false;
}
