#include "stdafx.h"

#include "xr_level_controller.h"
#include "xr_input.h"

#include "StringTable/StringTable.h"

#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"

constexpr size_t bindings_count = kLASTACTION;

ENGINE_API key_binding g_key_bindings[bindings_count];
ENGINE_API EKeyGroup g_current_keygroup = _sp;

struct action_binding_desc
{
    string128 keyboard_mouse;
    string128 controller;
};

static action_binding_desc g_action_bindings[bindings_count];

// clang-format off
game_action actions[] =
{
    { "look_around",            kLOOK_AROUND,               _both }, // gamepad
    { "left",                   kLEFT,                      _both },
    { "right",                  kRIGHT,                     _both },
    { "up",                     kUP,                        _both },
    { "down",                   kDOWN,                      _both },

    { "move_around",            kMOVE_AROUND,               _both }, // gamepad
    { "forward",                kFWD,                       _both },
    { "back",                   kBACK,                      _both },
    { "lstrafe",                kL_STRAFE,                  _both },
    { "rstrafe",                kR_STRAFE,                  _both },

    { "llookout",               kL_LOOKOUT,                 _both },
    { "rlookout",               kR_LOOKOUT,                 _both },

    { "jump",                   kJUMP,                      _both },
    { "crouch",                 kCROUCH,                    _both },
    { "crouch_toggle",          kCROUCH_TOGGLE,             _both },
    { "accel",                  kACCEL,                     _both },
    { "sprint_toggle",          kSPRINT_TOGGLE,             _both },

    { "turn_engine",            kENGINE,                    _sp},

    { "cam_1",                  kCAM_1,                     _both },
    { "cam_2",                  kCAM_2,                     _both },
    { "cam_3",                  kCAM_3,                     _both },
    { "cam_4",                  kCAM_4,                     _both},
    { "cam_zoom_in",            kCAM_ZOOM_IN,               _both },
    { "cam_zoom_out",           kCAM_ZOOM_OUT,              _both },
    { "cam_autoaim",            kCAM_AUTOAIM,               _sp },

    { "torch",                  kTORCH,                     _both },
    { "night_vision",           kNIGHT_VISION,              _both },
    { "show_detector",          kDETECTOR,                  _sp },

    { "wpn_1",                  kWPN_1,                     _both },
    { "wpn_2",                  kWPN_2,                     _both },
    { "wpn_3",                  kWPN_3,                     _both },
    { "wpn_4",                  kWPN_4,                     _both },
    { "wpn_5",                  kWPN_5,                     _both },
    { "wpn_6",                  kWPN_6,                     _both },
    { "artefact",               kARTEFACT,                  _both /*_mp*/ },
    { "wpn_next",               kWPN_NEXT,                  _both }, // means next ammo type
    { "wpn_fire",               kWPN_FIRE,                  _both },
    { "wpn_zoom",               kWPN_ZOOM,                  _both },
    { "wpn_zoom_inc",           kWPN_ZOOM_INC,              _both },
    { "wpn_zoom_dec",           kWPN_ZOOM_DEC,              _both },
    { "wpn_reload",             kWPN_RELOAD,                _both },
    { "wpn_func",               kWPN_FUNC,                  _both },
    { "wpn_firemode_prev",      kWPN_FIREMODE_PREV,         _both },
    { "wpn_firemode_next",      kWPN_FIREMODE_NEXT,         _both },

    { "pause",                  kPAUSE,                     _both },
    { "drop",                   kDROP,                      _both },
    { "use",                    kUSE,                       _both },
    { "scores",                 kSCORES,                    _both },
    { "chat",                   kCHAT,                      _mp },
    { "chat_team",              kCHAT_TEAM,                 _mp },
    { "screenshot",             kSCREENSHOT,                _both },
    { "enter",                  kENTER,                     _both },
    { "quit",                   kQUIT,                      _both },
    { "console",                kCONSOLE,                   _both },
    { "inventory",              kINVENTORY,                 _both },
    { "buy_menu",               kBUY,                       _mp },
    { "skin_menu",              kSKIN,                      _mp },
    { "team_menu",              kTEAM,                      _mp },
    { "active_jobs",            kACTIVE_JOBS,               _sp },
    { "map",                    kMAP,                       _both },
    { "contacts",               kCONTACTS,                  _sp },
    { "ext_1",                  kEXT_1,                     _both },

    { "vote_begin",             kVOTE_BEGIN,                _mp },
    { "show_admin_menu",        kSHOW_ADMIN_MENU,           _mp },
    { "vote",                   kVOTE,                      _mp },
    { "vote_yes",               kVOTEYES,                   _mp },
    { "vote_no",                kVOTENO,                    _mp },

    { "next_slot",              kNEXT_SLOT,                 _both },
    { "prev_slot",              kPREV_SLOT,                 _both },

    { "speech_menu_0",          kSPEECH_MENU_0,             _mp },
    { "speech_menu_1",          kSPEECH_MENU_1,             _mp },
    { "speech_menu_2",          kSPEECH_MENU_2,             _mp },
    { "speech_menu_3",          kSPEECH_MENU_3,             _mp },
    { "speech_menu_4",          kSPEECH_MENU_4,             _mp },
    { "speech_menu_5",          kSPEECH_MENU_5,             _mp },
    { "speech_menu_6",          kSPEECH_MENU_6,             _mp },
    { "speech_menu_7",          kSPEECH_MENU_7,             _mp },
    { "speech_menu_8",          kSPEECH_MENU_8,             _mp },
    { "speech_menu_9",          kSPEECH_MENU_9,             _mp },

    { "use_bandage",            kUSE_BANDAGE,               _sp },
    { "use_medkit",             kUSE_MEDKIT,                _sp },

    { "quick_use_1",            kQUICK_USE_1,               _both },
    { "quick_use_2",            kQUICK_USE_2,               _both },
    { "quick_use_3",            kQUICK_USE_3,               _both },
    { "quick_use_4",            kQUICK_USE_4,               _both },

    { "quick_save",             kQUICK_SAVE,                _sp },
    { "quick_load",             kQUICK_LOAD,                _sp },
    { "alife_command",          kALIFE_CMD,                 _sp },

    { "numpad0",                kNUMPAD0,                   _sp },
    { "insert",                 kINSERT,                    _sp },
    { "numpad1",                kNUMPAD1,                   _sp },
    { "numpad2",                kNUMPAD2,                   _sp },
    { "numpad3",                kNUMPAD3,                   _sp },
    { "numpad4",                kNUMPAD4,                   _sp },
    { "numpad5",                kNUMPAD5,                   _sp },
    { "numpad6",                kNUMPAD6,                   _sp },
    { "numpad7",                kNUMPAD7,                   _sp },
    { "numpad8",                kNUMPAD8,                   _sp },
    { "numpad9",                kNUMPAD9,                   _sp },
    { "numpad_enter",           kNUMPADENTER,              _sp },

    { "custom1",                kCUSTOM1,                   _sp },
    { "custom2",                kCUSTOM2,                   _sp },
    { "custom3",                kCUSTOM3,                   _sp },
    { "custom4",                kCUSTOM4,                   _sp },
    { "custom5",                kCUSTOM5,                   _sp },
    { "custom6",                kCUSTOM6,                   _sp },
    { "custom7",                kCUSTOM7,                   _sp },
    { "custom8",                kCUSTOM8,                   _sp },
    { "custom9",                kCUSTOM9,                   _sp },
    { "custom10",               kCUSTOM10,                  _sp },
    { "custom11",               kCUSTOM11,                  _sp },
    { "custom12",               kCUSTOM12,                  _sp },
    { "custom13",               kCUSTOM13,                  _sp },
    { "custom14",               kCUSTOM14,                  _sp },
    { "custom15",               kCUSTOM15,                  _sp },

    { "pda_tab1",               kPDA_TAB1,                  _sp },
    { "pda_tab2",               kPDA_TAB2,                  _sp },
    { "pda_tab3",               kPDA_TAB3,                  _sp },
    { "pda_tab4",               kPDA_TAB4,                  _sp },
    { "pda_tab5",               kPDA_TAB5,                  _sp },
    { "pda_tab6",               kPDA_TAB6,                  _sp },

    { "kick",                   kKICK,                      _sp },

    { "editor",                 kEDITOR,                    _both },

    // Contextual actions:
    // UI
    { "ui_move",                kUI_MOVE,                   _both,  EKeyContext::UI },
    { "ui_move_left",           kUI_MOVE_LEFT,              _both,  EKeyContext::UI },
    { "ui_move_right",          kUI_MOVE_RIGHT,             _both,  EKeyContext::UI },
    { "ui_move_up",             kUI_MOVE_UP,                _both,  EKeyContext::UI },
    { "ui_move_down",           kUI_MOVE_DOWN,              _both,  EKeyContext::UI },

    { "ui_move_secondary",      kUI_MOVE_SECONDARY,         _both,  EKeyContext::UI },

    { "ui_accept",              kUI_ACCEPT,                 _both,  EKeyContext::UI },
    { "ui_back",                kUI_BACK,                   _both,  EKeyContext::UI },
    { "ui_action_1",            kUI_ACTION_1,               _both,  EKeyContext::UI },
    { "ui_action_2",            kUI_ACTION_2,               _both,  EKeyContext::UI },

    { "ui_tab_prev",            kUI_TAB_PREV,               _both,  EKeyContext::UI },
    { "ui_tab_next",            kUI_TAB_NEXT,               _both,  EKeyContext::UI },

    { "ui_button_1",            kUI_BUTTON_1,               _both,  EKeyContext::UI },
    { "ui_button_2",            kUI_BUTTON_2,               _both,  EKeyContext::UI },
    { "ui_button_3",            kUI_BUTTON_3,               _both,  EKeyContext::UI },
    { "ui_button_4",            kUI_BUTTON_4,               _both,  EKeyContext::UI },
    { "ui_button_5",            kUI_BUTTON_5,               _both,  EKeyContext::UI },
    { "ui_button_6",            kUI_BUTTON_6,               _both,  EKeyContext::UI },
    { "ui_button_7",            kUI_BUTTON_7,               _both,  EKeyContext::UI },
    { "ui_button_8",            kUI_BUTTON_8,               _both,  EKeyContext::UI },
    { "ui_button_9",            kUI_BUTTON_9,               _both,  EKeyContext::UI },
    { "ui_button_0",            kUI_BUTTON_0,               _both,  EKeyContext::UI },

    // PDA:
    { "pda_map_move",           kPDA_MAP_MOVE,              _sp,    EKeyContext::PDA },
    { "pda_map_move_left",      kPDA_MAP_MOVE_LEFT,         _sp,    EKeyContext::PDA },
    { "pda_map_move_right",     kPDA_MAP_MOVE_RIGHT,        _sp,    EKeyContext::PDA },
    { "pda_map_move_up",        kPDA_MAP_MOVE_UP,           _sp,    EKeyContext::PDA },
    { "pda_map_move_down",      kPDA_MAP_MOVE_DOWN,         _sp,    EKeyContext::PDA },

    { "pda_map_zoom_in",        kPDA_MAP_ZOOM_IN,           _sp,    EKeyContext::PDA },
    { "pda_map_zoom_out",       kPDA_MAP_ZOOM_OUT,          _sp,    EKeyContext::PDA },
    { "pda_map_zoom_reset",     kPDA_MAP_ZOOM_RESET,        _sp,    EKeyContext::PDA },

    { "pda_map_show_actor",     kPDA_MAP_SHOW_ACTOR,        _sp,    EKeyContext::PDA },
    { "pda_map_show_legend",    kPDA_MAP_SHOW_LEGEND,       _sp,    EKeyContext::PDA },

    { "pda_filter_toggle",      kPDA_FILTER_TOGGLE,         _sp,    EKeyContext::PDA },
    { "pda_tasks_toggle",       kPDA_TASKS_TOGGLE,          _sp,    EKeyContext::PDA },

    // Talk:
    { "talk_switch_to_trade",   kTALK_SWITCH_TO_TRADE,      _sp,    EKeyContext::Talk },
    { "talk_log_scroll",        kTALK_LOG_SCROLL,           _sp,    EKeyContext::Talk },
    { "talk_log_scroll_up",     kTALK_LOG_SCROLL_UP,        _sp,    EKeyContext::Talk },
    { "talk_log_scroll_down",   kTALK_LOG_SCROLL_DOWN,      _sp,    EKeyContext::Talk },

    { nullptr,                  kLASTACTION,                _both }
};

