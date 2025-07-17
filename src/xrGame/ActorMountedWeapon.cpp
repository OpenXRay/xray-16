#include "StdAfx.h"
#pragma hdrstop

#include "Actor.h"
#include "xrEngine/CameraBase.h"
#include "ActorEffector.h"
#include "CharacterPhysicsSupport.h"
#include "holder_custom.h"
#include "Car.h"
#include <actor_anim_defs.h>
#include <xrPhysics/ActorCameraCollision.h>

bool CActor::use_HolderEx(CHolderCustom* object, bool bForce)
{
    if (m_holder)
    {
        if (!m_holder->ExitLocked() || bForce)
        {
            if (!object || (m_holder == object)) {

                CGameObject* go = smart_cast<CGameObject*>(m_holder);
                CPhysicsShellHolder* pholder = smart_cast<CPhysicsShellHolder*>(go);
                if (pholder)
                {
                    pholder->PPhysicsShell()->SplitterHolderDeactivate();
                    if (!character_physics_support()->movement()->ActivateBoxDynamic(0))
                    {
                        pholder->PPhysicsShell()->SplitterHolderActivate();
                        return true;
                    }
                    pholder->PPhysicsShell()->SplitterHolderActivate();
                }

                SetWeaponHideState(INV_STATE_BLOCK_ALL, false);

                if (go) {
                    callback(GameObject::eDetachVehicle)(go->lua_game_object());
                }

                m_holder->detach_Actor();

                character_physics_support()->movement()->CreateCharacter();
                character_physics_support()->movement()->SetPosition(m_holder->ExitPosition());
                character_physics_support()->movement()->SetVelocity(m_holder->ExitVelocity());

                r_model_yaw = -m_holder->Camera()->yaw;
                r_torso.yaw = r_model_yaw;
                r_model_yaw_dest = r_model_yaw;

                cam_Active()->Direction().set(m_holder->Camera()->Direction());

                SetCallbacks();

                m_holder = NULL;
                m_holderID = u16(-1);

                IKinematicsAnimated* V = smart_cast<IKinematicsAnimated*>(Visual()); R_ASSERT(V);
                V->PlayCycle(m_anims->m_normal.legs_idle);
                V->PlayCycle(m_anims->m_normal.m_torso_idle);

                IKinematics* pK = smart_cast<IKinematics*>(Visual());
                u16 head_bone = pK->LL_BoneID("bip01_head");
                pK->LL_GetBoneInstance(u16(head_bone)).set_callback(bctPhysics, HeadCallback, this);
            }
        }
        return true;
    }
    else
    {
        if (object && !object->EnterLocked() || bForce)
        {
            Fvector center;	Center(center);
            if (bForce || object->Use(Device.vCameraPosition, Device.vCameraDirection, center) && object->attach_Actor(this))
            {
                inventory().SetActiveSlot(NO_ACTIVE_SLOT);
                SetWeaponHideState(INV_STATE_BLOCK_ALL, true);

                // destroy actor character
                character_physics_support()->movement()->DestroyCharacter();

                m_holder = object;
                IGameObject* oHolder = smart_cast<IGameObject*>(object);
                m_holderID = oHolder->ID();

                if (pCamBobbing) {
                    Cameras().RemoveCamEffector(eCEBobbing);
                    pCamBobbing = NULL;
                }

                if (actor_camera_shell)
                    destroy_physics_shell(actor_camera_shell);

                IKinematics* pK = smart_cast<IKinematics*>(Visual());
                u16 head_bone = pK->LL_BoneID("bip01_head");
                pK->LL_GetBoneInstance(u16(head_bone)).set_callback(bctPhysics, VehicleHeadCallback, this);

                CCar* car = smart_cast<CCar*>(object);
                if (car)
                {
                    u16 anim_type = car->DriverAnimationType();
                    SVehicleAnimCollection& anims = m_vehicle_anims->m_vehicles_type_collections[anim_type];
                    IKinematicsAnimated* V = smart_cast<IKinematicsAnimated*>(Visual()); R_ASSERT(V);
                    V->PlayCycle(anims.idles[0], FALSE);
                    CStepManager::on_animation_start(MotionID(), 0);
                }

                CGameObject* go = smart_cast<CGameObject*>(object);
                if (go) {
                    callback(GameObject::eAttachVehicle)(go->lua_game_object());
                }
                return true;
            }
        }
    }
    return false;
}
