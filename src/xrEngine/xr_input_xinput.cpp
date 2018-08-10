#include "stdafx.h"
#include <dinput.h>
#include "xr_input_xinput.h"
#include "xr_input.h"
#include "StringTable/IStringTable.h"

ENGINE_API _binding g_key_bindings[bindings_count];
ENGINE_API _key_group g_current_keygroup = _sp;

// clang-format off
_action actions[] = {
    {"left", kLEFT, _both}, {"right", kRIGHT, _both}, {"up", kUP, _both}, {"down", kDOWN, _both},
    {"jump", kJUMP, _both}, {"crouch", kCROUCH, _both}, {"accel", kACCEL, _both},
    {"sprint_toggle", kSPRINT_TOGGLE, _both},

    {"forward", kFWD, _both}, {"back", kBACK, _both}, {"lstrafe", kL_STRAFE, _both}, {"rstrafe", kR_STRAFE, _both},

    {"llookout", kL_LOOKOUT, _both}, {"rlookout", kR_LOOKOUT, _both},

    {"cam_1", kCAM_1, _both}, {"cam_2", kCAM_2, _both}, {"cam_3", kCAM_3, _both}, {"cam_zoom_in", kCAM_ZOOM_IN, _both},
    {"cam_zoom_out", kCAM_ZOOM_OUT, _both},
	{ "flashlight",			kFLASH					,_both},	//Romann
    {"torch", kTORCH, _both}, {"night_vision", kNIGHT_VISION, _both}, {"show_detector", kDETECTOR, _sp},

    {"wpn_1", kWPN_1, _both}, {"wpn_2", kWPN_2, _both}, {"wpn_3", kWPN_3, _both}, {"wpn_4", kWPN_4, _both},
    {"wpn_5", kWPN_5, _both}, {"wpn_6", kWPN_6, _both}, {"artefact", kARTEFACT, _both /*_mp*/},
    {"wpn_next", kWPN_NEXT, _both}, // means next ammo type
    {"wpn_fire", kWPN_FIRE, _both}, {"wpn_zoom", kWPN_ZOOM, _both}, {"wpn_zoom_inc", kWPN_ZOOM_INC, _both},
    {"wpn_zoom_dec", kWPN_ZOOM_DEC, _both}, {"wpn_reload", kWPN_RELOAD, _both}, {"wpn_func", kWPN_FUNC, _both},
    {"wpn_firemode_prev", kWPN_FIREMODE_PREV, _both}, {"wpn_firemode_next", kWPN_FIREMODE_NEXT, _both},

    {"pause", kPAUSE, _both},
    {"drop", kDROP, _both},
    {"use", kUSE, _both},
    {"scores", kSCORES, _both},
    {"chat", kCHAT, _both},
    {"chat_team", kCHAT_TEAM, _both},
    {"screenshot", kSCREENSHOT, _both},
    {"quit", kQUIT, _both},
    {"console", kCONSOLE, _both},
    {"inventory", kINVENTORY, _both},
    {"buy_menu", kBUY, _both},
    {"skin_menu", kSKIN, _both},
    {"team_menu", kTEAM, _both},
    {"active_jobs", kACTIVE_JOBS, _sp},

    {"vote_begin", kVOTE_BEGIN, _both},
    {"show_admin_menu", kSHOW_ADMIN_MENU, _both},
    {"vote", kVOTE, _both},
    {"vote_yes", kVOTEYES, _both},
    {"vote_no", kVOTENO, _both},

    {"next_slot", kNEXT_SLOT, _both},
    {"prev_slot", kPREV_SLOT, _both},

    {"speech_menu_0", kSPEECH_MENU_0, _both},
    {"speech_menu_1", kSPEECH_MENU_1, _both},

    {"quick_use_1", kQUICK_USE_1, _both},
    {"quick_use_2", kQUICK_USE_2, _both},
    {"quick_use_3", kQUICK_USE_3, _both},
    {"quick_use_4", kQUICK_USE_4, _both},

    {"quick_save", kQUICK_SAVE, _sp},
    {"quick_load", kQUICK_LOAD, _sp},

    { "custom1", kCUSTOM1, _sp },
    { "custom2", kCUSTOM2, _sp },
    { "custom3", kCUSTOM3, _sp },
    { "custom4", kCUSTOM4, _sp },
    { "custom5", kCUSTOM5, _sp },
    { "custom6", kCUSTOM6, _sp },
    { "custom7", kCUSTOM7, _sp },
    { "custom8", kCUSTOM8, _sp },
    { "custom9", kCUSTOM9, _sp },
    { "custom10", kCUSTOM10, _sp },
    { "custom11", kCUSTOM11, _sp },
    { "custom12", kCUSTOM12, _sp },
    { "custom13", kCUSTOM13, _sp },
    { "custom14", kCUSTOM14, _sp },
    { "custom15", kCUSTOM15, _sp },
    { "cam_autoaim", kCAM_AUTOAIM, _sp },
    { "pda_tab1", kPDA_TAB1, _sp },
    { "pda_tab2", kPDA_TAB2, _sp },
    { "pda_tab3", kPDA_TAB3, _sp },
    { "pda_tab4", kPDA_TAB4, _sp },
    { "pda_tab5", kPDA_TAB5, _sp },
    { "pda_tab6", kPDA_TAB6, _sp },
#ifdef COC_KICK
    { "kick", kKICK, _sp },
#endif

    {NULL, kLASTACTION, _both}
};