keyboard_key keyboards[] =
{
    { "kA",                     SDL_SCANCODE_A,                  "A" },
    { "kB",                     SDL_SCANCODE_B,                  "B" },
    { "kC",                     SDL_SCANCODE_C,                  "C" },
    { "kD",                     SDL_SCANCODE_D,                  "D" },
    { "kE",                     SDL_SCANCODE_E,                  "E" },
    { "kF",                     SDL_SCANCODE_F,                  "F" },
    { "kG",                     SDL_SCANCODE_G,                  "G" },
    { "kH",                     SDL_SCANCODE_H,                  "H" },
    { "kI",                     SDL_SCANCODE_I,                  "I" },
    { "kJ",                     SDL_SCANCODE_J,                  "J" },
    { "kK",                     SDL_SCANCODE_K,                  "K" },
    { "kL",                     SDL_SCANCODE_L,                  "L" },
    { "kM",                     SDL_SCANCODE_M,                  "M" },
    { "kN",                     SDL_SCANCODE_N,                  "N" },
    { "kO",                     SDL_SCANCODE_O,                  "O" },
    { "kP",                     SDL_SCANCODE_P,                  "P" },
    { "kQ",                     SDL_SCANCODE_Q,                  "Q" },
    { "kR",                     SDL_SCANCODE_R,                  "R" },
    { "kS",                     SDL_SCANCODE_S,                  "S" },
    { "kT",                     SDL_SCANCODE_T,                  "T" },
    { "kU",                     SDL_SCANCODE_U,                  "U" },
    { "kV",                     SDL_SCANCODE_V,                  "V" },
    { "kW",                     SDL_SCANCODE_W,                  "W" },
    { "kX",                     SDL_SCANCODE_X,                  "X" },
    { "kY",                     SDL_SCANCODE_Y,                  "Y" },
    { "kZ",                     SDL_SCANCODE_Z,                  "Z" },

    { "k1",                     SDL_SCANCODE_1,                  "1" },
    { "k2",                     SDL_SCANCODE_2,                  "2" },
    { "k3",                     SDL_SCANCODE_3,                  "3" },
    { "k4",                     SDL_SCANCODE_4,                  "4" },
    { "k5",                     SDL_SCANCODE_5,                  "5" },
    { "k6",                     SDL_SCANCODE_6,                  "6" },
    { "k7",                     SDL_SCANCODE_7,                  "7" },
    { "k8",                     SDL_SCANCODE_8,                  "8" },
    { "k9",                     SDL_SCANCODE_9,                  "9" },
    { "k0",                     SDL_SCANCODE_0,                  "0" },

    { "kRETURN",                SDL_SCANCODE_RETURN,             "Return" },
    { "kESCAPE",                SDL_SCANCODE_ESCAPE,             "Escape" },
    { "kBACK",                  SDL_SCANCODE_BACKSPACE,          "Backspace" },
    { "kTAB",                   SDL_SCANCODE_TAB,                "Tab" },
    { "kSPACE",                 SDL_SCANCODE_SPACE,              "Space" },

    { "kMINUS",                 SDL_SCANCODE_MINUS,              "Minus" },
    { "kEQUALS",                SDL_SCANCODE_EQUALS,             "Equals" },
    { "kLBRACKET",              SDL_SCANCODE_LEFTBRACKET,        "Left bracket" },
    { "kRBRACKET",              SDL_SCANCODE_RIGHTBRACKET,       "Right bracket" },
    { "kBACKSLASH",             SDL_SCANCODE_BACKSLASH,          "Backslash" },
    { "kNONUSHASH",             SDL_SCANCODE_NONUSHASH,          "Non US Hash" },

    { "kSEMICOLON",             SDL_SCANCODE_SEMICOLON,          "Semicolon" },
    { "kAPOSTROPHE",            SDL_SCANCODE_APOSTROPHE,         "Apostrophe" },
    { "kGRAVE",                 SDL_SCANCODE_GRAVE,              "Grave" },
    { "kCOMMA",                 SDL_SCANCODE_COMMA,              "Comma" },
    { "kPERIOD",                SDL_SCANCODE_PERIOD,             "Period" },
    { "kSLASH",                 SDL_SCANCODE_SLASH,              "Slash" },

    { "kCAPITAL",               SDL_SCANCODE_CAPSLOCK,           "Caps Lock" },

    { "kF1",                    SDL_SCANCODE_F1,                 "F1" },
    { "kF2",                    SDL_SCANCODE_F2,                 "F2" },
    { "kF3",                    SDL_SCANCODE_F3,                 "F3" },
    { "kF4",                    SDL_SCANCODE_F4,                 "F4" },
    { "kF5",                    SDL_SCANCODE_F5,                 "F5" },
    { "kF6",                    SDL_SCANCODE_F6,                 "F6" },
    { "kF7",                    SDL_SCANCODE_F7,                 "F7" },
    { "kF8",                    SDL_SCANCODE_F8,                 "F8" },
    { "kF9",                    SDL_SCANCODE_F9,                 "F9" },
    { "kF10",                   SDL_SCANCODE_F10,                "F10" },
    { "kF11",                   SDL_SCANCODE_F11,                "F11" },
    { "kF12",                   SDL_SCANCODE_F12,                "F12" },

    { "kPRINTSCREEN",           SDL_SCANCODE_PRINTSCREEN,        "Print Screen" },
    { "kSCROLL",                SDL_SCANCODE_SCROLLLOCK,         "Scroll Lock" },
    { "kPAUSE",                 SDL_SCANCODE_PAUSE,              "Pause" },
    { "kINSERT",                SDL_SCANCODE_INSERT,             "Insert" },

    { "kHOME",                  SDL_SCANCODE_HOME,               "Home" },
    { "kPGUP",                  SDL_SCANCODE_PAGEUP,             "Page Up" },
    { "kDELETE",                SDL_SCANCODE_DELETE,             "Delete" },
    { "kEND",                   SDL_SCANCODE_END,                "End" },
    { "kPGDN",                  SDL_SCANCODE_PAGEDOWN,           "Page Down" },

    { "kRIGHT",                 SDL_SCANCODE_RIGHT,              "Right" },
    { "kLEFT",                  SDL_SCANCODE_LEFT,               "Left" },
    { "kDOWN",                  SDL_SCANCODE_DOWN,               "Down" },
    { "kUP",                    SDL_SCANCODE_UP,                 "Up" },

    { "kNUMLOCK",               SDL_SCANCODE_NUMLOCKCLEAR,       "Num Lock" },

    { "kDIVIDE",                SDL_SCANCODE_KP_DIVIDE,          "Numpad Divide" },
    { "kMULTIPLY",              SDL_SCANCODE_KP_MULTIPLY,        "Numpad Multiply" },
    { "kSUBTRACT",              SDL_SCANCODE_KP_MINUS,           "Numpad Minus" },
    { "kADD",                   SDL_SCANCODE_KP_PLUS,            "Numpad Plus" },
    { "kNUMPADENTER",           SDL_SCANCODE_KP_ENTER,           "Numpad Enter" },

    { "kNUMPAD1",               SDL_SCANCODE_KP_1,               "Numpad 1" },
    { "kNUMPAD2",               SDL_SCANCODE_KP_2,               "Numpad 2" },
    { "kNUMPAD3",               SDL_SCANCODE_KP_3,               "Numpad 3" },
    { "kNUMPAD4",               SDL_SCANCODE_KP_4,               "Numpad 4" },
    { "kNUMPAD5",               SDL_SCANCODE_KP_5,               "Numpad 5" },
    { "kNUMPAD6",               SDL_SCANCODE_KP_6,               "Numpad 6" },
    { "kNUMPAD7",               SDL_SCANCODE_KP_7,               "Numpad 7" },
    { "kNUMPAD8",               SDL_SCANCODE_KP_8,               "Numpad 8" },
    { "kNUMPAD9",               SDL_SCANCODE_KP_9,               "Numpad 9" },
    { "kNUMPAD0",               SDL_SCANCODE_KP_0,               "Numpad 0" },

    { "kNUMPADPERIOD",          SDL_SCANCODE_KP_PERIOD,          "Numpad Period" },
    { "kNONUSBACKSLASH",        SDL_SCANCODE_NONUSBACKSLASH,     "Non US Backslash" },
    { "kAPPLICATION",           SDL_SCANCODE_APPLICATION,        "Application" },
    { "kPOWER",                 SDL_SCANCODE_POWER,              "Power" },
    { "kNUMPADEQUALS",          SDL_SCANCODE_KP_EQUALS,          "Numpad Equals" },

    { "kF13",                   SDL_SCANCODE_F13,                "F13" },
    { "kF14",                   SDL_SCANCODE_F14,                "F14" },
    { "kF15",                   SDL_SCANCODE_F15,                "F15" },
    { "kF16",                   SDL_SCANCODE_F16,                "F16" },
    { "kF17",                   SDL_SCANCODE_F17,                "F17" },
    { "kF18",                   SDL_SCANCODE_F18,                "F18" },
    { "kF19",                   SDL_SCANCODE_F19,                "F19" },
    { "kF20",                   SDL_SCANCODE_F20,                "F20" },
    { "kF21",                   SDL_SCANCODE_F21,                "F21" },
    { "kF22",                   SDL_SCANCODE_F22,                "F22" },
    { "kF23",                   SDL_SCANCODE_F23,                "F23" },
    { "kF24",                   SDL_SCANCODE_F24,                "F24" },

    { "kEXECUTE",               SDL_SCANCODE_EXECUTE,            "Execute" },
    { "kHELP",                  SDL_SCANCODE_HELP,               "Help" },
    { "kMENU",                  SDL_SCANCODE_MENU,               "Menu" },

    { "kSELECT",                SDL_SCANCODE_SELECT,             "Select" },
    { "kSTOP",                  SDL_SCANCODE_STOP,               "Stop" },

    { "kREDO",                  SDL_SCANCODE_AGAIN,              "Redo" },
    { "kUNDO",                  SDL_SCANCODE_UNDO,               "Undo" },

    { "kCUT",                   SDL_SCANCODE_CUT,                "Cut" },
    { "kCOPY",                  SDL_SCANCODE_COPY,               "Copy" },
    { "kPASTE",                 SDL_SCANCODE_PASTE,              "Paste" },

    { "kFIND",                  SDL_SCANCODE_FIND,               "Find" },

    { "kMUTE",                  SDL_SCANCODE_MUTE,               "Mute" },
    { "kVOLUMEUP",              SDL_SCANCODE_VOLUMEUP,           "Volume Up" },
    { "kVOLUMEDOWN",            SDL_SCANCODE_VOLUMEDOWN,         "Volume Down" },

    { "kNUMPADCOMMA",           SDL_SCANCODE_KP_COMMA,           "Numpad Comma" },
    { "kNUMPADEQUALSAS400",     SDL_SCANCODE_KP_EQUALSAS400,     "Equals AS400" },

    { "kINTERNATIONAL1",        SDL_SCANCODE_INTERNATIONAL1,     "kINTERNATIONAL1" /* Give a better name? */ },
    { "kINTERNATIONAL2",        SDL_SCANCODE_INTERNATIONAL2,     "kINTERNATIONAL2" /* Give a better name? */ },
    { "kYEN",                   SDL_SCANCODE_INTERNATIONAL3,     "Yen" },
    { "kINTERNATIONAL4",        SDL_SCANCODE_INTERNATIONAL4,     "kINTERNATIONAL4" /* Give a better name? */ },
    { "kINTERNATIONAL5",        SDL_SCANCODE_INTERNATIONAL5,     "kINTERNATIONAL5" /* Give a better name? */ },
    { "kINTERNATIONAL6",        SDL_SCANCODE_INTERNATIONAL6,     "kINTERNATIONAL6" /* Give a better name? */ },
    { "kINTERNATIONAL7",        SDL_SCANCODE_INTERNATIONAL7,     "kINTERNATIONAL7" /* Give a better name? */ },
    { "kINTERNATIONAL8",        SDL_SCANCODE_INTERNATIONAL8,     "kINTERNATIONAL8" /* Give a better name? */ },
    { "kINTERNATIONAL9",        SDL_SCANCODE_INTERNATIONAL9,     "kINTERNATIONAL9" /* Give a better name? */ },

    { "kHANGUL",                SDL_SCANCODE_LANG1,              "Hangul" },
    { "kHANJA",                 SDL_SCANCODE_LANG2,              "Hanja" },
    { "kKATAKANA",              SDL_SCANCODE_LANG3,              "Katakana" },
    { "kHIRAGANA",              SDL_SCANCODE_LANG4,              "Hiragana" },
    { "kZENHANKAKU",            SDL_SCANCODE_LANG5,              "Zen-Han-kaku" },
    { "kLANG6",                 SDL_SCANCODE_LANG6,              "kLANG6" /* Give a better name? */ },
    { "kLANG7",                 SDL_SCANCODE_LANG7,              "kLANG7" /* Give a better name? */ },
    { "kLANG8",                 SDL_SCANCODE_LANG8,              "kLANG8" /* Give a better name? */ },
    { "kLANG9",                 SDL_SCANCODE_LANG9,              "kLANG9" /* Give a better name? */ },

    { "kALTERASE",              SDL_SCANCODE_ALTERASE,           "Alterase" },
    { "kCANCEL",                SDL_SCANCODE_CANCEL,             "Cancel" },
    { "kCLEAR",                 SDL_SCANCODE_CLEAR,              "Clear" },
    { "kPRIOR",                 SDL_SCANCODE_PRIOR,              "Prior" },
    { "kRETURN2",               SDL_SCANCODE_RETURN2,            "Return 2" },
    { "kSEPARATOR",             SDL_SCANCODE_SEPARATOR,          "Separator" },
    { "kOUT",                   SDL_SCANCODE_OUT,                "Out" },
    { "kOPER",                  SDL_SCANCODE_OPER,               "Oper" },
    { "kCLEARAGAIN",            SDL_SCANCODE_CLEARAGAIN,         "Clear Again" },
    { "kCRSEL",                 SDL_SCANCODE_CRSEL,              "Crsel" },
    { "kEXSEL",                 SDL_SCANCODE_EXSEL,              "Excel" },

    { "kNUMPAD_00",             SDL_SCANCODE_KP_00,              "Numpad 00" },
    { "kNUMPAD_000",            SDL_SCANCODE_KP_000,             "Numpad 000" },
    { "kTHOUSANDSSEPARATOR",    SDL_SCANCODE_THOUSANDSSEPARATOR, "Thousand Separator" },
    { "kDECIMALSEPARATOR",      SDL_SCANCODE_DECIMALSEPARATOR,   "Decimal Separator" },
    { "kCURRENCYUNIT",          SDL_SCANCODE_CURRENCYUNIT,       "Currency Unit" },
    { "kCURRENCYSUBUNIT",       SDL_SCANCODE_CURRENCYSUBUNIT,    "Currency Subunit" },

    { "kNUMPAD_LEFTPAREN",      SDL_SCANCODE_KP_LEFTPAREN,       "Numpad Left Paren" },
    { "kNUMPAD_RIGHTPAREN",     SDL_SCANCODE_KP_RIGHTPAREN,      "Numpad Right Paren" },
    { "kNUMPAD_LEFTBRACE",      SDL_SCANCODE_KP_LEFTBRACE,       "Numpad Left Brace" },
    { "kNUMPAD_RIGHTBRACE",     SDL_SCANCODE_KP_RIGHTBRACE,      "Numpad Right Brace" },
    { "kNUMPAD_TAB",            SDL_SCANCODE_KP_TAB,             "Numpad Tab" },
    { "kNUMPAD_BACKSPACE",      SDL_SCANCODE_KP_BACKSPACE,       "Numpad Backspace" },

    { "kNUMPAD_A",              SDL_SCANCODE_KP_A,               "Numpad A" },
    { "kNUMPAD_B",              SDL_SCANCODE_KP_B,               "Numpad B" },
    { "kNUMPAD_C",              SDL_SCANCODE_KP_C,               "Numpad C" },
    { "kNUMPAD_D",              SDL_SCANCODE_KP_D,               "Numpad D" },
    { "kNUMPAD_E",              SDL_SCANCODE_KP_E,               "Numpad E" },
    { "kNUMPAD_F",              SDL_SCANCODE_KP_F,               "Numpad F" },

    { "kNUMPAD_XOR",            SDL_SCANCODE_KP_XOR,             "Numpad XOR" },

    { "kNUMPAD_POWER",          SDL_SCANCODE_KP_POWER,           "Numpad Power" },
    { "kNUMPAD_PERCENT",        SDL_SCANCODE_KP_PERCENT,         "Numpad Percent" },

    { "kNUMPAD_LESS",           SDL_SCANCODE_KP_LESS,            "Numpad Less" },
    { "kNUMPAD_GREATER",        SDL_SCANCODE_KP_GREATER,         "Numpad Greater" },

    { "kNUMPAD_AMPERSAND",      SDL_SCANCODE_KP_AMPERSAND,       "Numpad Ampersand" },
    { "kNUMPAD_DBLAMPERSAND",   SDL_SCANCODE_KP_DBLAMPERSAND,    "Numpad Double Ampersand" },

    { "kNUMPAD_VERTICALBAR",    SDL_SCANCODE_KP_VERTICALBAR,     "Numpad Vertical Bar" },
    { "kNUMPAD_DBLVERTICALBAR", SDL_SCANCODE_KP_DBLVERTICALBAR,  "Numpad Double Vertical Bar" },

    { "kNUMPAD_COLON",          SDL_SCANCODE_KP_COLON,           "Numpad Colon" },
    { "kNUMPAD_HASH",           SDL_SCANCODE_KP_HASH,            "Numpad Hash" },
    { "kNUMPAD_SPACE",          SDL_SCANCODE_KP_SPACE,           "Numpad Space" },
    { "kNUMPAD_AT",             SDL_SCANCODE_KP_AT,              "Numpad At" },
    { "kNUMPAD_EXCLAM",         SDL_SCANCODE_KP_EXCLAM,          "Numpad Exclam" },

    { "kNUMPAD_MEMSTORE",       SDL_SCANCODE_KP_MEMSTORE,        "Numpad Mem Store" },
    { "kNUMPAD_MEMRECALL",      SDL_SCANCODE_KP_MEMRECALL,       "Numpad Mem Recall" },
    { "kNUMPAD_MEMCLEAR",       SDL_SCANCODE_KP_MEMCLEAR,        "Numpad Mem Clear" },
    { "kNUMPAD_MEMADD",         SDL_SCANCODE_KP_MEMADD,          "Numpad Mem Add" },
    { "kNUMPAD_MEMSUBTRACT",    SDL_SCANCODE_KP_MEMSUBTRACT,     "Numpad Mem Subtract" },
    { "kNUMPAD_MEMMULTIPLY",    SDL_SCANCODE_KP_MEMMULTIPLY,     "Numpad Mem Multiply" },
    { "kNUMPAD_MEMDIVIDE",      SDL_SCANCODE_KP_MEMDIVIDE,       "Numpad Mem Divide" },

    { "kNUMPAD_PLUSMINUS",      SDL_SCANCODE_KP_PLUSMINUS,       "Numpad Plus-Minus" },
    { "kNUMPAD_CLEAR",          SDL_SCANCODE_KP_CLEAR,           "Numpad Clear" },
    { "kNUMPAD_CLEARENTRY",     SDL_SCANCODE_KP_CLEARENTRY,      "Numpad Clear Entry" },
    { "kNUMPAD_BINARY",         SDL_SCANCODE_KP_BINARY,          "Numpad Binary" },
    { "kNUMPAD_OCTAL",          SDL_SCANCODE_KP_OCTAL,           "Numpad Octal" },
    { "kNUMPAD_DECIMAL",        SDL_SCANCODE_KP_DECIMAL,         "Numpad Decimal" },
    { "kNUMPAD_HEXADECIMAL",    SDL_SCANCODE_KP_HEXADECIMAL,     "Numpad Hexadecimal" },

    { "kLCONTROL",              SDL_SCANCODE_LCTRL,              "Left Ctrl" },
    { "kLSHIFT",                SDL_SCANCODE_LSHIFT,             "Left shift" },
    { "kLMENU",                 SDL_SCANCODE_LALT,               "Left Alt" },
    { "kLWIN",                  SDL_SCANCODE_LGUI,               "Left Windows" },
    { "kRCONTROL",              SDL_SCANCODE_RCTRL,              "Right Ctrl" },
    { "kRSHIFT",                SDL_SCANCODE_RSHIFT,             "Right Shift" },
    { "kRMENU",                 SDL_SCANCODE_RALT,               "Right Alt" },
    { "kRWIN",                  SDL_SCANCODE_RGUI,               "Right Windows" },

    { "kMODE",                  SDL_SCANCODE_MODE,               "Mode" },

    { "kAUDIONEXT",             SDL_SCANCODE_AUDIONEXT,          "Audio Next" },
    { "kAUDIOPREV",             SDL_SCANCODE_AUDIOPREV,          "Audio Prev" },
    { "kAUDIOSTOP",             SDL_SCANCODE_AUDIOSTOP,          "Audio Stop" },
    { "kAUDIOPLAY",             SDL_SCANCODE_AUDIOPLAY,          "Audio Play" },
    { "kAUDIOMUTE",             SDL_SCANCODE_AUDIOMUTE,          "Audio Mute" },

    { "kMEDIASELECT",           SDL_SCANCODE_MEDIASELECT,        "Media Select" },
    { "kWWW",                   SDL_SCANCODE_WWW,                "WWW" },
    { "kMAIL",                  SDL_SCANCODE_MAIL,               "Mail" },
    { "kCALCULATOR",            SDL_SCANCODE_CALCULATOR,         "Calculator" },
    { "kCOMPUTER",              SDL_SCANCODE_COMPUTER,           "My Computer" },

    { "kNUMPAD_AC_SEARCH",      SDL_SCANCODE_AC_SEARCH,          "AC Search" },
    { "kNUMPAD_AC_HOME",        SDL_SCANCODE_AC_HOME,            "AC Home" },
    { "kNUMPAD_AC_BACK",        SDL_SCANCODE_AC_BACK,            "AC Back" },
    { "kNUMPAD_AC_FORWARD",     SDL_SCANCODE_AC_FORWARD,         "AC Forward" },
    { "kNUMPAD_AC_STOP",        SDL_SCANCODE_AC_STOP,            "AC Stop" },
    { "kNUMPAD_AC_REFRESH",     SDL_SCANCODE_AC_REFRESH,         "AC Refresh" },
    { "kNUMPAD_AC_BOOKMARKS",   SDL_SCANCODE_AC_BOOKMARKS,       "AC Bookmarks" },

    { "kBRIGHTNESSDOWN",        SDL_SCANCODE_BRIGHTNESSDOWN,     "Brightness Down" },
    { "kBRIGHTNESSUP",          SDL_SCANCODE_BRIGHTNESSUP,       "Brightness Up" },
    { "kDISPLAYSWITCH",         SDL_SCANCODE_DISPLAYSWITCH,      "Display Switch" },

    { "kKBDILLUMTOGGLE",        SDL_SCANCODE_KBDILLUMTOGGLE,     "Illum Toogle" },
    { "kKBDILLUMDOWN",          SDL_SCANCODE_KBDILLUMDOWN,       "Illum Down" },
    { "kKBDILLUMUP",            SDL_SCANCODE_KBDILLUMUP,         "Illum Up" },

    { "kEJECT",                 SDL_SCANCODE_EJECT,              "Eject" },
    { "kSLEEP",                 SDL_SCANCODE_SLEEP,              "Sleep" },

    { "kAPP1",                  SDL_SCANCODE_APP1,               "App 1" },
    { "kAPP2",                  SDL_SCANCODE_APP2,               "App 2" },

    { "mouse1",                 MOUSE_1,                         "Left mouse button" },
    { "mouse2",                 MOUSE_2,                         "Right mouse button" },
    { "mouse3",                 MOUSE_3,                         "Mouse wheel button" },
    { "mouse4",                 MOUSE_4,                         "Mouse X1" },
    { "mouse5",                 MOUSE_5,                         "Mouse X2" },

    { "gpA",                    XR_CONTROLLER_BUTTON_A,             "A" },
    { "gpB",                    XR_CONTROLLER_BUTTON_B,             "B" },
    { "gpX",                    XR_CONTROLLER_BUTTON_X,             "X" },
    { "gpY",                    XR_CONTROLLER_BUTTON_Y,             "Y" },
    { "gpBACK",                 XR_CONTROLLER_BUTTON_BACK,          "Back" },
    { "gpGUIDE",                XR_CONTROLLER_BUTTON_GUIDE,         "Guide" },
    { "gpSTART",                XR_CONTROLLER_BUTTON_START,         "Start" },
    { "gpLEFT_STICK",           XR_CONTROLLER_BUTTON_LEFTSTICK,     "Left Stick Press" },
    { "gpRIGHT_STICK",          XR_CONTROLLER_BUTTON_RIGHTSTICK,    "Right Stick Press" },
    { "gpLEFT_SHOULDER",        XR_CONTROLLER_BUTTON_LEFTSHOULDER,  "Left Shoulder" },
    { "gpRIGHT_SHOULDER",       XR_CONTROLLER_BUTTON_RIGHTSHOULDER, "Right Shoulder" },
    { "gpDPAD_UP",              XR_CONTROLLER_BUTTON_DPAD_UP,       "D-Pad Up" },
    { "gpDPAD_DOWN",            XR_CONTROLLER_BUTTON_DPAD_DOWN,     "D-Pad Down" },
    { "gpDPAD_LEFT",            XR_CONTROLLER_BUTTON_DPAD_LEFT,     "D-Pad Left" },
    { "gpDPAD_RIGHT",           XR_CONTROLLER_BUTTON_DPAD_RIGHT,    "D-Pad Right" },
    { "gpMISC1",                XR_CONTROLLER_BUTTON_MISC1,         "Misc1" },
    { "gpPADDLE_P1",            XR_CONTROLLER_BUTTON_PADDLE1,       "Paddle 1" },
    { "gpPADDLE_P2",            XR_CONTROLLER_BUTTON_PADDLE2,       "Paddle 2" },
    { "gpPADDLE_P3",            XR_CONTROLLER_BUTTON_PADDLE3,       "Paddle 3" },
    { "gpPADDLE_P4",            XR_CONTROLLER_BUTTON_PADDLE4,       "Paddle 4" },
    { "gpTOUCHPAD",             XR_CONTROLLER_BUTTON_TOUCHPAD,      "Touchpad" },

    { "gpAXIS_LEFT",            XR_CONTROLLER_AXIS_LEFT,            "Left Stick Move" },
    { "gpAXIS_RIGHT",           XR_CONTROLLER_AXIS_RIGHT,           "Right Stick Move" },
    { "gpAXIS_TRIGGER_LEFT",    XR_CONTROLLER_AXIS_TRIGGER_LEFT,    "Left Trigger" },
    { "gpAXIS_TRIGGER_RIGHT",   XR_CONTROLLER_AXIS_TRIGGER_RIGHT,   "Right Trigger" },

    { nullptr,                  -1,                                 "(null)" }
};
// clang-format on

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
                Msg("%s == %s", k1.key_name, k2.key_name);
            }
            ++i2;
        }
        ++i1;
    }
