#include "StdAfx.h"
#include "Actor.h"
#include "Torch.h"
#include "trade.h"
#include "xrEngine/CameraBase.h"

#ifdef DEBUG
#include "PHDebug.h"
#endif

#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "UIGameSP.h"
#include "Inventory.h"
#include "Level.h"
#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "ActorCondition.h"
#include "actor_input_handler.h"
#include "string_table.h"
#include "xrUICore/Static/UIStatic.h"
#include "ui/UIActorMenu.h"
#include "ui/UIDragDropReferenceList.h"
#include "CharacterPhysicsSupport.h"
#include "InventoryBox.h"
#include "player_hud.h"
#include "xrEngine/xr_input.h"
#include "flare.h"
#include "CustomDetector.h"
#include "clsid_game.h"
#include "HUDManager.h"
#include "Weapon.h"

extern u32 hud_adj_mode;

void CActor::IR_OnKeyboardPress(int cmd)
{
    if (hud_adj_mode && pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
        return;

    if (Remote())
        return;

    if (IsTalking())
        return;
    if (m_input_external_handler && !m_input_external_handler->authorized(cmd))
        return;

    if (load_screen_renderer.IsActive())
        return;

    switch (cmd)
    {
    case kWPN_FIRE:
    {
        if ((mstate_wishful & mcLookout) && !IsGameTypeSingle())
            return;

        u16 slot = inventory().GetActiveSlot();
        if (inventory().ActiveItem() && (slot == INV_SLOT_3 || slot == INV_SLOT_2))
            mstate_wishful &= ~mcSprint;
        //-----------------------------
        if (OnServer())
        {
            NET_Packet P;
            P.w_begin(M_PLAYER_FIRE);
            P.w_u16(ID());
            u_EventSend(P);
        }
    }
    break;
    default: {
    }
    break;
    }

    if (!g_Alive())
        return;

    if (m_holder && kUSE != cmd)
    {
        m_holder->OnKeyboardPress(cmd);
        if (m_holder->allowWeapon() && inventory().Action((u16)cmd, CMD_START))
            return;
        return;
    }
    else if (inventory().Action((u16)cmd, CMD_START))
        return;

#ifdef DEBUG
    if (psActorFlags.test(AF_NO_CLIP))
    {
        NoClipFly(cmd);
        return;
    }
#endif // DEBUG
    switch (cmd)
    {
    case kJUMP: { mstate_wishful |= mcJump;
    }
    break;
    case kSPRINT_TOGGLE: { mstate_wishful ^= mcSprint;
    }
    break;
    case kCROUCH:
    {
        if (psActorFlags.test(AF_CROUCH_TOGGLE))
            mstate_wishful ^= mcCrouch;
    }
    break;
    case kCAM_1: cam_Set(eacFirstEye); break;
    case kCAM_2: cam_Set(eacLookAt); break;
    case kCAM_3: cam_Set(eacFreeLook); break;
    case kNIGHT_VISION:
    {
        SwitchNightVision();
        break;
    }
    case kTORCH:
    {
        SwitchTorch();
        break;
    }

    case kDETECTOR:
    {
        PIItem det_active = inventory().ItemFromSlot(DETECTOR_SLOT);
        if (det_active)
        {
            CCustomDetector* det = smart_cast<CCustomDetector*>(det_active);
            det->ToggleDetector(g_player_hud->attached_item(0) != NULL);
            return;
        }
    }
    break;
    /*
        case kFLARE:{
                PIItem fl_active = inventory().ItemFromSlot(FLARE_SLOT);
                if(fl_active)
                {
                    CFlare* fl			= smart_cast<CFlare*>(fl_active);
                    fl->DropFlare		();
                    return				;
                }

                PIItem fli = inventory().Get(CLSID_DEVICE_FLARE, true);
                if(!fli)			return;

                CFlare* fl			= smart_cast<CFlare*>(fli);

                if(inventory().Slot(fl))
                    fl->ActivateFlare	();
            }break;
    */
    case kUSE: ActorUse(); break;
    case kDROP:
        b_DropActivated = TRUE;
        f_DropPower = 0;
        break;
    case kNEXT_SLOT: { OnNextWeaponSlot();
    }
    break;
    case kPREV_SLOT: { OnPrevWeaponSlot();
    }
    break;

    case kQUICK_USE_1:
    case kQUICK_USE_2:
    case kQUICK_USE_3:
    case kQUICK_USE_4:
    {
        const shared_str& item_name = g_quick_use_slots[cmd - kQUICK_USE_1];
        if (item_name.size())
        {
            PIItem itm = inventory().GetAny(item_name.c_str());

            if (itm)
            {
                if (IsGameTypeSingle())
                {
                    inventory().Eat(itm);
                }
                else
                {
                    inventory().ClientEat(itm);
                }

                StaticDrawableWrapper* _s = CurrentGameUI()->AddCustomStatic("item_used", true);
                string1024 str;
                strconcat(sizeof(str), str, *StringTable().translate("st_item_used"), ": ", itm->NameItem());
                _s->wnd()->TextItemControl()->SetText(str);

                CurrentGameUI()->GetActorMenu().m_pQuickSlot->ReloadReferences(this);
            }
        }
    }
    break;
    }
}

void CActor::IR_OnMouseWheel(int x, int y)
{
    if (hud_adj_mode)
    {
        g_player_hud->tune(Ivector().set(0, 0, x));
        return;
    }

    if (inventory().Action((x > 0) ? (u16)kWPN_ZOOM_DEC : (u16)kWPN_ZOOM_INC, CMD_START))
        return;

    if (x > 0)
        OnNextWeaponSlot();
    else
        OnPrevWeaponSlot();
}

void CActor::IR_OnKeyboardRelease(int cmd)
{
    if (hud_adj_mode && pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
        return;

    if (Remote())
        return;

    if (m_input_external_handler && !m_input_external_handler->authorized(cmd))
        return;

    if (g_Alive())
    {
        if (m_holder)
        {
            m_holder->OnKeyboardRelease(cmd);

            if (m_holder->allowWeapon() && inventory().Action((u16)cmd, CMD_STOP))
                return;
            return;
        }
        else if (inventory().Action((u16)cmd, CMD_STOP))
            return;

        switch (cmd)
        {
        case kJUMP: mstate_wishful &= ~mcJump; break;
        case kDROP:
            if (GAME_PHASE_INPROGRESS == Game().Phase())
                g_PerformDrop();
            break;
        }
    }
}

void CActor::IR_OnKeyboardHold(int cmd)
{
    if (hud_adj_mode && pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
        return;

    if (Remote() || !g_Alive())
        return;
    if (m_input_external_handler && !m_input_external_handler->authorized(cmd))
        return;
    if (IsTalking())
        return;

    if (m_holder)
    {
        m_holder->OnKeyboardHold(cmd);
        return;
    }

#ifdef DEBUG
    if (psActorFlags.test(AF_NO_CLIP) &&
        (cmd == kFWD || cmd == kBACK || cmd == kL_STRAFE || cmd == kR_STRAFE || cmd == kJUMP || cmd == kCROUCH))
    {
        NoClipFly(cmd);
        return;
    }
#endif // DEBUG
    float LookFactor = GetLookFactor();
    switch (cmd)
    {
    case kUP:
    case kDOWN: cam_Active()->Move((cmd == kUP) ? kDOWN : kUP, 0, LookFactor); break;
    case kCAM_ZOOM_IN:
    case kCAM_ZOOM_OUT: cam_Active()->Move(cmd); break;
    case kLEFT:
    case kRIGHT:
        if (eacFreeLook != cam_active)
            cam_Active()->Move(cmd, 0, LookFactor);
        break;

    case kACCEL: mstate_wishful |= mcAccel; break;
    case kL_STRAFE: mstate_wishful |= mcLStrafe; break;
    case kR_STRAFE: mstate_wishful |= mcRStrafe; break;
    case kL_LOOKOUT: mstate_wishful |= mcLLookout; break;
    case kR_LOOKOUT: mstate_wishful |= mcRLookout; break;
    case kFWD: mstate_wishful |= mcFwd; break;
    case kBACK: mstate_wishful |= mcBack; break;
    case kCROUCH:
    {
        if (!psActorFlags.test(AF_CROUCH_TOGGLE))
            mstate_wishful |= mcCrouch;
    }
    break;
    }
}

void CActor::IR_OnMouseMove(int dx, int dy)
{
    if (hud_adj_mode)
    {
        g_player_hud->tune(Ivector().set(dx, dy, 0));
        return;
    }

    PIItem iitem = inventory().ActiveItem();
    if (iitem && iitem->cast_hud_item())
        iitem->cast_hud_item()->ResetSubStateTime();

    if (Remote())
        return;

    if (m_holder)
    {
        m_holder->OnMouseMove(dx, dy);
        return;
    }

    float LookFactor = GetLookFactor();

    CCameraBase* C = cameras[cam_active];
    float scale = (C->f_fov / g_fov) * psMouseSens * psMouseSensScale / 50.f / LookFactor;
    if (dx)
    {
        float d = float(dx) * scale;
        cam_Active()->Move((d < 0) ? kLEFT : kRIGHT, _abs(d));
    }
    if (dy)
    {
        float d = ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * 3.f / 4.f;
        cam_Active()->Move((d > 0) ? kUP : kDOWN, _abs(d));
    }
}
#include "HudItem.h"
bool CActor::use_Holder(CHolderCustom* holder)
{
    if (m_holder)
    {
        bool b = false;
        CGameObject* holderGO = smart_cast<CGameObject*>(m_holder);

        if (smart_cast<CCar*>(holderGO))
            b = use_Vehicle(0);
        else if (holderGO->CLS_ID == CLSID_OBJECT_W_STATMGUN)
            b = use_MountedWeapon(0);

        if (inventory().ActiveItem())
        {
            CHudItem* hi = smart_cast<CHudItem*>(inventory().ActiveItem());
            if (hi)
                hi->OnAnimationEnd(hi->GetState());
        }

        return b;
    }
    else
    {
        bool b = false;
        CGameObject* holderGO = smart_cast<CGameObject*>(holder);
        if (smart_cast<CCar*>(holder))
            b = use_Vehicle(holder);

        if (holderGO->CLS_ID == CLSID_OBJECT_W_STATMGUN)
            b = use_MountedWeapon(holder);

        if (b)
        { // used succesfully
            // switch off torch...
            CAttachableItem* I = CAttachmentOwner::attachedItem(CLSID_DEVICE_TORCH);
            if (I)
            {
                CTorch* torch = smart_cast<CTorch*>(I);
                if (torch)
                    torch->Switch(false);
            }
        }

        if (inventory().ActiveItem())
        {
            CHudItem* hi = smart_cast<CHudItem*>(inventory().ActiveItem());
            if (hi)
                hi->OnAnimationEnd(hi->GetState());
        }

        return b;
    }
}

void CActor::ActorUse()
{
    if (m_holder)
    {
        CGameObject* GO = smart_cast<CGameObject*>(m_holder);
        NET_Packet P;
        CGameObject::u_EventGen(P, GEG_PLAYER_DETACH_HOLDER, ID());
        P.w_u16(GO->ID());
        CGameObject::u_EventSend(P);
        return;
    }

    if (character_physics_support()->movement()->PHCapture())
        character_physics_support()->movement()->PHReleaseObject();

    if (m_pUsableObject && NULL == m_pObjectWeLookingAt->cast_inventory_item())
    {
        m_pUsableObject->use(this);
    }

    if (m_pInvBoxWeLookingAt && m_pInvBoxWeLookingAt->nonscript_usable())
    {
        CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
        if (pGameSP) // single
        {
            if (!m_pInvBoxWeLookingAt->closed())
            {
                pGameSP->StartCarBody(this, m_pInvBoxWeLookingAt);
            }
        }
        return;
    }

    if (!m_pUsableObject || m_pUsableObject->nonscript_usable())
    {
        if (m_pPersonWeLookingAt)
        {
            CEntityAlive* pEntityAliveWeLookingAt = smart_cast<CEntityAlive*>(m_pPersonWeLookingAt);

            VERIFY(pEntityAliveWeLookingAt);

            if (IsGameTypeSingle())
            {
                if (pEntityAliveWeLookingAt->g_Alive())
                {
                    TryToTalk();
                }
                else
                {
                    //только если находимся в режиме single
                    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
                    if (pGameSP)
                    {
                        if (!m_pPersonWeLookingAt->deadbody_closed_status())
                        {
                            if (pEntityAliveWeLookingAt->AlreadyDie() &&
                                pEntityAliveWeLookingAt->GetLevelDeathTime() + 3000 < Device.dwTimeGlobal)
                                // 99.9% dead
                                pGameSP->StartCarBody(this, m_pPersonWeLookingAt);
                        }
                    }
                }
            }
        }

        collide::rq_result& RQ = HUD().GetCurrentRayQuery();
        CPhysicsShellHolder* object = smart_cast<CPhysicsShellHolder*>(RQ.O);
        u16 element = BI_NONE;
        if (object)
            element = (u16)RQ.element;

        if (object && Level().IR_GetKeyState(SDL_SCANCODE_LSHIFT))
        {
            bool b_allow = !!pSettings->line_exist("ph_capture_visuals", object->cNameVisual());
            if (b_allow && !character_physics_support()->movement()->PHCapture())
            {
                character_physics_support()->movement()->PHCaptureObject(object, element);
            }
        }
        else
        {
            if (object && smart_cast<CHolderCustom*>(object))
            {
                NET_Packet P;
                CGameObject::u_EventGen(P, GEG_PLAYER_ATTACH_HOLDER, ID());
                P.w_u16(object->ID());
                CGameObject::u_EventSend(P);
                return;
            }
        }
    }
}

BOOL CActor::HUDview() const
{
    return IsFocused() && (cam_active == eacFirstEye) &&
        ((!m_holder) || (m_holder && m_holder->allowWeapon() && m_holder->HUDView()));
}

static u16 SlotsToCheck[] = {
    KNIFE_SLOT, // 0
    INV_SLOT_2, // 1
    INV_SLOT_3, // 2
    GRENADE_SLOT, // 3
    ARTEFACT_SLOT, // 10
};

void CActor::OnNextWeaponSlot()
{
    u32 ActiveSlot = inventory().GetActiveSlot();
    if (ActiveSlot == NO_ACTIVE_SLOT)
        ActiveSlot = inventory().GetPrevActiveSlot();

    if (ActiveSlot == NO_ACTIVE_SLOT)
        ActiveSlot = KNIFE_SLOT;

    u32 NumSlotsToCheck = sizeof(SlotsToCheck) / sizeof(SlotsToCheck[0]);

    u32 CurSlot = 0;
    for (; CurSlot < NumSlotsToCheck; CurSlot++)
    {
        if (SlotsToCheck[CurSlot] == ActiveSlot)
            break;
    };

    if (CurSlot >= NumSlotsToCheck)
        return;

    for (u32 i = CurSlot + 1; i < NumSlotsToCheck; i++)
    {
        if (inventory().ItemFromSlot(SlotsToCheck[i]))
        {
            if (SlotsToCheck[i] == ARTEFACT_SLOT)
            {
                IR_OnKeyboardPress(kARTEFACT);
            }
            else
                IR_OnKeyboardPress(kWPN_1 + i);
            return;
        }
    }
};

void CActor::OnPrevWeaponSlot()
{
    u32 ActiveSlot = inventory().GetActiveSlot();
    if (ActiveSlot == NO_ACTIVE_SLOT)
        ActiveSlot = inventory().GetPrevActiveSlot();

    if (ActiveSlot == NO_ACTIVE_SLOT)
        ActiveSlot = KNIFE_SLOT;

    u32 NumSlotsToCheck = sizeof(SlotsToCheck) / sizeof(SlotsToCheck[0]);
    u32 CurSlot = 0;

    for (; CurSlot < NumSlotsToCheck; CurSlot++)
    {
        if (SlotsToCheck[CurSlot] == ActiveSlot)
            break;
    };

    if (CurSlot >= NumSlotsToCheck)
        CurSlot = NumSlotsToCheck - 1; // last in row

    for (s32 i = s32(CurSlot - 1); i >= 0; i--)
    {
        if (inventory().ItemFromSlot(SlotsToCheck[i]))
        {
            if (SlotsToCheck[i] == ARTEFACT_SLOT)
            {
                IR_OnKeyboardPress(kARTEFACT);
            }
            else
                IR_OnKeyboardPress(kWPN_1 + i);
            return;
        }
    }
};

float CActor::GetLookFactor()
{
    if (m_input_external_handler)
        return m_input_external_handler->mouse_scale_factor();

    float factor = 1.f;

    PIItem pItem = inventory().ActiveItem();

    if (pItem)
        factor *= pItem->GetControlInertionFactor();

    VERIFY(!fis_zero(factor));

    return factor;
}

void CActor::set_input_external_handler(CActorInputHandler* handler)
{
    // clear state
    if (handler)
        mstate_wishful = 0;

    // release fire button
    if (handler)
        IR_OnKeyboardRelease(kWPN_FIRE);

    // set handler
    m_input_external_handler = handler;
}

void CActor::SwitchNightVision()
{
    CWeapon* wpn1 = NULL;
    CWeapon* wpn2 = NULL;
    if (inventory().ItemFromSlot(INV_SLOT_2))
        wpn1 = smart_cast<CWeapon*>(inventory().ItemFromSlot(INV_SLOT_2));

    if (inventory().ItemFromSlot(INV_SLOT_3))
        wpn2 = smart_cast<CWeapon*>(inventory().ItemFromSlot(INV_SLOT_3));

    xr_vector<CAttachableItem*> const& all = CAttachmentOwner::attached_objects();
    xr_vector<CAttachableItem*>::const_iterator it = all.begin();
    xr_vector<CAttachableItem*>::const_iterator it_e = all.end();
    for (; it != it_e; ++it)
    {
        CTorch* torch = smart_cast<CTorch*>(*it);
        if (torch)
        {
            if (wpn1 && wpn1->IsZoomed())
                return;

            if (wpn2 && wpn2->IsZoomed())
                return;

            torch->SwitchNightVision();
            return;
        }
    }
}

void CActor::SwitchTorch()
{
    xr_vector<CAttachableItem*> const& all = CAttachmentOwner::attached_objects();
    xr_vector<CAttachableItem*>::const_iterator it = all.begin();
    xr_vector<CAttachableItem*>::const_iterator it_e = all.end();
    for (; it != it_e; ++it)
    {
        CTorch* torch = smart_cast<CTorch*>(*it);
        if (torch)
        {
            torch->Switch();
            return;
        }
    }
}

#ifdef DEBUG
void CActor::NoClipFly(int cmd)
{
    Fvector cur_pos; // = Position();
    cur_pos.set(0, 0, 0);
    float scale = 1.0f;
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
        scale = 0.25f;
    else if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LALT))
        scale = 4.0f;

    switch (cmd)
    {
    case kJUMP: cur_pos.y += 0.1f; break;
    case kCROUCH: cur_pos.y -= 0.1f; break;
    case kFWD: cur_pos.z += 0.1f; break;
    case kBACK: cur_pos.z -= 0.1f; break;
    case kL_STRAFE: cur_pos.x -= 0.1f; break;
    case kR_STRAFE: cur_pos.x += 0.1f; break;
    case kCAM_1: cam_Set(eacFirstEye); break;
    case kCAM_2: cam_Set(eacLookAt); break;
    case kCAM_3: cam_Set(eacFreeLook); break;
    case kNIGHT_VISION: SwitchNightVision(); break;
    case kTORCH: SwitchTorch(); break;
    case kDETECTOR:
    {
        PIItem det_active = inventory().ItemFromSlot(DETECTOR_SLOT);
        if (det_active)
        {
            CCustomDetector* det = smart_cast<CCustomDetector*>(det_active);
            det->ToggleDetector(g_player_hud->attached_item(0) != NULL);
            return;
        }
    }
    break;
    case kUSE: ActorUse(); break;
    }
    cur_pos.mul(scale);
    Fmatrix mOrient;
    mOrient.rotateY(-(cam_Active()->GetWorldYaw()));
    mOrient.transform_dir(cur_pos);
    Position().add(cur_pos);
    character_physics_support()->movement()->SetPosition(Position());
}
#endif // DEBUG
