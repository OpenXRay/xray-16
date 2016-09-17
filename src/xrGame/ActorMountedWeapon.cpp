#include "stdafx.h"
#pragma hdrstop

#include "actor.h"
#include "xrEngine/CameraBase.h"
#include "ActorEffector.h"
#include "CharacterPhysicsSupport.h"
#include "holder_custom.h"
#include "script_callback_ex.h"
#include "script_game_object.h"

bool CActor::use_MountedWeapon(CHolderCustom* object)
{
    CHolderCustom* wpn = object;
    if (m_holder)
    {
        if (!wpn || m_holder == wpn)
        {
            m_holder->detach_Actor();

            CGameObject* go = smart_cast<CGameObject*>(m_holder);
            if (go)
                this->callback(GameObject::eDetachVehicle)(go->lua_game_object());

            character_physics_support()->movement()->CreateCharacter();
            m_holder = nullptr;
        }
        return true;
    }
    if (wpn)
    {
        Fvector center;
        Center(center);
        if (wpn->Use(Device.vCameraPosition, Device.vCameraDirection, center))
        {
            if (wpn->attach_Actor(this))
            {
                // destroy actor character
                character_physics_support()->movement()->DestroyCharacter();
                //PickupModeOff();
                m_holder = wpn;
                if (pCamBobbing)
                {
                    Cameras().RemoveCamEffector(eCEBobbing);
                    pCamBobbing = nullptr;
                }

                CGameObject* go = smart_cast<CGameObject*>(m_holder);
                if (go)
                    this->callback(GameObject::eAttachVehicle)(go->lua_game_object());

                return true;
            }
        }
    }
    return false;
}