#endif

    for (size_t idx = 0; idx < bindings_count; ++idx)
    {
        R_ASSERT3(actions[idx].id == (EGameActions)idx,
            make_string("actions array and EGameActions IDs mismatch: %s has index %zu, but correct index is %u",
                 actions[idx].action_name, idx, actions[idx].id).c_str(),
            "Did you added action to EGameActions enum, but didn't added it to 'actions' array in the correct place?"
        );
        g_key_bindings[idx].m_action = &actions[idx];
    }
}

static void TranslateBinding(key_binding& keyBinding, action_binding_desc& actionBinding)
{
    // Keyboard & mouse
    if (keyBinding.m_keyboard[0] || keyBinding.m_keyboard[1])
    {
        const auto& prim = keyBinding.m_keyboard[0];
        const auto& sec  = keyBinding.m_keyboard[1];

        strconcat(actionBinding.keyboard_mouse,
                  prim        ? prim->key_local_name.c_str() : "",
                  sec && prim ? ", "                         : "",
                  sec         ? sec->key_local_name.c_str()  : "");
    }
    else
    {
        xr_strcpy(actionBinding.keyboard_mouse, StringTable().translate("st_key_notbinded").c_str());
    }

    // Gamepad
    if (keyBinding.m_keyboard[2])
    {
        xr_strcpy(actionBinding.controller, keyBinding.m_keyboard[2]->key_local_name.c_str());
    }
    else
    {
        xr_strcpy(actionBinding.controller, StringTable().translate("st_key_notbinded").c_str());
    }
}

