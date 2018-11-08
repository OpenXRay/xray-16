#include "StdAfx.h"
#include "SDL.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "xr_level_controller.h"
#include "string_table.h"

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
    string128 buff;
    // Log("Keys remap:");
    for (int idx = 0; keyboards[idx].key_name; ++idx)
    {
        buff[0] = 0;
        _keyboard& kb = keyboards[idx];
        if (pInput->get_dik_name(kb.dik, buff, sizeof(buff)))
            kb.key_local_name = buff;
        else
        {
            Msg("! Can't find a key name for %s", kb.key_name);
            if (kb.key_local_name.empty())
                kb.key_local_name = kb.key_name;
        }

        // Msg("[%s]-[%s]", kb.key_name, kb.key_local_name.c_str());
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

pcstr dik_to_keyname(int _dik)
{
    _keyboard* kb = dik_to_ptr(_dik, true);
    if (kb)
        return kb->key_name;
    else
        return NULL;
}

_keyboard* dik_to_ptr(int _dik, bool bSafe)
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
        Msg("! [dik_to_ptr] cant find corresponding '_keyboard' for dik '%d'", _dik);
    return NULL;
}

bool is_group_not_conflicted(_key_group g1, _key_group g2)
{
    return ((g1 == _sp && g2 == _mp) || (g1 == _mp && g2 == _sp));
}

bool is_group_matching(_key_group g1, _key_group g2) { return ((g1 == g2) || (g1 == _both) || (g2 == _both)); }

EGameActions get_binded_action(int _dik)
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
        pcstr keyname = dik_to_keyname(it->first);
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
