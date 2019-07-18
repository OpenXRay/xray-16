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
        keyboard_key& k1 = keyboards[i1];
        if (k1.key_name == NULL)
            break;

        int i2 = i1;
        while (true)
        {
            keyboard_key& k2 = keyboards[i2];
            if (k2.key_name == NULL)
                break;
            if (k1.dik == k2.dik && i1 != i2)
            {
                Msg("%s==%s", k1.key_name, k2.key_name);
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
        keyboard_key& kb = keyboards[idx];
        if (pInput->GetKeyName(kb.dik, buff, sizeof(buff)))
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

pcstr IdToActionName(EGameActions id)
{
    int idx = 0;
    while (actions[idx].action_name)
    {
        if (id == actions[idx].id)
            return actions[idx].action_name;

        ++idx;
    }
    Msg("! cant find corresponding [action_name] for id");
    return NULL;
}

pcstr DikToKeyname(int dik)
{
    keyboard_key* kb = DikToPtr(dik, true);
    if (kb)
        return kb->key_name;
    else
        return NULL;
}

keyboard_key* DikToPtr(int dik, bool safe)
{
    int idx = 0;
    while (keyboards[idx].key_name)
    {
        keyboard_key& kb = keyboards[idx];
        if (kb.dik == dik)
            return &keyboards[idx];

        ++idx;
    }

    if (!safe)
        Msg("! [DikToPtr] cant find corresponding 'keyboard_key' for dik '%d'", dik);

    return NULL;
}

bool IsGroupNotConflicted(EKeyGroup g1, EKeyGroup g2)
{
    return ((g1 == _sp && g2 == _mp) || (g1 == _mp && g2 == _sp));
}

bool is_group_matching(EKeyGroup g1, EKeyGroup g2) 
{
    return ((g1 == g2) || (g1 == _both) || (g2 == _both));
}

EGameActions GetBindedAction(int dik)
{
    for (int idx = 0; idx < bindings_count; ++idx)
    {
        key_binding* binding = &g_key_bindings[idx];

        bool isGroupMatching = is_group_matching(binding->m_action->key_group, g_current_keygroup);

        if (!isGroupMatching)
            continue;

        for (u8 i = 0; i < bindtypes_count && isGroupMatching; ++i)
            if (binding->m_keyboard[i] && binding->m_keyboard[i]->dik == dik)
                return binding->m_action->id;
    }
    return kNOTBINDED;
}

ConsoleBindCmds g_consoleBindCmds;
BOOL g_remapped = false;

class CCC_Bind : public IConsole_Command
{
    int m_workIdx;

public:
    CCC_Bind(LPCSTR n, int idx) : IConsole_Command(n), m_workIdx(idx) {};

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

        if (!g_remapped)
        {
            remap_keys();
            g_remapped = true;
        }

        if (!ActionNameToPtr(action))
            return;

        int actionId = ActionNameToId(action);
        if (actionId == kNOTBINDED)
            return;

        keyboard_key* keyboard = KeynameToPtr(key);
        if (!keyboard)
            return;

        key_binding* currBinding = &g_key_bindings[actionId];

        currBinding->m_keyboard[m_workIdx] = keyboard;

        {
            for (int idx = 0; idx < bindings_count; ++idx)
            {
                key_binding* binding = &g_key_bindings[idx];
                if (binding == currBinding)
                    continue;

                bool isConflict = !IsGroupNotConflicted(binding->m_action->key_group, currBinding->m_action->key_group);

                for (u8 i = 0; i < bindtypes_count; ++i)
                    if (binding->m_keyboard[i] == keyboard && isConflict)
                        binding->m_keyboard[i] = nullptr;
            }
        }

        CStringTable::ReparseKeyBindings();
    }

    virtual void Save(IWriter* f)
    {
        if (m_workIdx == 0)
            f->w_printf("default_controls\r\n");

        for (int idx = 0; idx < bindings_count; ++idx)
        {
            key_binding* binding = &g_key_bindings[idx];
            if (binding->m_keyboard[m_workIdx])
                f->w_printf("%s %s %s\r\n", cName, binding->m_action->action_name, binding->m_keyboard[m_workIdx]->key_name);
        }
    }
};

class CCC_UnBind : public IConsole_Command
{
    int m_workIdx;

public:
    CCC_UnBind(LPCSTR n, int idx) : IConsole_Command(n), m_workIdx(idx) { bEmptyArgsHandled = true; };

    virtual void Execute(LPCSTR args)
    {
        int actionId = ActionNameToId(args);
        key_binding* binding = &g_key_bindings[actionId];
        binding->m_keyboard[m_workIdx] = nullptr;

        CStringTable::ReparseKeyBindings();
    }
};

class CCC_ListActions : public IConsole_Command
{
public:
    CCC_ListActions(LPCSTR n) : IConsole_Command(n) { bEmptyArgsHandled = true; };

    virtual void Execute(LPCSTR args)
    {
        Log("- --- Action list start ---");
        for (int idx = 0; idx < bindings_count; ++idx)
        {
            key_binding* binding = &g_key_bindings[idx];
            Log("-", binding->m_action->action_name);
        }
        Log("- --- Action list end   ---");
    }
};

class CCC_UnBindAll : public IConsole_Command
{
public:
    CCC_UnBindAll(LPCSTR n) : IConsole_Command(n) { bEmptyArgsHandled = true; };

    virtual void Execute(LPCSTR args)
    {
        for (int idx = 0; idx < bindings_count; ++idx)
        {
            key_binding* binding = &g_key_bindings[idx];
            for (u8 i = 0; i < bindtypes_count; ++i)
                binding->m_keyboard[i] = nullptr;
        }
        g_consoleBindCmds.clear();
    }
};

class CCC_DefControls : public CCC_UnBindAll
{
public:
    CCC_DefControls(LPCSTR n) : CCC_UnBindAll(n) {}

    virtual void Execute(LPCSTR args)
    {
        CCC_UnBindAll::Execute(args);
        string_path cfg;
        string_path cmd;
        FS.update_path(cfg, "$game_config$", "default_controls.ltx");
        strconcat(sizeof(cmd), cmd, "cfg_load", " ", cfg);
        Console->Execute(cmd);
    }
};

class CCC_BindList : public IConsole_Command
{
public:
    CCC_BindList(LPCSTR n) : IConsole_Command(n) { bEmptyArgsHandled = true; };

    virtual void Execute(LPCSTR args)
    {
        Log("- --- Bind list start ---");
        string512 buff;

        for (int idx = 0; idx < bindings_count; ++idx)
        {
            key_binding* binding = &g_key_bindings[idx];
            xr_sprintf(buff, "[%s] primary is[%s] secondary is[%s] pad button is[%s]", binding->m_action->action_name,
                (binding->m_keyboard[0]) ? binding->m_keyboard[0]->key_local_name.c_str() : "NULL",
                (binding->m_keyboard[1]) ? binding->m_keyboard[1]->key_local_name.c_str() : "NULL",
                (binding->m_keyboard[2]) ? binding->m_keyboard[2]->key_local_name.c_str() : "NULL");
            Log(buff);
        }
        Log("- --- Bind list end   ---");
    }
};

class CCC_BindConsoleCmd : public IConsole_Command
{
public:
    CCC_BindConsoleCmd(LPCSTR n) : IConsole_Command(n) {};

    virtual void Execute(LPCSTR args)
    {
        string512 consoleCmd;
        string256 key;
        int cnt = _GetItemCount(args, ' ');
        _GetItems(args, 0, cnt - 1, consoleCmd, ' ');
        _GetItem(args, cnt - 1, key, ' ');

        int dik = KeynameToDik(key);
        g_consoleBindCmds.bind(dik, consoleCmd);
    }

    virtual void Save(IWriter* f) { g_consoleBindCmds.save(f); }
};

class CCC_UnBindConsoleCmd : public IConsole_Command
{
public:
    CCC_UnBindConsoleCmd(LPCSTR n) : IConsole_Command(n) { bEmptyArgsHandled = false; };

    virtual void Execute(LPCSTR args)
    {
        int dik = KeynameToDik(args);
        g_consoleBindCmds.unbind(dik);
    }
};

void ConsoleBindCmds::bind(int dik, LPCSTR n)
{
    con_cmd& c = m_bindConsoleCmds[dik];
    c.cmd = n;
}

void ConsoleBindCmds::unbind(int dik)
{
    xr_map<int, con_cmd>::iterator it = m_bindConsoleCmds.find(dik);
    if (it == m_bindConsoleCmds.end())
        return;

    m_bindConsoleCmds.erase(it);
}

void ConsoleBindCmds::clear() { m_bindConsoleCmds.clear(); }

bool ConsoleBindCmds::execute(int dik)
{
    xr_map<int, con_cmd>::iterator it = m_bindConsoleCmds.find(dik);
    if (it == m_bindConsoleCmds.end())
        return false;

    Console->Execute(it->second.cmd.c_str());
    return true;
}

void ConsoleBindCmds::save(IWriter* f)
{
    xr_map<int, con_cmd>::iterator it = m_bindConsoleCmds.begin();

    for (; it != m_bindConsoleCmds.end(); ++it)
    {
        pcstr keyname = DikToKeyname(it->first);
        f->w_printf("bind_console %s %s\n", *it->second.cmd, keyname);
    }
}

void CCC_RegisterInput()
{
    initialize_bindings();
    CMD2(CCC_Bind, "bind", 0);
    CMD2(CCC_Bind, "bind_sec", 1);
    CMD2(CCC_Bind, "bind_gpad", 2);
    CMD2(CCC_UnBind, "unbind", 0);
    CMD2(CCC_UnBind, "unbind_sec", 1);
    CMD2(CCC_UnBind, "unbind_gpad", 2);
    CMD1(CCC_UnBindAll, "unbindall");
    CMD1(CCC_DefControls, "default_controls");
    CMD1(CCC_ListActions, "list_actions");

    CMD1(CCC_BindList, "bind_list");
    CMD1(CCC_BindConsoleCmd, "bind_console");
    CMD1(CCC_UnBindConsoleCmd, "unbind_console");
};
