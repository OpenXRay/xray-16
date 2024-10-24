#include "StdAfx.h"
#include "xrEngine/XR_IOConsole.h"
#include "entity_alive.h"
#include "game_sv_single.h"
#include "alife_simulator.h"
#include "alife_simulator_header.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrEngine/FDemoRecord.h"
#include "Level.h"
#include "xrEngine/xr_level_controller.h"
#include "game_cl_base.h"
#include "stalker_movement_manager_smart_cover.h"
#include "Inventory.h"
#include "xrServer.h"
#include "autosave_manager.h"
#include "xrScriptEngine/script_callback_ex.h"

#include "Actor.h"
#include "HudItem.h"
#include "UIGameCustom.h"
#include "ui/UIDialogWnd.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/xr_object.h"
#include "saved_game_wrapper.h"
#include "xrNetServer/NET_Messages.h"

#include "Include/xrRender/DebugRender.h"

#ifdef DEBUG
#include "ai/monsters/basemonster/base_monster.h"

// Lain: add
#include "level_debug.h"
#endif

#ifdef DEBUG
extern void try_change_current_entity();
extern void restore_actor();
#endif

bool g_bDisableAllInput = false;
extern float g_fTimeFactor;

#define CURRENT_ENTITY() (game ? ((GameID() == eGameIDSingle) ? CurrentEntity() : CurrentControlEntity()) : NULL)

void CLevel::IR_OnMouseWheel(float x, float y)
{
    if (g_bDisableAllInput)
        return;

    /* avo: script callback */
    if (g_actor)
    {
        g_actor->callback(GameObject::eMouseWheel)(y, x);
    }

    if (CurrentGameUI()->IR_UIOnMouseWheel(x, y))
        return;

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused())
            return;
    }

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnMouseWheel(x, y);
    }
}

void CLevel::IR_OnMousePress(int btn) { IR_OnKeyboardPress(btn); }
void CLevel::IR_OnMouseRelease(int btn) { IR_OnKeyboardRelease(btn); }
void CLevel::IR_OnMouseHold(int btn) { IR_OnKeyboardHold(btn); }

void CLevel::IR_OnMouseMove(int dx, int dy)
{
    if (g_bDisableAllInput)
        return;

    /* avo: script callback */
    if (g_actor)
    {
        g_actor->callback(GameObject::eMouseMove)(dx, dy);
    }

    if (CurrentGameUI()->IR_UIOnMouseMove(dx, dy))
        return;

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused() && !IsDemoPlay())
            return;
    }

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnMouseMove(dx, dy);
    }
}

// Обработка нажатия клавиш
extern bool g_block_pause;

// Lain: added TEMP!!!
#ifdef DEBUG
extern float g_separate_factor;
extern float g_separate_radius;
#endif

#include "xrScriptEngine/script_engine.hpp"
#include "ai_space.h"