static void RemapKeys()
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
#ifndef MASTER_GOLD
            Msg("! Can't find a key name for %s", kb.key_name);
#endif
            if (kb.key_local_name.empty())
                kb.key_local_name = kb.key_name;
        }

        // Msg("[%s]-[%s]", kb.key_name, kb.key_local_name.c_str());
    }

    for (size_t i = 0; i < bindings_count; ++i)
    {
        TranslateBinding(g_key_bindings[i], g_action_bindings[i]);
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

EGameActions ActionNameToId(pcstr name)
{
    game_action* action = ActionNameToPtr(name);
    if (action)
        return action->id;
    else
        return kNOTBINDED;
}

game_action* ActionNameToPtr(pcstr name)
{
    size_t idx = 0;
    while (actions[idx].action_name)
    {
        if (!xr_stricmp(name, actions[idx].action_name))
            return &actions[idx];
        ++idx;
    }
    Msg("! [ActionNameToPtr] cant find corresponding 'id' for '%s'", name);
    return nullptr;
}

bool IsBinded(EGameActions action_id, int dik, EKeyContext context /*= EKeyContext::Undefined*/)
{
    key_binding* binding = &g_key_bindings[action_id];
    for (u8 i = 0; i < bindtypes_count; ++i)
    {
        if (binding->m_keyboard[i] && binding->m_keyboard[i]->dik == dik)
            if (binding->m_action->key_context == context)
                return true;
    }
    return false;
}

int GetActionDik(EGameActions action_id, int idx)
{
    key_binding* binding = &g_key_bindings[action_id];

    if (idx == -1)
    {
        for (u8 i = 0; i < bindtypes_count; ++i)
            if (binding->m_keyboard[i])
                return binding->m_keyboard[i]->dik;
    }
    else
    {
        if (binding->m_keyboard[idx])
            return binding->m_keyboard[idx]->dik;
    }
    return SDL_SCANCODE_UNKNOWN;
}

pcstr DikToKeyname(int dik)
{
    keyboard_key* kb = DikToPtr(dik, true);
    if (kb)
        return kb->key_name;
    else
        return nullptr;
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

    return nullptr;
}

int KeynameToDik(pcstr name)
{
    keyboard_key* kb = KeynameToPtr(name);
    return kb->dik;
}

keyboard_key* KeynameToPtr(pcstr name)
{
    size_t idx = 0;
    while (keyboards[idx].key_name)
    {
        keyboard_key& kb = keyboards[idx];
        if (!xr_stricmp(name, kb.key_name))
            return &keyboards[idx];
        ++idx;
    }

    Msg("! [KeynameToPtr] cant find corresponding 'keyboard_key' for keyname %s", name);
    return NULL;
}

bool IsGroupNotConflicted(EKeyGroup g1, EKeyGroup g2)
{
    return ((g1 == _sp && g2 == _mp) || (g1 == _mp && g2 == _sp));
}

bool IsGroupMatching(EKeyGroup g1, EKeyGroup g2)
{
    return ((g1 == g2) || (g1 == _both) || (g2 == _both));
}

bool IsContextNotConflicted(EKeyContext c1, EKeyContext c2)
{
    return c1 != c2;
}

bool IsContextMatching(EKeyContext c1, EKeyContext c2)
{
    return c1 == c2 || (c1 == EKeyContext::Undefined && c2 == EKeyContext::Undefined);
}

EGameActions GetBindedAction(int dik, EKeyContext context /*= EKeyContext::Undefined*/)
{
    for (int idx = 0; idx < static_cast<int>(bindings_count); ++idx)
    {
        key_binding* binding = &g_key_bindings[idx];

        const bool isGroupMatching = IsGroupMatching(binding->m_action->key_group, g_current_keygroup);
        if (!isGroupMatching)
            continue;

        const bool isContextMatching = IsContextMatching(binding->m_action->key_context, context);
        if (!isContextMatching)
            continue;

        for (u8 i = 0; i < bindtypes_count && isGroupMatching; ++i)
            if (binding->m_keyboard[i] && binding->m_keyboard[i]->dik == dik)
                return binding->m_action->id;
    }
    return kNOTBINDED;
}

ENGINE_API pcstr GetActionBinding(EGameActions action)
{
    auto& binding = g_action_bindings[action];

    switch (pInput->GetCurrentInputType())
    {
    default:
        NODEFAULT;
        [[fallthrough]];
    case CInput::KeyboardMouse:
        return binding.keyboard_mouse;
    case CInput::Controller:
        return binding.controller;
    }
}

static class KeyMapWatcher final : public pureKeyMapChanged
{
public:
    void Initialize()
    {
        pInput->RegisterKeyMapChangeWatcher(this, REG_PRIORITY_HIGH);
    }

    void Destroy()
    {
        if (pInput) // XXX: this check should not exist
            pInput->RemoveKeyMapChangeWatcher(this);
    }

    void OnKeyMapChanged() override
    {
        RemapKeys();
    }
} s_keymap_watcher;

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
            RemapKeys();
            s_keymap_watcher.Initialize();
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

        for (size_t idx = 0; idx < bindings_count; ++idx)
        {
            key_binding* binding = &g_key_bindings[idx];
            if (binding == currBinding)
                continue;

            const bool groupConflict = !IsGroupNotConflicted(binding->m_action->key_group, currBinding->m_action->key_group);
            const bool contextConflict = !IsContextNotConflicted(binding->m_action->key_context, currBinding->m_action->key_context);

            for (u8 i = 0; i < bindtypes_count; ++i)
                if (binding->m_keyboard[i] == keyboard && (groupConflict && contextConflict))
                    binding->m_keyboard[i] = nullptr;
        }

        TranslateBinding(g_key_bindings[actionId], g_action_bindings[actionId]);
    }

    virtual void Save(IWriter* f)
    {
        if (m_workIdx == 0)
            f->w_printf("default_controls\r\n");

        for (int idx = 0; idx < static_cast<int>(bindings_count); ++idx)
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
        TranslateBinding(g_key_bindings[actionId], g_action_bindings[actionId]);
    }
};

