#include "stdafx.h"
#include "xr_input_xinput.h"
#include "xr_input.h"
#include "StringTable/IStringTable.h"

ENGINE_API key_binding g_key_bindings[bindings_count];
ENGINE_API EKeyGroup g_current_keygroup = _sp;

// clang-format off
game_action actions[] = {
    { "left",              kLEFT,              _both },
    { "right",             kRIGHT,             _both },
    { "up",                kUP,                _both },
    { "down",              kDOWN,              _both },
    { "jump",              kJUMP,              _both },
    { "crouch",            kCROUCH,            _both },
    { "crouch_toggle",     kCROUCH_TOGGLE,     _both},
    { "accel",             kACCEL,             _both },
    { "sprint_toggle",     kSPRINT_TOGGLE,     _both },

    { "forward",           kFWD,               _both },
    { "back",              kBACK,              _both },
    { "lstrafe",           kL_STRAFE,          _both },
    { "rstrafe",           kR_STRAFE,          _both },

    { "llookout",          kL_LOOKOUT,         _both },
    { "rlookout",          kR_LOOKOUT,         _both },

    { "turn_engine",       kENGINE,            _sp},

    { "cam_1",             kCAM_1,             _both },
    { "cam_2",             kCAM_2,             _both },
    { "cam_3",             kCAM_3,             _both },
    { "cam_4",             kCAM_4,             _both},
    { "cam_zoom_in",       kCAM_ZOOM_IN,       _both },
    { "cam_zoom_out",      kCAM_ZOOM_OUT,      _both },
    { "cam_autoaim",       kCAM_AUTOAIM,       _sp },

    { "torch",             kTORCH,             _both },
    { "night_vision",      kNIGHT_VISION,      _both },
    { "show_detector",     kDETECTOR,          _sp },

    { "wpn_1",             kWPN_1,             _both },
    { "wpn_2",             kWPN_2,             _both },
    { "wpn_3",             kWPN_3,             _both },
    { "wpn_4",             kWPN_4,             _both },
    { "wpn_5",             kWPN_5,             _both },
    { "wpn_6",             kWPN_6,             _both },
    { "artefact",          kARTEFACT,          _both /*_mp*/ },
    { "wpn_next",          kWPN_NEXT,          _both }, // means next ammo type
    { "wpn_fire",          kWPN_FIRE,          _both },
    { "wpn_zoom",          kWPN_ZOOM,          _both },
    { "wpn_zoom_inc",      kWPN_ZOOM_INC,      _both },
    { "wpn_zoom_dec",      kWPN_ZOOM_DEC,      _both },
    { "wpn_reload",        kWPN_RELOAD,        _both },
    { "wpn_func",          kWPN_FUNC,          _both },
    { "wpn_firemode_prev", kWPN_FIREMODE_PREV, _both },
    { "wpn_firemode_next", kWPN_FIREMODE_NEXT, _both },

    { "pause",             kPAUSE,             _both },
    { "drop",              kDROP,              _both },
    { "use",               kUSE,               _both },
    { "scores",            kSCORES,            _both },
    { "chat",              kCHAT,              _both },
    { "chat_team",         kCHAT_TEAM,         _both },
    { "screenshot",        kSCREENSHOT,        _both },
    { "quit",              kQUIT,              _both },
    { "console",           kCONSOLE,           _both },
    { "inventory",         kINVENTORY,         _both },
    { "buy_menu",          kBUY,               _both },
    { "skin_menu",         kSKIN,              _both },
    { "team_menu",         kTEAM,              _both },
    { "active_jobs",       kACTIVE_JOBS,       _sp },
    { "map",                kMAP,               _both },
    { "contacts",           kCONTACTS,          _sp },
    { "ext_1",              kEXT_1,             _both },

    { "vote_begin",        kVOTE_BEGIN,        _both },
    { "show_admin_menu",   kSHOW_ADMIN_MENU,   _both },
    { "vote",              kVOTE,              _both },
    { "vote_yes",          kVOTEYES,           _both },
    { "vote_no",           kVOTENO,            _both },

    { "next_slot",         kNEXT_SLOT,         _both },
    { "prev_slot",         kPREV_SLOT,         _both },

    { "speech_menu_0",     kSPEECH_MENU_0,     _both },
    { "speech_menu_1",     kSPEECH_MENU_1,     _both },

    { "speech_menu_2",     kSPEECH_MENU_2,     _mp },
    { "speech_menu_3",     kSPEECH_MENU_3,     _mp },
    { "speech_menu_4",     kSPEECH_MENU_4,     _mp },
    { "speech_menu_5",     kSPEECH_MENU_5,     _mp },
    { "speech_menu_6",     kSPEECH_MENU_6,     _mp },
    { "speech_menu_7",     kSPEECH_MENU_7,     _mp },
    { "speech_menu_8",     kSPEECH_MENU_8,     _mp },
    { "speech_menu_9",     kSPEECH_MENU_9,     _mp },

    { "use_bandage",       kUSE_BANDAGE,       _sp },
    { "use_medkit",        kUSE_MEDKIT,        _sp },

    { "quick_use_1",       kQUICK_USE_1,       _both },
    { "quick_use_2",       kQUICK_USE_2,       _both },
    { "quick_use_3",       kQUICK_USE_3,       _both },
    { "quick_use_4",       kQUICK_USE_4,       _both },

    { "quick_save",        kQUICK_SAVE,        _sp },
    { "quick_load",        kQUICK_LOAD,        _sp },
    { "alife_command",     kALIFE_CMD,         _sp },

    { "custom1",           kCUSTOM1,           _sp },
    { "custom2",           kCUSTOM2,           _sp },
    { "custom3",           kCUSTOM3,           _sp },
    { "custom4",           kCUSTOM4,           _sp },
    { "custom5",           kCUSTOM5,           _sp },
    { "custom6",           kCUSTOM6,           _sp },
    { "custom7",           kCUSTOM7,           _sp },
    { "custom8",           kCUSTOM8,           _sp },
    { "custom9",           kCUSTOM9,           _sp },
    { "custom10",          kCUSTOM10,          _sp },
    { "custom11",          kCUSTOM11,          _sp },
    { "custom12",          kCUSTOM12,          _sp },
    { "custom13",          kCUSTOM13,          _sp },
    { "custom14",          kCUSTOM14,          _sp },
    { "custom15",          kCUSTOM15,          _sp },

    { "pda_tab1",          kPDA_TAB1,          _sp },
    { "pda_tab2",          kPDA_TAB2,          _sp },
    { "pda_tab3",          kPDA_TAB3,          _sp },
    { "pda_tab4",          kPDA_TAB4,          _sp },
    { "pda_tab5",          kPDA_TAB5,          _sp },
    { "pda_tab6",          kPDA_TAB6,          _sp },

    { "kick",              kKICK,              _sp },

    { nullptr,             kLASTACTION,        _both }
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

    { "kINTERNATIONAL1",        SDL_SCANCODE_INTERNATIONAL1      /* Give a better name? */ },
    { "kINTERNATIONAL2",        SDL_SCANCODE_INTERNATIONAL2      /* Give a better name? */ },
    { "kYEN",                   SDL_SCANCODE_INTERNATIONAL3,     "Yen" },
    { "kINTERNATIONAL4",        SDL_SCANCODE_INTERNATIONAL4      /* Give a better name? */ },
    { "kINTERNATIONAL5",        SDL_SCANCODE_INTERNATIONAL5      /* Give a better name? */ },
    { "kINTERNATIONAL6",        SDL_SCANCODE_INTERNATIONAL6      /* Give a better name? */ },
    { "kINTERNATIONAL7",        SDL_SCANCODE_INTERNATIONAL7      /* Give a better name? */ },
    { "kINTERNATIONAL8",        SDL_SCANCODE_INTERNATIONAL8      /* Give a better name? */ },
    { "kINTERNATIONAL9",        SDL_SCANCODE_INTERNATIONAL9      /* Give a better name? */ },

    { "kHANGUL",                SDL_SCANCODE_LANG1,              "Hangul" },
    { "kHANJA",                 SDL_SCANCODE_LANG2,              "Hanja" },
    { "kKATAKANA",              SDL_SCANCODE_LANG3,              "Katakana" },
    { "kHIRAGANA",              SDL_SCANCODE_LANG4,              "Hiragana" },
    { "kZENHANKAKU",            SDL_SCANCODE_LANG5,              "Zen-Han-kaku" },
    { "kLANG6",                 SDL_SCANCODE_LANG6               /* Give a better name? */ },
    { "kLANG7",                 SDL_SCANCODE_LANG7               /* Give a better name? */ },
    { "kLANG8",                 SDL_SCANCODE_LANG8               /* Give a better name? */ },
    { "kLANG9",                 SDL_SCANCODE_LANG9               /* Give a better name? */ },

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

    { "mouse1",                 MOUSE_1,                         "LMB" },
    { "mouse3",                 MOUSE_3,                         "MMB" },       // This is not a mistake because init algorithm was changed.
    { "mouse2",                 MOUSE_2,                         "RMB" },
    { "mouse4",                 MOUSE_4,                         "Mouse X1" },
    { "mouse5",                 MOUSE_5,                         "Mouse X2" },

    { "kGAMEPAD_A",             XR_CONTROLLER_BUTTON_A,             "Gamepad A" },
    { "kGAMEPAD_B",             XR_CONTROLLER_BUTTON_B,             "Gamepad B" },
    { "kGAMEPAD_X",             XR_CONTROLLER_BUTTON_X,             "Gamepad X" },
    { "kGAMEPAD_Y",             XR_CONTROLLER_BUTTON_Y,             "Gamepad Y" },
    { "kGAMEPAD_BACK",          XR_CONTROLLER_BUTTON_BACK,          "Gamepad Back" },
    { "kGAMEPAD_GUIDE",         XR_CONTROLLER_BUTTON_GUIDE,         "Gamepad Guide" },
    { "kGAMEPAD_START",         XR_CONTROLLER_BUTTON_START,         "Gamepad Start" },
    { "kGAMEPAD_LEFTSTICK",     XR_CONTROLLER_BUTTON_LEFTSTICK,     "Gamepad Left Stick" },
    { "kGAMEPAD_RIGHTSTICK",    XR_CONTROLLER_BUTTON_RIGHTSTICK,    "Gamepad Right Stick" },
    { "kGAMEPAD_LEFTSHOULDER",  XR_CONTROLLER_BUTTON_LEFTSHOULDER,  "Gamepad Left Shoulder" },
    { "kGAMEPAD_RIGHTSHOULDER", XR_CONTROLLER_BUTTON_RIGHTSHOULDER, "Gamepad Right Shoulder" },
    { "kGAMEPAD_DPAD_UP",       XR_CONTROLLER_BUTTON_DPAD_UP,       "Gamepad Up" },
    { "kGAMEPAD_DPAD_DOWN",     XR_CONTROLLER_BUTTON_DPAD_DOWN,     "Gamepad Down" },
    { "kGAMEPAD_DPAD_LEFT",     XR_CONTROLLER_BUTTON_DPAD_LEFT,     "Gamepad Left" },
    { "kGAMEPAD_DPAD_RIGHT",    XR_CONTROLLER_BUTTON_DPAD_RIGHT,    "Gamepad Right" },

    { nullptr,                  -1,                              "(null)" }
};
// clang-format on

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

