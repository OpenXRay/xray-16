#include "stdafx.h"
#include <SDL.h>
#include "xrEngine/xr_ioconsole.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "xr_level_controller.h"
#include "string_table.h"

_binding g_key_bindings[bindings_count];
_key_group g_current_keygroup = _sp;

_action actions[] = {{"left", kLEFT, _both}, {"right", kRIGHT, _both}, {"up", kUP, _both}, {"down", kDOWN, _both},
    {"jump", kJUMP, _both}, {"crouch", kCROUCH, _both}, {"accel", kACCEL, _both},
    {"sprint_toggle", kSPRINT_TOGGLE, _both},

    {"forward", kFWD, _both}, {"back", kBACK, _both}, {"lstrafe", kL_STRAFE, _both}, {"rstrafe", kR_STRAFE, _both},

    {"llookout", kL_LOOKOUT, _both}, {"rlookout", kR_LOOKOUT, _both},

    {"cam_1", kCAM_1, _both}, {"cam_2", kCAM_2, _both}, {"cam_3", kCAM_3, _both}, {"cam_zoom_in", kCAM_ZOOM_IN, _both},
    {"cam_zoom_out", kCAM_ZOOM_OUT, _both},

    {"torch", kTORCH, _both}, {"night_vision", kNIGHT_VISION, _both}, {"show_detector", kDETECTOR, _sp},

    {"wpn_1", kWPN_1, _both}, {"wpn_2", kWPN_2, _both}, {"wpn_3", kWPN_3, _both}, {"wpn_4", kWPN_4, _both},
    {"wpn_5", kWPN_5, _both}, {"wpn_6", kWPN_6, _both}, {"artefact", kARTEFACT, _both /*_mp*/},
    {"wpn_next", kWPN_NEXT, _both}, // means next ammo type
    {"wpn_fire", kWPN_FIRE, _both}, {"wpn_zoom", kWPN_ZOOM, _both}, {"wpn_zoom_inc", kWPN_ZOOM_INC, _both},
    {"wpn_zoom_dec", kWPN_ZOOM_DEC, _both}, {"wpn_reload", kWPN_RELOAD, _both}, {"wpn_func", kWPN_FUNC, _both},
    {"wpn_firemode_prev", kWPN_FIREMODE_PREV, _both}, {"wpn_firemode_next", kWPN_FIREMODE_NEXT, _both},

    {"pause", kPAUSE, _both}, {"drop", kDROP, _both}, {"use", kUSE, _both}, {"scores", kSCORES, _both},
    {"chat", kCHAT, _mp}, {"chat_team", kCHAT_TEAM, _mp}, {"screenshot", kSCREENSHOT, _both}, {"quit", kQUIT, _both},
    {"console", kCONSOLE, _both}, {"inventory", kINVENTORY, _both}, {"buy_menu", kBUY, _mp}, {"skin_menu", kSKIN, _mp},
    {"team_menu", kTEAM, _mp}, {"active_jobs", kACTIVE_JOBS, _sp},

    {"vote_begin", kVOTE_BEGIN, _mp}, {"show_admin_menu", kSHOW_ADMIN_MENU, _mp}, {"vote", kVOTE, _mp},
    {"vote_yes", kVOTEYES, _mp}, {"vote_no", kVOTENO, _mp},

    {"next_slot", kNEXT_SLOT, _both}, {"prev_slot", kPREV_SLOT, _both},

    {"speech_menu_0", kSPEECH_MENU_0, _mp}, {"speech_menu_1", kSPEECH_MENU_1, _mp},

    {"quick_use_1", kQUICK_USE_1, _both}, {"quick_use_2", kQUICK_USE_2, _both}, {"quick_use_3", kQUICK_USE_3, _both},
    {"quick_use_4", kQUICK_USE_4, _both},

    {"quick_save", kQUICK_SAVE, _sp}, {"quick_load", kQUICK_LOAD, _sp},
    //	{ "alife_command",		kALIFE_CMD				,_sp},

    {NULL, kLASTACTION, _both}};