class CCC_ListActions : public IConsole_Command
{
public:
    CCC_ListActions(LPCSTR n) : IConsole_Command(n) { bEmptyArgsHandled = true; };

    virtual void Execute(LPCSTR args)
    {
        Log("- --- Action list start ---");
        for (int idx = 0; idx < static_cast<int>(bindings_count); ++idx)
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
        for (int idx = 0; idx < static_cast<int>(bindings_count); ++idx)
        {
            key_binding* binding = &g_key_bindings[idx];
            for (u8 i = 0; i < bindtypes_count; ++i)
                binding->m_keyboard[i] = nullptr;

            TranslateBinding(g_key_bindings[idx], g_action_bindings[idx]);
        }
        g_consoleBindCmds.clear();
    }
};

class CCC_DefControls : public CCC_UnBindAll
{
    struct binding
    {
        EGameActions action;
        int keys[bindtypes_count];
    };

    // clang-format off
    constexpr static binding predefined_bindings[] =
    {
        { kLOOK_AROUND,             { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_AXIS_RIGHT } },
        { kMOVE_AROUND,             { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_AXIS_LEFT } },

        { kWPN_FIRE,                { MOUSE_1,              SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_AXIS_TRIGGER_RIGHT } },
        { kWPN_ZOOM,                { MOUSE_2,              SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_AXIS_TRIGGER_LEFT } },

        { kINVENTORY,               { SDL_SCANCODE_I,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_RIGHTSHOULDER } },
        { kACTIVE_JOBS,             { SDL_SCANCODE_P,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_LEFTSHOULDER } },
        { kMAP,                     { SDL_SCANCODE_M,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kCONTACTS,                { SDL_SCANCODE_H,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },

        { kJUMP,                    { SDL_SCANCODE_SPACE,   SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_A } },
        { kCROUCH_TOGGLE,           { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_B } },
        { kWPN_RELOAD,              { SDL_SCANCODE_R,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_X } },
        { kUSE,                     { SDL_SCANCODE_F,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_Y } },

        { kTORCH,                   { SDL_SCANCODE_L,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_RIGHTSTICK } },
        { kSCORES,                  { SDL_SCANCODE_TAB,     SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_LEFTSTICK } },

        { kENTER,                   { SDL_SCANCODE_RETURN,  SDL_SCANCODE_KP_ENTER,      XR_CONTROLLER_BUTTON_START } },
        { kQUIT,                    { SDL_SCANCODE_ESCAPE,  SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_BACK } },

        { kQUICK_USE_1,             { SDL_SCANCODE_F1,      SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_DPAD_UP } },
        { kQUICK_USE_2,             { SDL_SCANCODE_F2,      SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_DPAD_LEFT } },
        { kQUICK_USE_3,             { SDL_SCANCODE_F3,      SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_DPAD_RIGHT } },
        { kQUICK_USE_4,             { SDL_SCANCODE_F4,      SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_DPAD_DOWN } },

        // Contextual actions:
        // UI
        { kUI_MOVE,                 { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_AXIS_RIGHT } },
        { kUI_MOVE_LEFT,            { SDL_SCANCODE_A,       SDL_SCANCODE_LEFT,          XR_CONTROLLER_BUTTON_DPAD_LEFT } },
        { kUI_MOVE_RIGHT,           { SDL_SCANCODE_D,       SDL_SCANCODE_RIGHT,         XR_CONTROLLER_BUTTON_DPAD_RIGHT } },
        { kUI_MOVE_UP,              { SDL_SCANCODE_W,       SDL_SCANCODE_UP,            XR_CONTROLLER_BUTTON_DPAD_UP } },
        { kUI_MOVE_DOWN,            { SDL_SCANCODE_S,       SDL_SCANCODE_DOWN,          XR_CONTROLLER_BUTTON_DPAD_DOWN } },

        { kUI_MOVE_SECONDARY,       { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_AXIS_LEFT } },

        { kUI_ACCEPT,               { SDL_SCANCODE_RETURN,  SDL_SCANCODE_F,             XR_CONTROLLER_BUTTON_A } },
        { kUI_BACK,                 { SDL_SCANCODE_ESCAPE,  SDL_SCANCODE_G,             XR_CONTROLLER_BUTTON_B } },
        { kUI_ACTION_1,             { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_Y,             XR_CONTROLLER_BUTTON_X } },
        { kUI_ACTION_2,             { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_N,             XR_CONTROLLER_BUTTON_Y } },

        { kUI_TAB_PREV,             { SDL_SCANCODE_Q,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_LEFTSHOULDER } },
        { kUI_TAB_NEXT,             { SDL_SCANCODE_E,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_RIGHTSHOULDER } },

        { kUI_BUTTON_1,             { SDL_SCANCODE_1,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_2,             { SDL_SCANCODE_2,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_3,             { SDL_SCANCODE_3,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_4,             { SDL_SCANCODE_4,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_5,             { SDL_SCANCODE_5,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_6,             { SDL_SCANCODE_6,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_7,             { SDL_SCANCODE_7,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_8,             { SDL_SCANCODE_8,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_9,             { SDL_SCANCODE_9,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
        { kUI_BUTTON_0,             { SDL_SCANCODE_0,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },

        // PDA:
        { kPDA_MAP_MOVE,            { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_AXIS_RIGHT } },
        { kPDA_MAP_MOVE_LEFT,       { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_LEFT,          XR_CONTROLLER_BUTTON_INVALID } },
        { kPDA_MAP_MOVE_RIGHT,      { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_RIGHT,         XR_CONTROLLER_BUTTON_INVALID } },
        { kPDA_MAP_MOVE_UP,         { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UP,            XR_CONTROLLER_BUTTON_INVALID } },
        { kPDA_MAP_MOVE_DOWN,       { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_DOWN,          XR_CONTROLLER_BUTTON_INVALID } },

        { kPDA_MAP_ZOOM_IN,         { SDL_SCANCODE_Z,       SDL_SCANCODE_KP_PLUS,       XR_CONTROLLER_AXIS_TRIGGER_RIGHT } },
        { kPDA_MAP_ZOOM_OUT,        { SDL_SCANCODE_C,       SDL_SCANCODE_KP_MINUS,      XR_CONTROLLER_AXIS_TRIGGER_LEFT } },
        { kPDA_MAP_ZOOM_RESET,      { SDL_SCANCODE_X,       SDL_SCANCODE_KP_0,          XR_CONTROLLER_BUTTON_INVALID } },

        { kPDA_MAP_SHOW_ACTOR,      { SDL_SCANCODE_R,       SDL_SCANCODE_KP_COMMA,      XR_CONTROLLER_BUTTON_RIGHTSTICK } },
        { kPDA_MAP_SHOW_LEGEND,     { SDL_SCANCODE_V,       SDL_SCANCODE_KP_MULTIPLY,   XR_CONTROLLER_BUTTON_INVALID } },

        { kPDA_FILTER_TOGGLE,       { SDL_SCANCODE_B,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_Y } },
        { kPDA_TASKS_TOGGLE,        { SDL_SCANCODE_TAB,     SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_X } },

        // Talk:
        { kTALK_SWITCH_TO_TRADE,    { SDL_SCANCODE_X,       SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_X } },
        { kTALK_LOG_SCROLL,         { SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_AXIS_RIGHT } },
        { kTALK_LOG_SCROLL_UP,      { SDL_SCANCODE_E,       SDL_SCANCODE_PAGEUP,        XR_CONTROLLER_AXIS_TRIGGER_LEFT } },
        { kTALK_LOG_SCROLL_DOWN,    { SDL_SCANCODE_Q,       SDL_SCANCODE_PAGEDOWN,      XR_CONTROLLER_AXIS_TRIGGER_RIGHT } },

        { kEDITOR,                  { SDL_SCANCODE_F10,     SDL_SCANCODE_UNKNOWN,       XR_CONTROLLER_BUTTON_INVALID } },
    };
    // clang-format on

public:
    CCC_DefControls(LPCSTR n) : CCC_UnBindAll(n) {}

    virtual void Execute(LPCSTR args)
    {
        CCC_UnBindAll::Execute(args);

        for (const auto& [action, keys] : predefined_bindings)
        {
            key_binding& binding = g_key_bindings[action];

            for (u8 i = 0; i < bindtypes_count; ++i)
            {
                if (!binding.m_keyboard[i])
                    binding.m_keyboard[i] = DikToPtr(keys[i], true);
            }

            TranslateBinding(binding, g_action_bindings[action]);
        }

        string_path cfg, cmd;
        FS.update_path(cfg, "$game_config$", "default_controls.ltx");
        strconcat(cmd, "cfg_load", " ", cfg);
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

        for (int idx = 0; idx < static_cast<int>(bindings_count); ++idx)
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

void CCC_DeregisterInput()
{
    if (g_remapped)
        s_keymap_watcher.Destroy();
}