void CLevel::IR_OnKeyboardPress(int key)
{
    if (Device.dwPrecacheFrame)
        return;

    bool b_ui_exist = !!CurrentGameUI();

    EGameActions _curr = GetBindedAction(key);

    /* avo: script callback */
    if (!g_bDisableAllInput && g_actor)
    {
        g_actor->callback(GameObject::eKeyPress)(key);
    }

    if (_curr == kPAUSE)
    {
        if (Device.editor_mode())
            return;

        if (!g_block_pause && (IsGameTypeSingle() || IsDemoPlay()))
        {
#ifdef MASTER_GOLD
            pcstr reason = "li_pause_key";
#else
            pcstr reason = psActorFlags.test(AF_NO_CLIP) ? "li_pause_key_no_clip" : "li_pause_key";
#endif
            Device.Pause(!Device.Paused(), TRUE, TRUE, reason);
        }
        return;
    }

    if (_curr == kEDITOR)
    {
        Device.editor().SwitchToNextState();
        return;
    }

    if (g_bDisableAllInput)
        return;

    switch (_curr)
    {
    case kSCREENSHOT:
        GEnv.Render->Screenshot();
        return;

    case kCONSOLE:
        Console->Show();
        return;

    case kQUIT:
    {
        if (b_ui_exist && CurrentGameUI()->TopInputReceiver() && !Device.Paused())
        {
            if (CurrentGameUI()->IR_UIOnKeyboardPress(key))
                return; // special case for mp and main_menu
            CurrentGameUI()->TopInputReceiver()->HideDialog();
        }
        else
        {
            Console->Execute("main_menu");
        }
        return;
    }
    case kALIFE_CMD:
    {
        luabind::functor<void> functor;
        if (GEnv.ScriptEngine->functor("sim_combat.start_attack", functor))
            functor();
#ifndef MASTER_GOLD
        else
        {
            Log("! failed to get sim_combat.start_attack functor");
        }
#endif
        break;
    }
    } // switch (_curr)

    if (!bReady || !b_ui_exist)
        return;

    if (b_ui_exist && CurrentGameUI()->IR_UIOnKeyboardPress(key))
        return;

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused() && !IsDemoPlay())
            return;
    }

    if (game && game->OnKeyboardPress(GetBindedAction(key)))
        return;

    luabind::functor<bool> funct;
    if (GEnv.ScriptEngine->functor("level_input.on_key_press", funct))
    {
        if (funct(key, _curr))
            return;
    }

    if (_curr == kQUICK_SAVE && IsGameTypeSingle())
    {
        Console->Execute("save");
        return;
    }
    if (_curr == kQUICK_LOAD && IsGameTypeSingle())
    {
#ifdef DEBUG
        FS.get_path("$game_config$")->m_Flags.set(FS_Path::flNeedRescan, TRUE);
        FS.get_path("$game_scripts$")->m_Flags.set(FS_Path::flNeedRescan, TRUE);
        FS.rescan_pathes();
#endif // DEBUG
        string_path saved_game, command;
        strconcat(sizeof(saved_game), saved_game, Core.UserName, " - ", "quicksave");
        if (!CSavedGameWrapper::valid_saved_game(saved_game))
            return;

        strconcat(sizeof(command), command, "load ", saved_game);
        Console->Execute(command);
        return;
    }