_keyboard keyboards[] = {{"kESCAPE", SDL_SCANCODE_ESCAPE}, {"k1", SDL_SCANCODE_1}, {"k2", SDL_SCANCODE_2},
    {"k3", SDL_SCANCODE_3}, {"k4", SDL_SCANCODE_4}, {"k5", SDL_SCANCODE_5}, {"k6", SDL_SCANCODE_6},
    {"k7", SDL_SCANCODE_7}, {"k8", SDL_SCANCODE_8}, {"k9", SDL_SCANCODE_9}, {"k0", SDL_SCANCODE_0},
    {"kMINUS", SDL_SCANCODE_MINUS}, {"kEQUALS", SDL_SCANCODE_EQUALS}, {"kBACK", SDL_SCANCODE_BACKSPACE},
    {"kTAB", SDL_SCANCODE_TAB}, {"kQ", SDL_SCANCODE_Q}, {"kW", SDL_SCANCODE_W}, {"kE", SDL_SCANCODE_E},
    {"kR", SDL_SCANCODE_R}, {"kT", SDL_SCANCODE_T}, {"kY", SDL_SCANCODE_Y}, {"kU", SDL_SCANCODE_U},
    {"kI", SDL_SCANCODE_I}, {"kO", SDL_SCANCODE_O}, {"kP", SDL_SCANCODE_P}, {"kLBRACKET", SDL_SCANCODE_LEFTBRACKET},
    {"kRBRACKET", SDL_SCANCODE_RIGHTBRACKET}, {"kRETURN", SDL_SCANCODE_RETURN}, {"kLCONTROL", SDL_SCANCODE_LCTRL},
    {"kA", SDL_SCANCODE_A}, {"kS", SDL_SCANCODE_S}, {"kD", SDL_SCANCODE_D}, {"kF", SDL_SCANCODE_F},
    {"kG", SDL_SCANCODE_G}, {"kH", SDL_SCANCODE_H}, {"kJ", SDL_SCANCODE_J}, {"kK", SDL_SCANCODE_K},
    {"kL", SDL_SCANCODE_L}, {"kSEMICOLON", SDL_SCANCODE_SEMICOLON}, {"kAPOSTROPHE", SDL_SCANCODE_APOSTROPHE},
    {"kGRAVE", SDL_SCANCODE_GRAVE}, {"kLSHIFT", SDL_SCANCODE_LSHIFT}, {"kBACKSLASH", SDL_SCANCODE_BACKSLASH},
    {"kZ", SDL_SCANCODE_Z}, {"kX", SDL_SCANCODE_X}, {"kC", SDL_SCANCODE_C}, {"kV", SDL_SCANCODE_V},
    {"kB", SDL_SCANCODE_B}, {"kN", SDL_SCANCODE_N}, {"kM", SDL_SCANCODE_M}, {"kCOMMA", SDL_SCANCODE_COMMA},
    {"kPERIOD", SDL_SCANCODE_PERIOD}, {"kSLASH", SDL_SCANCODE_SLASH}, {"kRSHIFT", SDL_SCANCODE_RSHIFT},
    {"kMULTIPLY", SDL_SCANCODE_KP_MULTIPLY}, {"kLMENU", SDL_SCANCODE_LALT}, {"kSPACE", SDL_SCANCODE_SPACE},
    {"kCAPITAL", SDL_SCANCODE_CAPSLOCK}, {"kF1", SDL_SCANCODE_F1}, {"kF2", SDL_SCANCODE_F2}, {"kF3", SDL_SCANCODE_F3},
    {"kF4", SDL_SCANCODE_F4}, {"kF5", SDL_SCANCODE_F5}, {"kF6", SDL_SCANCODE_F6}, {"kF7", SDL_SCANCODE_F7},
    {"kF8", SDL_SCANCODE_F8}, {"kF9", SDL_SCANCODE_F9}, {"kF10", SDL_SCANCODE_F10},
    {"kNUMLOCK", SDL_SCANCODE_NUMLOCKCLEAR}, {"kSCROLL", SDL_SCANCODE_SCROLLLOCK}, {"kNUMPAD7", SDL_SCANCODE_KP_7},
    {"kNUMPAD8", SDL_SCANCODE_KP_8}, {"kNUMPAD9", SDL_SCANCODE_KP_9}, {"kSUBTRACT", SDL_SCANCODE_KP_MINUS},
    {"kNUMPAD4", SDL_SCANCODE_KP_4}, {"kNUMPAD5", SDL_SCANCODE_KP_5}, {"kNUMPAD6", SDL_SCANCODE_KP_6},
    {"kADD", SDL_SCANCODE_KP_PLUS}, {"kNUMPAD1", SDL_SCANCODE_KP_1}, {"kNUMPAD2", SDL_SCANCODE_KP_2},
    {"kNUMPAD3", SDL_SCANCODE_KP_3}, {"kNUMPAD0", SDL_SCANCODE_KP_0}, {"kDECIMAL", SDL_SCANCODE_KP_DECIMAL},
    {"kF11", SDL_SCANCODE_F11}, {"kF12", SDL_SCANCODE_F12}, {"kF13", SDL_SCANCODE_F13}, {"kF14", SDL_SCANCODE_F14},
    {"kF15", SDL_SCANCODE_F15}, {"kNUMPADEQUALS", SDL_SCANCODE_KP_EQUALS}, {"kCIRCUMFLEX", SDL_SCANCODE_PAGEDOWN},
    {"kAT", SDL_SCANCODE_KP_AT}, {"kCOLON", SDL_SCANCODE_KP_COLON}, {"kSTOP", SDL_SCANCODE_STOP},
    {"kRCONTROL", SDL_SCANCODE_RCTRL}, {"kNUMPADCOMMA", SDL_SCANCODE_KP_COMMA}, {"kDIVIDE", SDL_SCANCODE_KP_DIVIDE},
    {"kSYSRQ", SDL_SCANCODE_SYSREQ}, {"kRMENU", SDL_SCANCODE_RALT}, {"kHOME", SDL_SCANCODE_HOME},
    {"kUP", SDL_SCANCODE_UP}, {"kPRIOR", SDL_SCANCODE_PAGEUP}, {"kLEFT", SDL_SCANCODE_LEFT},
    {"kRIGHT", SDL_SCANCODE_RIGHT}, {"kEND", SDL_SCANCODE_END}, {"kDOWN", SDL_SCANCODE_DOWN},
    {"kNEXT", SDL_SCANCODE_AUDIONEXT}, {"kINSERT", SDL_SCANCODE_INSERT}, {"kDELETE", SDL_SCANCODE_DELETE},
    {"kLWIN", SDL_SCANCODE_LGUI}, {"kRWIN", SDL_SCANCODE_RGUI}, {"kAPPS", SDL_SCANCODE_APPLICATION},
    {"kPAUSE", SDL_SCANCODE_PAUSE}, 
    {"mouse1", MOUSE_1}, {"mouse2", MOUSE_2}, {"mouse3", MOUSE_3}, {"mouse4", MOUSE_4},
    {"mouse5", MOUSE_5}, {"mouse6", MOUSE_6}, {"mouse7", MOUSE_7}, {"mouse8", MOUSE_8}, 
    {NULL, SDL_SCANCODE_UNKNOWN}};

