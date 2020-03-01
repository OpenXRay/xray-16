#pragma once
#include "xrCore/xrstring.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_map.h"
#include "xrEngine/xr_input_xinput.h"

bool IsGroupNotConflicted(EKeyGroup g1, EKeyGroup g2);

pcstr DikToKeyname(int dik);
keyboard_key* DikToPtr(int dik, bool safe);

pcstr IdToActionName(EGameActions id);
EGameActions GetBindedAction(int dik);

extern void CCC_RegisterInput();

struct con_cmd
{
    shared_str cmd;
};

class ConsoleBindCmds
{
public:
    xr_map<int, con_cmd> m_bindConsoleCmds;

    void bind(int dik, LPCSTR N);
    void unbind(int dik);
    bool execute(int dik);
    void clear();
    void save(IWriter* F);
};

extern ConsoleBindCmds g_consoleBindCmds;