#ifndef MASTER_GOLD
    switch (key)
    {
    case SDL_SCANCODE_F7:
    {
        if (GameID() != eGameIDSingle)
            return;
        FS.get_path("$game_config$")->m_Flags.set(FS_Path::flNeedRescan, TRUE);
        FS.get_path("$game_scripts$")->m_Flags.set(FS_Path::flNeedRescan, TRUE);
        FS.rescan_pathes();
        NET_Packet net_packet;
        net_packet.w_begin(M_RELOAD_GAME);
        Send(net_packet, net_flags(TRUE));
        return;
    }
    case SDL_SCANCODE_KP_5:
    {
        if (GameID() != eGameIDSingle)
        {
            Msg("For this game type Demo Record is disabled.");
            ///				return;
        };
        if (!pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
        {
            Console->Hide();
            Console->Execute("demo_record 1");
        }
    }
    break;

#ifdef DEBUG

    // Lain: added TEMP!!!
    case SDL_SCANCODE_UP:
    {
        g_separate_factor /= 0.9f;
        break;
    }
    case SDL_SCANCODE_DOWN:
    {
        g_separate_factor *= 0.9f;
        if (g_separate_factor < 0.1f)
        {
            g_separate_factor = 0.1f;
        }
        break;
    }
    case SDL_SCANCODE_LEFT:
    {
        g_separate_radius *= 0.9f;
        if (g_separate_radius < 0)
        {
            g_separate_radius = 0;
        }
        break;
    }
    case SDL_SCANCODE_RIGHT:
    {
        g_separate_radius /= 0.9f;
        break;
    }

    case SDL_SCANCODE_RETURN:
    {
        bDebug = !bDebug;
        return;
    }
    case SDL_SCANCODE_BACKSPACE:
        if (GameID() == eGameIDSingle)
            GEnv.DRender->NextSceneMode();
        // HW.Caps.SceneMode			= (HW.Caps.SceneMode+1)%3;
        return;

    case SDL_SCANCODE_F4:
    {
        if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LALT))
            break;

        if (pInput->iGetAsyncKeyState(SDL_SCANCODE_RALT))
            break;

        bool bOk = false;
        u32 i = 0, j, n = Objects.o_count();
        if (pCurrentEntity)
            for (; i < n; ++i)
                if (Objects.o_get_by_iterator(i) == pCurrentEntity)
                    break;
        if (i < n)
        {
            j = i;
            bOk = false;
            for (++i; i < n; ++i)
            {
                CEntityAlive* tpEntityAlive = smart_cast<CEntityAlive*>(Objects.o_get_by_iterator(i));
                if (tpEntityAlive)
                {
                    bOk = true;
                    break;
                }
            }
            if (!bOk)
                for (i = 0; i < j; ++i)
                {
                    CEntityAlive* tpEntityAlive = smart_cast<CEntityAlive*>(Objects.o_get_by_iterator(i));
                    if (tpEntityAlive)
                    {
                        bOk = true;
                        break;
                    }
                }
            if (bOk)
            {
                IGameObject* tpObject = CurrentEntity();
                IGameObject* __I = Objects.o_get_by_iterator(i);
                IGameObject** I = &__I;

                SetEntity(*I);
                if (tpObject != *I)
                {
                    CActor* pActor = smart_cast<CActor*>(tpObject);
                    if (pActor)
                        pActor->inventory().Items_SetCurrentEntityHud(false);
                }
                if (tpObject)
                {
                    Engine.Sheduler.Unregister(tpObject);
                    Engine.Sheduler.Register(tpObject, TRUE);
                };
                Engine.Sheduler.Unregister(*I);
                Engine.Sheduler.Register(*I, TRUE);

                CActor* pActor = smart_cast<CActor*>(*I);
                if (pActor)
                {
                    pActor->inventory().Items_SetCurrentEntityHud(true);

                    CHudItem* pHudItem = smart_cast<CHudItem*>(pActor->inventory().ActiveItem());
                    if (pHudItem)
                    {
                        pHudItem->OnStateSwitch(pHudItem->GetState(), pHudItem->GetState());
                    }
                }
            }
        }
        return;
    }
    // Lain: added
    case SDL_SCANCODE_F5:
    {
        if (smart_cast<CBaseMonster*>(CurrentEntity()))
        {
            DBG().log_debug_info();
        }
        break;
    }

    case MOUSE_1:
    {
        if (GameID() != eGameIDSingle)
            break;

        if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LALT))
        {
            if (smart_cast<CActor*>(CurrentEntity()))
                try_change_current_entity();
            else
                restore_actor();
            return;
        }
        break;
    }

#endif
    } // switch (key)
#endif // MASTER_GOLD

    if (g_consoleBindCmds.execute(key))
        return;

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnKeyboardPress(GetBindedAction(key));
    }

#ifdef _DEBUG
    IGameObject* obj = Level().Objects.FindObjectByName("monster");
    if (obj)
    {
        CBaseMonster* monster = smart_cast<CBaseMonster*>(obj);
        if (monster)
            monster->debug_on_key(key);
    }
#endif
}

void CLevel::IR_OnKeyboardRelease(int key)
{
    if (!bReady || g_bDisableAllInput)
        return;

    /* avo: script callback */
    if (g_actor)
    {
        g_actor->callback(GameObject::eKeyRelease)(key);
    }

    if (CurrentGameUI() && CurrentGameUI()->IR_UIOnKeyboardRelease(key))
        return;
    if (game && game->OnKeyboardRelease(GetBindedAction(key)))
        return;

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused())
            return;
    }

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnKeyboardRelease(GetBindedAction(key));
    }
}

void CLevel::IR_OnKeyboardHold(int key)
{
    if (g_bDisableAllInput)
        return;

    /* avo: script callback */
    if (g_actor)
    {
        g_actor->callback(GameObject::eKeyHold)(key);
    }

#ifdef DEBUG
    // Lain: added
    if (key == SDL_SCANCODE_UP)
    {
        static u32 time = Device.dwTimeGlobal;
        if (Device.dwTimeGlobal - time > 20)
        {
            if (smart_cast<CBaseMonster*>(CurrentEntity()))
            {
                DBG().debug_info_up();
                time = Device.dwTimeGlobal;
            }
        }
    }
    else if (key == SDL_SCANCODE_DOWN)
    {
        static u32 time = Device.dwTimeGlobal;
        if (Device.dwTimeGlobal - time > 20)
        {
            if (smart_cast<CBaseMonster*>(CurrentEntity()))
            {
                DBG().debug_info_down();
                time = Device.dwTimeGlobal;
            }
        }
    }

#endif // DEBUG

    if (CurrentGameUI() && CurrentGameUI()->IR_UIOnKeyboardHold(key))
        return;

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused() && !IsDemoPlay())
            return;
    }

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnKeyboardHold(GetBindedAction(key));
    }
}