void initialize_bindings()
{
#ifdef DEBUG
    int i1 = 0;
    while (true)
    {
        _keyboard& _k1 = keyboards[i1];
        if (_k1.key_name == NULL)
            break;
        int i2 = i1;
        while (true)
        {
            _keyboard& _k2 = keyboards[i2];
            if (_k2.key_name == NULL)
                break;
            if (_k1.dik == _k2.dik && i1 != i2)
            {
                Msg("%s==%s", _k1.key_name, _k2.key_name);
            }
            ++i2;
        }
        ++i1;
    }
#endif

    for (int idx = 0; idx < bindings_count; ++idx)
        g_key_bindings[idx].m_action = &actions[idx];
}

void remap_keys()
{
    int idx = 0;
    string128 buff;
    while (keyboards[idx].key_name)
    {
        buff[0] = 0;
        _keyboard& kb = keyboards[idx];
        bool res = pInput->get_key_name(kb.dik, buff, sizeof(buff));
        if (res)
            kb.key_local_name = buff;
        else
            kb.key_local_name = kb.key_name;

        //.		Msg("[%s]-[%s]",kb.key_name, kb.key_local_name.c_str());
        ++idx;
    }
}

pcstr id_to_action_name(EGameActions _id)
{
    int idx = 0;
    while (actions[idx].action_name)
    {
        if (_id == actions[idx].id)
            return actions[idx].action_name;
        ++idx;
    }
    Msg("! cant find corresponding [action_name] for id");
    return NULL;
}

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

pcstr dik_to_keyname(SDL_Scancode _dik)
{
    _keyboard* kb = dik_to_ptr(_dik, true);
    if (kb)
        return kb->key_name;
    else
        return NULL;
}

