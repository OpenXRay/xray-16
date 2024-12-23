#include "StdAfx.h"
#pragma hdrstop

#include "Actor.h"
#include "xrEngine/CameraBase.h"
#include "ActorEffector.h"
#include "CharacterPhysicsSupport.h"
#include "holder_custom.h"
#include "Car.h"

bool CActor::use_HolderEx(CHolderCustom* object, bool bForce)
{
    if (m_holder)
    {
        if (smart_cast<CCar*>(m_holder))
        {
            detach_Vehicle();
            return true;
        }
        if (!m_holder->ExitLocked())
        {
            if (!object || (m_holder == object))
            {
                m_holder->detach_Actor();

                if (const CGameObject* go = smart_cast<CGameObject*>(m_holder))
                    callback(GameObject::eDetachVehicle)(go->lua_game_object());

                character_physics_support()->movement()->CreateCharacter();
                m_holder = nullptr;
            }
        }
        return true;
    }
    if (smart_cast<CCar*>(m_holder))
    {
        attach_Vehicle(object);
        return true;
    }
    if (object && !object->EnterLocked())
    {
        Fvector center;
        Center(center);
        if (object->Use(Device.vCameraPosition, Device.vCameraDirection, center))
        {
            if (object->attach_Actor(this))
            {
                // destroy actor character
                character_physics_support()->movement()->DestroyCharacter();

                m_holder = object;
                if (pCamBobbing)
                {
                    Cameras().RemoveCamEffector(eCEBobbing);
                    pCamBobbing = nullptr;
                }

                if (const CGameObject* go = smart_cast<CGameObject*>(m_holder))
                    callback(GameObject::eAttachVehicle)(go->lua_game_object());
                return true;
            }
        }
    }
    return false;
}
