#pragma once
#include "xrCore/xrstring.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_map.h"
#include "xrEngine/xr_input.h" // Don't remove this include

enum EGameActions : u32
{
    kLOOK_AROUND, // gamepad
    kLEFT,
    kRIGHT,
    kUP,
    kDOWN,

    kMOVE_AROUND, // gamepad
    kFWD,
    kBACK,
    kL_STRAFE,
    kR_STRAFE,

    kL_LOOKOUT,
    kR_LOOKOUT,

    kJUMP,
    kCROUCH,
    kCROUCH_TOGGLE,
    kACCEL,
    kSPRINT_TOGGLE,

    kENGINE,

    kCAM_1,
    kCAM_2,
    kCAM_3,
    kCAM_4,
    kCAM_ZOOM_IN,
    kCAM_ZOOM_OUT,
    kCAM_AUTOAIM,

    kTORCH,
    kNIGHT_VISION,
    kDETECTOR,
    kWPN_1,
    kWPN_2,
    kWPN_3,
    kWPN_4,
    kWPN_5,
    kWPN_6,
    kARTEFACT,
    kWPN_NEXT,
    kWPN_FIRE,
    kWPN_ZOOM,
    kWPN_ZOOM_INC,
    kWPN_ZOOM_DEC,
    kWPN_RELOAD,
    kWPN_FUNC,
    kWPN_FIREMODE_PREV,
    kWPN_FIREMODE_NEXT,

    kPAUSE,
    kDROP,
    kUSE,
    kSCORES,
    kCHAT,
    kCHAT_TEAM,
    kSCREENSHOT,
    kENTER,
    kQUIT,
    kCONSOLE,
    kINVENTORY,
    kBUY,
    kSKIN,
    kTEAM,
    kACTIVE_JOBS,
    kMAP,
    kCONTACTS,
    kEXT_1,

    kVOTE_BEGIN,
    kSHOW_ADMIN_MENU,
    kVOTE,
    kVOTEYES,
    kVOTENO,

    kNEXT_SLOT,
    kPREV_SLOT,

    kSPEECH_MENU_0,
    kSPEECH_MENU_1,
    kSPEECH_MENU_2,
    kSPEECH_MENU_3,
    kSPEECH_MENU_4,
    kSPEECH_MENU_5,
    kSPEECH_MENU_6,
    kSPEECH_MENU_7,
    kSPEECH_MENU_8,
    kSPEECH_MENU_9,

    kUSE_BANDAGE,
    kUSE_MEDKIT,

    kQUICK_USE_1,
    kQUICK_USE_2,
    kQUICK_USE_3,
    kQUICK_USE_4,

    kQUICK_SAVE,
    kQUICK_LOAD,
    kALIFE_CMD,

    kNUMPAD0,
    kINSERT,
    kNUMPAD1,
    kNUMPAD2,
    kNUMPAD3,
    kNUMPAD4,
    kNUMPAD5,
    kNUMPAD6,
    kNUMPAD7,
    kNUMPAD8,
    kNUMPAD9,
    kNUMPADENTER,

    kCUSTOM1,
    kCUSTOM2,
    kCUSTOM3,
    kCUSTOM4,
    kCUSTOM5,
    kCUSTOM6,
    kCUSTOM7,
    kCUSTOM8,
    kCUSTOM9,
    kCUSTOM10,
    kCUSTOM11,
    kCUSTOM12,
    kCUSTOM13,
    kCUSTOM14,
    kCUSTOM15,

    kPDA_TAB1,
    kPDA_TAB2,
    kPDA_TAB3,
    kPDA_TAB4,
    kPDA_TAB5,
    kPDA_TAB6,

    kKICK, // alpet: kick dynamic objects

    kEDITOR,

    // Contextual actions:
    // UI
    kUI_MOVE,
    kUI_MOVE_LEFT,
    kUI_MOVE_RIGHT,
    kUI_MOVE_UP,
    kUI_MOVE_DOWN,

    kUI_MOVE_SECONDARY,

    kUI_ACCEPT,
    kUI_BACK,
    kUI_ACTION_1,
    kUI_ACTION_2,

    kUI_TAB_PREV,
    kUI_TAB_NEXT,