_keyboard* dik_to_ptr(SDL_Scancode _dik, bool bSafe)
{
    int idx = 0;
    while (keyboards[idx].key_name)
    {
        _keyboard& kb = keyboards[idx];
        if (kb.dik == _dik)
            return &keyboards[idx];
        ++idx;
    }
    if (!bSafe)
        Msg("! cant find corresponding [_keyboard] for dik");
    return NULL;
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

bool is_group_not_conflicted(_key_group g1, _key_group g2)
{
    return ((g1 == _sp && g2 == _mp) || (g1 == _mp && g2 == _sp));
}

bool is_group_matching(_key_group g1, _key_group g2) { return ((g1 == g2) || (g1 == _both) || (g2 == _both)); }
bool is_binded(EGameActions _action_id, SDL_Scancode _dik)
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
    return SDL_SCANCODE_UNKNOWN;
}

EGameActions get_binded_action(SDL_Scancode _dik)
{
    for (int idx = 0; idx < bindings_count; ++idx)
    {
        _binding* binding = &g_key_bindings[idx];

        bool b_is_group_matching = is_group_matching(binding->m_action->key_group, g_current_keygroup);

        if (!b_is_group_matching)
            continue;

        if (binding->m_keyboard[0] && binding->m_keyboard[0]->dik == _dik && b_is_group_matching)
            return binding->m_action->id;

        if (binding->m_keyboard[1] && binding->m_keyboard[1]->dik == _dik && b_is_group_matching)
            return binding->m_action->id;
    }
    return kNOTBINDED;
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
        xr_sprintf(dst_buff, dst_buff_sz, "%s", CStringTable().translate("st_key_notbinded").c_str());
    }
    else
        xr_sprintf(
            dst_buff, dst_buff_sz, "%s%s%s", prim[0] ? prim : "", (sec[0] && prim[0]) ? " , " : "", sec[0] ? sec : "");
}

ConsoleBindCmds bindConsoleCmds;
BOOL bRemapped = FALSE;

class CCC_Bind : public IConsole_Command
{
    int m_work_idx;

public:
    CCC_Bind(LPCSTR N, int idx) : IConsole_Command(N), m_work_idx(idx){};
    virtual void Execute(LPCSTR args)
    {
        string256 action;
        string256 key;
        *action = 0;
        *key = 0;

        sscanf(args, "%s %s", action, key);
        if (!*action)
            return;

        if (!*key)
            return;

        if (!bRemapped)
        {
            remap_keys();
            bRemapped = TRUE;
        }

        if (!action_name_to_ptr(action))
            return;

        int action_id = action_name_to_id(action);
        if (action_id == kNOTBINDED)
            return;

        _keyboard* pkeyboard = keyname_to_ptr(key);
        if (!pkeyboard)
            return;

        _binding* curr_pbinding = &g_key_bindings[action_id];

        curr_pbinding->m_keyboard[m_work_idx] = pkeyboard;

        {
            for (int idx = 0; idx < bindings_count; ++idx)
            {
                _binding* binding = &g_key_bindings[idx];
                if (binding == curr_pbinding)
                    continue;

                bool b_conflict =
                    !is_group_not_conflicted(binding->m_action->key_group, curr_pbinding->m_action->key_group);

                if (binding->m_keyboard[0] == pkeyboard && b_conflict)
                    binding->m_keyboard[0] = NULL;

                if (binding->m_keyboard[1] == pkeyboard && b_conflict)
                    binding->m_keyboard[1] = NULL;
            }
        }

        CStringTable::ReparseKeyBindings();
    }
    virtual void Save(IWriter* F)
    {
        if (m_work_idx == 0)
            F->w_printf("default_controls\r\n");

        for (int idx = 0; idx < bindings_count; ++idx)
        {
            _binding* pbinding = &g_key_bindings[idx];
            if (pbinding->m_keyboard[m_work_idx])
            {
                F->w_printf(
                    "%s %s %s\r\n", cName, pbinding->m_action->action_name, pbinding->m_keyboard[m_work_idx]->key_name);
            }
        }
    }
};

class CCC_UnBind : public IConsole_Command
{
    int m_work_idx;

public:
    CCC_UnBind(LPCSTR N, int idx) : IConsole_Command(N), m_work_idx(idx) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR args)
    {
        int action_id = action_name_to_id(args);
        _binding* pbinding = &g_key_bindings[action_id];
        pbinding->m_keyboard[m_work_idx] = NULL;

        CStringTable::ReparseKeyBindings();
    }
};

class CCC_ListActions : public IConsole_Command
{
public:
    CCC_ListActions(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR args)
    {
        Log("- --- Action list start ---");
        for (int idx = 0; idx < bindings_count; ++idx)
        {
            _binding* pbinding = &g_key_bindings[idx];
            Log("-", pbinding->m_action->action_name);
        }
        Log("- --- Action list end   ---");
    }
};