_keyboard keyboards[] = {{"kESCAPE", DIK_ESCAPE}, {"k1", DIK_1}, {"k2", DIK_2}, {"k3", DIK_3}, {"k4", DIK_4},
    {"k5", DIK_5}, {"k6", DIK_6}, {"k7", DIK_7}, {"k8", DIK_8}, {"k9", DIK_9}, {"k0", DIK_0}, {"kMINUS", DIK_MINUS},
    {"kEQUALS", DIK_EQUALS}, {"kBACK", DIK_BACK}, {"kTAB", DIK_TAB}, {"kQ", DIK_Q}, {"kW", DIK_W}, {"kE", DIK_E},
    {"kR", DIK_R}, {"kT", DIK_T}, {"kY", DIK_Y}, {"kU", DIK_U}, {"kI", DIK_I}, {"kO", DIK_O}, {"kP", DIK_P},
    {"kLBRACKET", DIK_LBRACKET}, {"kRBRACKET", DIK_RBRACKET}, {"kRETURN", DIK_RETURN}, {"kLCONTROL", DIK_LCONTROL},
    {"kA", DIK_A}, {"kS", DIK_S}, {"kD", DIK_D}, {"kF", DIK_F}, {"kG", DIK_G}, {"kH", DIK_H}, {"kJ", DIK_J},
    {"kK", DIK_K}, {"kL", DIK_L}, {"kSEMICOLON", DIK_SEMICOLON}, {"kAPOSTROPHE", DIK_APOSTROPHE}, {"kGRAVE", DIK_GRAVE},
    {"kLSHIFT", DIK_LSHIFT}, {"kBACKSLASH", DIK_BACKSLASH}, {"kZ", DIK_Z}, {"kX", DIK_X}, {"kC", DIK_C}, {"kV", DIK_V},
    {"kB", DIK_B}, {"kN", DIK_N}, {"kM", DIK_M}, {"kCOMMA", DIK_COMMA}, {"kPERIOD", DIK_PERIOD}, {"kSLASH", DIK_SLASH},
    {"kRSHIFT", DIK_RSHIFT}, {"kMULTIPLY", DIK_MULTIPLY}, {"kLMENU", DIK_LMENU}, {"kSPACE", DIK_SPACE},
    {"kCAPITAL", DIK_CAPITAL}, {"kF1", DIK_F1}, {"kF2", DIK_F2}, {"kF3", DIK_F3}, {"kF4", DIK_F4}, {"kF5", DIK_F5},
    {"kF6", DIK_F6}, {"kF7", DIK_F7}, {"kF8", DIK_F8}, {"kF9", DIK_F9}, {"kF10", DIK_F10}, {"kNUMLOCK", DIK_NUMLOCK},
    {"kSCROLL", DIK_SCROLL}, {"kNUMPAD7", DIK_NUMPAD7}, {"kNUMPAD8", DIK_NUMPAD8}, {"kNUMPAD9", DIK_NUMPAD9},
    {"kSUBTRACT", DIK_SUBTRACT}, {"kNUMPAD4", DIK_NUMPAD4}, {"kNUMPAD5", DIK_NUMPAD5}, {"kNUMPAD6", DIK_NUMPAD6},
    {"kADD", DIK_ADD}, {"kNUMPAD1", DIK_NUMPAD1}, {"kNUMPAD2", DIK_NUMPAD2}, {"kNUMPAD3", DIK_NUMPAD3},
    {"kNUMPAD0", DIK_NUMPAD0}, {"kDECIMAL", DIK_DECIMAL}, {"kF11", DIK_F11}, {"kF12", DIK_F12}, {"kF13", DIK_F13},
    {"kF14", DIK_F14}, {"kF15", DIK_F15}, {"kKANA", DIK_KANA}, {"kCONVERT", DIK_CONVERT}, {"kNOCONVERT", DIK_NOCONVERT},
    {"kYEN", DIK_YEN}, {"kNUMPADEQUALS", DIK_NUMPADEQUALS}, {"kCIRCUMFLEX", DIK_CIRCUMFLEX}, {"kAT", DIK_AT},
    {"kCOLON", DIK_COLON}, {"kUNDERLINE", DIK_UNDERLINE}, {"kKANJI", DIK_KANJI}, {"kSTOP", DIK_STOP}, {"kAX", DIK_AX},
    {"kUNLABELED", DIK_UNLABELED}, {"kNUMPADENTER", DIK_NUMPADENTER}, {"kRCONTROL", DIK_RCONTROL},
    {"kNUMPADCOMMA", DIK_NUMPADCOMMA}, {"kDIVIDE", DIK_DIVIDE}, {"kSYSRQ", DIK_SYSRQ}, {"kRMENU", DIK_RMENU},
    {"kHOME", DIK_HOME}, {"kUP", DIK_UP}, {"kPRIOR", DIK_PRIOR}, {"kLEFT", DIK_LEFT}, {"kRIGHT", DIK_RIGHT},
    {"kEND", DIK_END}, {"kDOWN", DIK_DOWN}, {"kNEXT", DIK_NEXT}, {"kINSERT", DIK_INSERT}, {"kDELETE", DIK_DELETE},
    {"kLWIN", DIK_LWIN}, {"kRWIN", DIK_RWIN}, {"kAPPS", DIK_APPS}, {"kPAUSE", DIK_PAUSE}, {"mouse1", MOUSE_1},
    {"mouse2", MOUSE_2}, {"mouse3", MOUSE_3}, {"mouse4", MOUSE_4}, {"mouse5", MOUSE_5}, {"mouse6", MOUSE_6},
    {"mouse7", MOUSE_7}, {"mouse8", MOUSE_8}, {NULL, 0}
};
// clang-format on