void CLevel::IR_OnTextInput(pcstr text)
{
    if (!bReady || g_bDisableAllInput)
        return;

    if (CurrentGameUI() && CurrentGameUI()->IR_UIOnTextInput(text))
        return;

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnTextInput(text);
    }
}

void CLevel::IR_OnControllerPress(int key, float x, float y)
{
    if (g_bDisableAllInput)
        return;

    if (key > XR_CONTROLLER_BUTTON_INVALID && key < XR_CONTROLLER_BUTTON_MAX)
    {
        IR_OnKeyboardPress(key);
        return;
    }

    /* avo: script callback */
    if (g_actor)
    {
        g_actor->callback(GameObject::eControllerPress)(key, x, y);
    }

    if (CurrentGameUI() && CurrentGameUI()->IR_UIOnControllerPress(key, x, y))
        return;

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused() && !IsDemoPlay())
            return;
    }

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnControllerPress(GetBindedAction(key), x, y);
    }

}

void CLevel::IR_OnControllerRelease(int key, float x, float y)
{
    if (g_bDisableAllInput)
        return;

    if (key > XR_CONTROLLER_BUTTON_INVALID && key < XR_CONTROLLER_BUTTON_MAX)
    {
        IR_OnKeyboardRelease(key);
        return;
    }

    /* avo: script callback */
    if (g_actor)
    {
        g_actor->callback(GameObject::eControllerRelease)(key, x, y);
    }

    if (CurrentGameUI() && CurrentGameUI()->IR_UIOnControllerRelease(key, x, y))
        return;

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused() && !IsDemoPlay())
            return;
    }

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnControllerRelease(GetBindedAction(key), x, y);
    }
}

void CLevel::IR_OnControllerHold(int key, float x, float y)
{
    if (g_bDisableAllInput)
        return;

    if (key > XR_CONTROLLER_BUTTON_INVALID && key < XR_CONTROLLER_BUTTON_MAX)
    {
        IR_OnKeyboardHold(key);
        return;
    }

    /* avo: script callback */
    if (g_actor)
    {
        g_actor->callback(GameObject::eControllerHold)(key, x, y);
    }

    if (CurrentGameUI() && CurrentGameUI()->IR_UIOnControllerHold(key, x, y))
        return;

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused() && !IsDemoPlay())
            return;
    }

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnControllerHold(GetBindedAction(key), x, y);
    }
}

void CLevel::IR_OnControllerAttitudeChange(Fvector change)
{
    if (g_bDisableAllInput)
        return;

    if (g_actor)
    {
        g_actor->callback(GameObject::eControllerAttitudeChange)(change);
    }

#ifndef MASTER_GOLD
    if (!psActorFlags.test(AF_NO_CLIP))
#endif
    {
        if (Device.Paused() && !IsDemoPlay())
            return;
    }

    if (CURRENT_ENTITY())
    {
        IInputReceiver* IR = smart_cast<IInputReceiver*>(smart_cast<CGameObject*>(CURRENT_ENTITY()));
        if (IR)
            IR->IR_OnControllerAttitudeChange(change);
    }
}

void CLevel::IR_OnActivate()
{
    if (!pInput)
        return;

    for (int i = 0; i < CInput::COUNT_KB_BUTTONS; i++)
    {
        if (IR_GetKeyState(i))
        {
            EGameActions action = GetBindedAction(i);
            switch (action)
            {
            case kFWD:
            case kBACK:
            case kL_STRAFE:
            case kR_STRAFE:
            case kLEFT:
            case kRIGHT:
            case kUP:
            case kDOWN:
            case kCROUCH:
            case kACCEL:
            case kL_LOOKOUT:
            case kR_LOOKOUT:
            case kWPN_FIRE:
                IR_OnKeyboardPress(i);
                break;
            };
        };
    }
}