bool IsBinded(EGameActions action_id, int dik)
{
    key_binding* binding = &g_key_bindings[action_id];
    for (u8 i = 0; i < bindtypes_count; ++i)
        if (binding->m_keyboard[i] && binding->m_keyboard[i]->dik == dik)
            return true;

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

bool GetActionAllBinding(pcstr action, char* dst_buff, int dst_buff_sz)
{
    const int action_id = ActionNameToId(action);
    if (action_id == kNOTBINDED)
    {
        // Just insert the unknown action name as is
        xr_strcpy(dst_buff, dst_buff_sz, action);
        return false;
    }
    key_binding* binding = &g_key_bindings[action_id];

    string128 prim;
    string128 sec;
    string128 gpad;
    prim[0] = 0;
    sec[0] = 0;
    gpad[0] = 0;

    if (binding->m_keyboard[0])
    {
        xr_strcpy(prim, binding->m_keyboard[0]->key_local_name.c_str());
    }
    if (binding->m_keyboard[1])
    {
        xr_strcpy(sec, binding->m_keyboard[1]->key_local_name.c_str());
    }
    if (binding->m_keyboard[2])
    {
        xr_strcpy(gpad, binding->m_keyboard[2]->key_local_name.c_str());
    }
    if (!binding->m_keyboard[0] && !binding->m_keyboard[1] && !binding->m_keyboard[2])
    {
        xr_sprintf(dst_buff, dst_buff_sz, "%s", gStringTable->translate("st_key_notbinded").c_str());
    }
    else
    {
        xr_sprintf(dst_buff, dst_buff_sz, "%s%s%s%s%s", prim[0] ? prim : "",
            (sec[0] && prim[0]) ? " , " : "", sec[0] ? sec : "",
            ((gpad[0] && prim[0]) || (gpad[0] && sec[0])) ? " , " : "", gpad[0] ? gpad : "");
    }
    return true;
}

#pragma todo("Artur to All: Gamepads has own bindings. Add m_keyboard[2]")

std::pair<int, int> GetKeysBindedTo(EGameActions action_id)
{
    const key_binding& binding = g_key_bindings[action_id];
    return
    {
        binding.m_keyboard[0] ? binding.m_keyboard[0]->dik : -1,
        binding.m_keyboard[1] ? binding.m_keyboard[1]->dik : -1
    };
}
