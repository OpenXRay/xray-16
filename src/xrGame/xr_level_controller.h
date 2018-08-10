#pragma once
#include "xrCore/xrstring.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_map.h"
#include "xrEngine/xr_input_xinput.h"

bool is_group_not_conflicted(_key_group g1, _key_group g2);

pcstr dik_to_keyname(int _dik);
_keyboard* dik_to_ptr(int _dik, bool bSafe);

pcstr id_to_action_name(EGameActions _id);
EGameActions get_binded_action(int dik);

extern void CCC_RegisterInput();

struct _conCmd
{
    shared_str cmd;
};

class ConsoleBindCmds
{
public:
    xr_map<int, _conCmd> m_bindConsoleCmds;

    void bind(int dik, LPCSTR N);
    void unbind(int dik);
    bool execute(int dik);
    void clear();
    void save(IWriter* F);
};

extern ConsoleBindCmds bindConsoleCmds;