EGameActions action_name_to_id(pcstr _name)
{
    _action* action = action_name_to_ptr(_name);
    if (action)
        return action->id;
    else
        return kNOTBINDED;
}

_action* action_name_to_ptr(pcstr _name)
{
    int idx = 0;
    while (actions[idx].action_name)
    {
        if (!xr_stricmp(_name, actions[idx].action_name))
            return &actions[idx];
        ++idx;
    }
    Msg("! cant find corresponding [id] for '%s'", _name);
    return NULL;
}

bool is_binded(EGameActions _action_id, int _dik)
{
    _binding* pbinding = &g_key_bindings[_action_id];
    if (pbinding->m_keyboard[0] && pbinding->m_keyboard[0]->dik == _dik)
        return true;

    if (pbinding->m_keyboard[1] && pbinding->m_keyboard[1]->dik == _dik)
        return true;

    return false;
}

int get_action_dik(EGameActions _action_id, int idx)
{
    _binding* pbinding = &g_key_bindings[_action_id];

    if (idx == -1)
    {
        if (pbinding->m_keyboard[0])
            return pbinding->m_keyboard[0]->dik;

        if (pbinding->m_keyboard[1])
            return pbinding->m_keyboard[1]->dik;
    }
    else
    {
        if (pbinding->m_keyboard[idx])
            return pbinding->m_keyboard[idx]->dik;
    }
    return 0;
}

int keyname_to_dik(pcstr _name)
{
    _keyboard* _kb = keyname_to_ptr(_name);
    return _kb->dik;
}

_keyboard* keyname_to_ptr(pcstr _name)
{
    int idx = 0;
    while (keyboards[idx].key_name)
    {
        _keyboard& kb = keyboards[idx];
        if (!xr_stricmp(_name, kb.key_name))
            return &keyboards[idx];
        ++idx;
    }

    Msg("! cant find corresponding [_keyboard*] for keyname %s", _name);
    return NULL;
}

void GetActionAllBinding(LPCSTR _action, char* dst_buff, int dst_buff_sz)
{
    int action_id = action_name_to_id(_action);
    _binding* pbinding = &g_key_bindings[action_id];

    string128 prim;
    string128 sec;
    prim[0] = 0;
    sec[0] = 0;

    if (pbinding->m_keyboard[0])
    {
        xr_strcpy(prim, pbinding->m_keyboard[0]->key_local_name.c_str());
    }
    if (pbinding->m_keyboard[1])
    {
        xr_strcpy(sec, pbinding->m_keyboard[1]->key_local_name.c_str());
    }
    if (NULL == pbinding->m_keyboard[0] && NULL == pbinding->m_keyboard[1])
    {
        xr_sprintf(dst_buff, dst_buff_sz, "%s", gStringTable->translate("st_key_notbinded").c_str());
    }
    else
        xr_sprintf(
            dst_buff, dst_buff_sz, "%s%s%s", prim[0] ? prim : "", (sec[0] && prim[0]) ? " , " : "", sec[0] ? sec : "");
}