class CCC_UnBindAll : public IConsole_Command
{
public:
    CCC_UnBindAll(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR args)
    {
        for (int idx = 0; idx < bindings_count; ++idx)
        {
            _binding* pbinding = &g_key_bindings[idx];
            pbinding->m_keyboard[0] = NULL;
            pbinding->m_keyboard[1] = NULL;
        }
        bindConsoleCmds.clear();
    }
};

class CCC_DefControls : public CCC_UnBindAll
{
public:
    CCC_DefControls(LPCSTR N) : CCC_UnBindAll(N) {}
    virtual void Execute(LPCSTR args)
    {
        CCC_UnBindAll::Execute(args);
        string_path _cfg;
        string_path cmd;
        FS.update_path(_cfg, "$game_config$", "default_controls.ltx");
        strconcat(sizeof(cmd), cmd, "cfg_load", " ", _cfg);
        Console->Execute(cmd);
    }
};

class CCC_BindList : public IConsole_Command
{
public:
    CCC_BindList(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR args)
    {
        Log("- --- Bind list start ---");
        string512 buff;

        for (int idx = 0; idx < bindings_count; ++idx)
        {
            _binding* pbinding = &g_key_bindings[idx];
            xr_sprintf(buff, "[%s] primary is[%s] secondary is[%s]", pbinding->m_action->action_name,
                (pbinding->m_keyboard[0]) ? pbinding->m_keyboard[0]->key_local_name.c_str() : "NULL",
                (pbinding->m_keyboard[1]) ? pbinding->m_keyboard[1]->key_local_name.c_str() : "NULL");
            Log(buff);
        }
        Log("- --- Bind list end   ---");
    }
};

class CCC_BindConsoleCmd : public IConsole_Command
{
public:
    CCC_BindConsoleCmd(LPCSTR N) : IConsole_Command(N){};
    virtual void Execute(LPCSTR args)
    {
        string512 console_command;
        string256 key;
        int cnt = _GetItemCount(args, ' ');
        _GetItems(args, 0, cnt - 1, console_command, ' ');
        _GetItem(args, cnt - 1, key, ' ');

        int dik = keyname_to_dik(key);
        bindConsoleCmds.bind(dik, console_command);
    }

    virtual void Save(IWriter* F) { bindConsoleCmds.save(F); }
};

class CCC_UnBindConsoleCmd : public IConsole_Command
{
public:
    CCC_UnBindConsoleCmd(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = FALSE; };
    virtual void Execute(LPCSTR args)
    {
        int _dik = keyname_to_dik(args);
        bindConsoleCmds.unbind(_dik);
    }
};

void ConsoleBindCmds::bind(int dik, LPCSTR N)
{
    _conCmd& c = m_bindConsoleCmds[dik];
    c.cmd = N;
}
void ConsoleBindCmds::unbind(int dik)
{
    xr_map<int, _conCmd>::iterator it = m_bindConsoleCmds.find(dik);
    if (it == m_bindConsoleCmds.end())
        return;

    m_bindConsoleCmds.erase(it);
}

void ConsoleBindCmds::clear() { m_bindConsoleCmds.clear(); }
bool ConsoleBindCmds::execute(int dik)
{
    xr_map<int, _conCmd>::iterator it = m_bindConsoleCmds.find(dik);
    if (it == m_bindConsoleCmds.end())
        return false;

    Console->Execute(it->second.cmd.c_str());
    return true;
}

void ConsoleBindCmds::save(IWriter* F)
{
    xr_map<int, _conCmd>::iterator it = m_bindConsoleCmds.begin();

    for (; it != m_bindConsoleCmds.end(); ++it)
    {
        pcstr keyname = dik_to_keyname((SDL_Scancode) it->first);
        F->w_printf("bind_console %s %s\n", *it->second.cmd, keyname);
    }
}

void CCC_RegisterInput()
{
    initialize_bindings();
    CMD2(CCC_Bind, "bind", 0);
    CMD2(CCC_Bind, "bind_sec", 1);
    CMD2(CCC_UnBind, "unbind", 0);
    CMD2(CCC_UnBind, "unbind_sec", 1);
    CMD1(CCC_UnBindAll, "unbindall");
    CMD1(CCC_DefControls, "default_controls");
    CMD1(CCC_ListActions, "list_actions");

    CMD1(CCC_BindList, "bind_list");
    CMD1(CCC_BindConsoleCmd, "bind_console");
    CMD1(CCC_UnBindConsoleCmd, "unbind_console");
};