    kUI_BUTTON_1,
    kUI_BUTTON_2,
    kUI_BUTTON_3,
    kUI_BUTTON_4,
    kUI_BUTTON_5,
    kUI_BUTTON_6,
    kUI_BUTTON_7,
    kUI_BUTTON_8,
    kUI_BUTTON_9,
    kUI_BUTTON_0,

    // PDA:
    kPDA_MAP_MOVE, // gamepad axis
    kPDA_MAP_MOVE_LEFT,
    kPDA_MAP_MOVE_RIGHT,
    kPDA_MAP_MOVE_UP,
    kPDA_MAP_MOVE_DOWN,

    kPDA_MAP_ZOOM_IN,
    kPDA_MAP_ZOOM_OUT,
    kPDA_MAP_ZOOM_RESET,

    kPDA_MAP_SHOW_ACTOR,
    kPDA_MAP_SHOW_LEGEND,

    kPDA_FILTER_TOGGLE,
    kPDA_TASKS_TOGGLE,

    // Talk:
    kTALK_SWITCH_TO_TRADE, // _OR_UPGRADE
    kTALK_LOG_SCROLL,
    kTALK_LOG_SCROLL_UP,
    kTALK_LOG_SCROLL_DOWN,

    kLASTACTION,
    kNOTBINDED
};

enum class EKeyContext
{
    Undefined = 0, // default behaviour
    UI, // UI overall
    PDA,
    Talk,
};

constexpr char GAME_ACTION_MARK = 27; // escape symbol

struct keyboard_key
{
    pcstr key_name;
    int dik;
    xr_string key_local_name;
};

enum EKeyGroup
{
    _both = (1 << 0),
    _sp = _both | (1 << 1),
    _mp = _both | (1 << 2),
};

struct game_action
{
    pcstr action_name;
    EGameActions id;
    EKeyGroup key_group;
    EKeyContext key_context{ EKeyContext::Undefined };
};

#define bindtypes_count 3
struct key_binding
{
    game_action* m_action;
    keyboard_key* m_keyboard[bindtypes_count];
};

extern ENGINE_API EKeyGroup g_current_keygroup;

extern ENGINE_API game_action actions[];
extern ENGINE_API keyboard_key keyboards[];

extern ENGINE_API key_binding g_key_bindings[];

ENGINE_API bool IsGroupNotConflicted(EKeyGroup g1, EKeyGroup g2);
ENGINE_API bool IsContextNotConflicted(EKeyContext c1, EKeyContext c2);

ENGINE_API pcstr IdToActionName(EGameActions id);
ENGINE_API EGameActions ActionNameToId(pcstr name);
ENGINE_API game_action* ActionNameToPtr(pcstr name);

ENGINE_API pcstr DikToKeyname(int dik);
ENGINE_API int KeynameToDik(pcstr name);
ENGINE_API keyboard_key* KeynameToPtr(pcstr name);
ENGINE_API keyboard_key* DikToPtr(int dik, bool safe);

ENGINE_API bool IsBinded(EGameActions action_id, int dik, EKeyContext context = EKeyContext::Undefined);
ENGINE_API int GetActionDik(EGameActions action_id, int idx = -1);
ENGINE_API EGameActions GetBindedAction(int dik, EKeyContext context = EKeyContext::Undefined);

ENGINE_API pcstr GetActionBinding(EGameActions action);

template <typename Invocable>
void ForAllActionKeys(EGameActions action_id, Invocable&& invocable)
{
    for (size_t i = 0; i < bindtypes_count; ++i)
    {
        const int key = GetActionDik(action_id, i);
        if (key == SDL_SCANCODE_UNKNOWN)
            continue;

        if constexpr (std::is_invocable_r_v<bool, Invocable, size_t, int>)
        {
            if (invocable(i, key))
                break;
        }
        else
        {
            static_assert(std::is_invocable_v<Invocable, size_t, int>);
            invocable(i, key);
        }
    }
}

extern ENGINE_API void CCC_RegisterInput();
extern ENGINE_API void CCC_DeregisterInput();

struct con_cmd
{
    shared_str cmd;
};

class ENGINE_API ConsoleBindCmds
{
public:
    xr_map<int, con_cmd> m_bindConsoleCmds;

    void bind(int dik, LPCSTR N);
    void unbind(int dik);
    bool execute(int dik);
    void clear();
    void save(IWriter* F);
};

extern ENGINE_API ConsoleBindCmds g_consoleBindCmds;
