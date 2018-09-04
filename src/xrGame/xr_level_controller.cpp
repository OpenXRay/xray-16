#include "stdafx.h"
#include "xrEngine/xr_ioconsole.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "xr_level_controller.h"
#include "string_table.h"

_binding g_key_bindings[bindings_count];
_key_group g_current_keygroup = _sp;

_action actions[] =
{
    { "left",              kLEFT,              _both },
    { "right",             kRIGHT,             _both },
    { "up",                kUP,                _both },
    { "down",              kDOWN,              _both },
    { "jump",              kJUMP,              _both },
    { "crouch",            kCROUCH,            _both },
    { "accel",             kACCEL,             _both },
    { "sprint_toggle",     kSPRINT_TOGGLE,     _both },

    { "forward",           kFWD,               _both },
    { "back",              kBACK,              _both },
    { "lstrafe",           kL_STRAFE,          _both },
    { "rstrafe",           kR_STRAFE,          _both },

    { "llookout",          kL_LOOKOUT,         _both },
    { "rlookout",          kR_LOOKOUT,         _both },

    { "cam_1",             kCAM_1,             _both },
    { "cam_2",             kCAM_2,             _both },
    { "cam_3",             kCAM_3,             _both },
    { "cam_zoom_in",       kCAM_ZOOM_IN,       _both },
    { "cam_zoom_out",      kCAM_ZOOM_OUT,      _both },

    { "torch",             kTORCH,             _both },
    { "night_vision",      kNIGHT_VISION,      _both },
    { "show_detector",     kDETECTOR,          _sp   },

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
    { "chat",              kCHAT,              _mp   },
    { "chat_team",         kCHAT_TEAM,         _mp   },
    { "screenshot",        kSCREENSHOT,        _both },
    { "quit",              kQUIT,              _both },
    { "console",           kCONSOLE,           _both },
    { "inventory",         kINVENTORY,         _both },
    { "buy_menu",          kBUY,               _mp   },
    { "skin_menu",         kSKIN,              _mp   },
    { "team_menu",         kTEAM,              _mp   },
    { "active_jobs",       kACTIVE_JOBS,       _sp   },

    { "vote_begin",        kVOTE_BEGIN,        _mp   },
    { "show_admin_menu",   kSHOW_ADMIN_MENU,   _mp   },
    { "vote",              kVOTE,              _mp   },
    { "vote_yes",          kVOTEYES,           _mp   },
    { "vote_no",           kVOTENO,            _mp   },

    { "next_slot",         kNEXT_SLOT,         _both },
    { "prev_slot",         kPREV_SLOT,         _both },

    { "speech_menu_0",     kSPEECH_MENU_0,     _mp   },
    { "speech_menu_1",     kSPEECH_MENU_1,     _mp   },

    { "quick_use_1",       kQUICK_USE_1,       _both },
    { "quick_use_2",       kQUICK_USE_2,       _both },
    { "quick_use_3",       kQUICK_USE_3,       _both },
    { "quick_use_4",       kQUICK_USE_4,       _both },

    { "quick_save",        kQUICK_SAVE,        _sp   },
    { "quick_load",        kQUICK_LOAD,        _sp   },
    //{ "alife_command",   kALIFE_CMD,         _sp   },

    { nullptr,             kLASTACTION,        _both }
};

_keyboard keyboards[] =
{
    { "mouse1",                 MOUSE_1,                         "Mouse 1"                    },
    { "mouse2",                 MOUSE_2,                         "Mouse 2"                    },
    { "mouse3",                 MOUSE_3,                         "Mouse 3"                    },
    { "mouse4",                 MOUSE_4,                         "Mouse 4"                    },
    { "mouse5",                 MOUSE_5,                         "Mouse 5"                    },
    { "mouse6",                 MOUSE_6,                         "Mouse 6"                    },
    { "mouse7",                 MOUSE_7,                         "Mouse 7"                    },
    { "mouse8",                 MOUSE_8,                         "Mouse 8"                    },

    { "kUNKNOWN",               SDL_SCANCODE_UNKNOWN,            "Unknown"                    },

    { "kA",                     SDL_SCANCODE_A,                  "A"                          },
    { "kB",                     SDL_SCANCODE_B,                  "B"                          },
    { "kC",                     SDL_SCANCODE_C,                  "C"                          },
    { "kD",                     SDL_SCANCODE_D,                  "D"                          },
    { "kE",                     SDL_SCANCODE_E,                  "E"                          },
    { "kF",                     SDL_SCANCODE_F,                  "F"                          },
    { "kG",                     SDL_SCANCODE_G,                  "G"                          },
    { "kH",                     SDL_SCANCODE_H,                  "H"                          },
    { "kI",                     SDL_SCANCODE_I,                  "I"                          },
    { "kJ",                     SDL_SCANCODE_J,                  "J"                          },
    { "kK",                     SDL_SCANCODE_K,                  "K"                          },
    { "kL",                     SDL_SCANCODE_L,                  "L"                          },
    { "kM",                     SDL_SCANCODE_M,                  "M"                          },
    { "kN",                     SDL_SCANCODE_N,                  "N"                          },
    { "kO",                     SDL_SCANCODE_O,                  "O"                          },
    { "kP",                     SDL_SCANCODE_P,                  "P"                          },
    { "kQ",                     SDL_SCANCODE_Q,                  "Q"                          },
    { "kR",                     SDL_SCANCODE_R,                  "R"                          },
    { "kS",                     SDL_SCANCODE_S,                  "S"                          },
    { "kT",                     SDL_SCANCODE_T,                  "T"                          },
    { "kU",                     SDL_SCANCODE_U,                  "U"                          },
    { "kV",                     SDL_SCANCODE_V,                  "V"                          },
    { "kW",                     SDL_SCANCODE_W,                  "W"                          },
    { "kX",                     SDL_SCANCODE_X,                  "X"                          },
    { "kY",                     SDL_SCANCODE_Y,                  "Y"                          },
    { "kZ",                     SDL_SCANCODE_Z,                  "Z"                          },

    { "k1",                     SDL_SCANCODE_1,                  "1"                          },
    { "k2",                     SDL_SCANCODE_2,                  "2"                          },
    { "k3",                     SDL_SCANCODE_3,                  "3"                          },
    { "k4",                     SDL_SCANCODE_4,                  "4"                          },
    { "k5",                     SDL_SCANCODE_5,                  "5"                          },
    { "k6",                     SDL_SCANCODE_6,                  "6"                          },
    { "k7",                     SDL_SCANCODE_7,                  "7"                          },
    { "k8",                     SDL_SCANCODE_8,                  "8"                          },
    { "k9",                     SDL_SCANCODE_9,                  "9"                          },
    { "k0",                     SDL_SCANCODE_0,                  "0"                          },

    { "kRETURN",                SDL_SCANCODE_RETURN,             "Return"                     },
    { "kESCAPE",                SDL_SCANCODE_ESCAPE,             "Escape"                     },
    { "kBACK",                  SDL_SCANCODE_BACKSPACE,          "Backspace"                  },
    { "kTAB",                   SDL_SCANCODE_TAB,                "Tab"                        },
    { "kSPACE",                 SDL_SCANCODE_SPACE,              "Space"                      },

    { "kMINUS",                 SDL_SCANCODE_MINUS,              "Minus"                      },
    { "kEQUALS",                SDL_SCANCODE_EQUALS,             "Equals"                     },
    { "kLBRACKET",              SDL_SCANCODE_LEFTBRACKET,        "Left bracket"               },
    { "kRBRACKET",              SDL_SCANCODE_RIGHTBRACKET,       "Right bracket"              },
    { "kBACKSLASH",             SDL_SCANCODE_BACKSLASH,          "Backslash"                  },
    { "kNONUSHASH",             SDL_SCANCODE_NONUSHASH,          "Non US Hash"                },

    { "kSEMICOLON",             SDL_SCANCODE_SEMICOLON,          "Semicolon"                  },
    { "kAPOSTROPHE",            SDL_SCANCODE_APOSTROPHE,         "Apostrophe"                 },
    { "kGRAVE",                 SDL_SCANCODE_GRAVE,              "Grave"                      },
    { "kCOMMA",                 SDL_SCANCODE_COMMA,              "Comma"                      },
    { "kPERIOD",                SDL_SCANCODE_PERIOD,             "Period"                     },
    { "kSLASH",                 SDL_SCANCODE_SLASH,              "Slash"                      },

    { "kCAPITAL",               SDL_SCANCODE_CAPSLOCK,           "Caps Lock"                  },

    { "kF1",                    SDL_SCANCODE_F1,                 "F1"                         },
    { "kF2",                    SDL_SCANCODE_F2,                 "F2"                         },
    { "kF3",                    SDL_SCANCODE_F3,                 "F3"                         },
    { "kF4",                    SDL_SCANCODE_F4,                 "F4"                         },
    { "kF5",                    SDL_SCANCODE_F5,                 "F5"                         },
    { "kF6",                    SDL_SCANCODE_F6,                 "F6"                         },
    { "kF7",                    SDL_SCANCODE_F7,                 "F7"                         },
    { "kF8",                    SDL_SCANCODE_F8,                 "F8"                         },
    { "kF9",                    SDL_SCANCODE_F9,                 "F9"                         },
    { "kF10",                   SDL_SCANCODE_F10,                "F10"                        },
    { "kF11",                   SDL_SCANCODE_F11,                "F11"                        },
    { "kF12",                   SDL_SCANCODE_F12,                "F12"                        },

    { "kPRINTSCREEN",           SDL_SCANCODE_PRINTSCREEN,        "Print Screen"               },
    { "kSCROLL",                SDL_SCANCODE_SCROLLLOCK,         "Scroll Lock"                },
    { "kPAUSE",                 SDL_SCANCODE_PAUSE,              "Pause"                      },
    { "kINSERT",                SDL_SCANCODE_INSERT,             "Insert"                     },

    { "kHOME",                  SDL_SCANCODE_HOME,               "Home"                       },
    { "kPGUP",                  SDL_SCANCODE_PAGEUP,             "Page Up"                    },
    { "kDELETE",                SDL_SCANCODE_DELETE,             "Delete"                     },
    { "kEND",                   SDL_SCANCODE_END,                "End"                        },
    { "kPGDN",                  SDL_SCANCODE_PAGEDOWN,           "Page Down"                  },

    { "kRIGHT",                 SDL_SCANCODE_RIGHT,              "Right"                      },
    { "kLEFT",                  SDL_SCANCODE_LEFT,               "Left"                       },
    { "kDOWN",                  SDL_SCANCODE_DOWN,               "Down"                       },
    { "kUP",                    SDL_SCANCODE_UP,                 "Up"                         },

    { "kNUMLOCK",               SDL_SCANCODE_NUMLOCKCLEAR,       "Num Lock"                   },

    { "kDIVIDE",                SDL_SCANCODE_KP_DIVIDE,          "Numpad Divide"              },
    { "kMULTIPLY",              SDL_SCANCODE_KP_MULTIPLY,        "Numpad Multiply"            },
    { "kSUBTRACT",              SDL_SCANCODE_KP_MINUS,           "Numpad Minus"               },
    { "kADD",                   SDL_SCANCODE_KP_PLUS,            "Numpad Plus"                },
    { "kNUMPADENTER",           SDL_SCANCODE_KP_ENTER,           "Numpad Enter"               },

    { "kNUMPAD1",               SDL_SCANCODE_KP_1,               "Numpad 1"                   },
    { "kNUMPAD2",               SDL_SCANCODE_KP_2,               "Numpad 2"                   },
    { "kNUMPAD3",               SDL_SCANCODE_KP_3,               "Numpad 3"                   },
    { "kNUMPAD4",               SDL_SCANCODE_KP_4,               "Numpad 4"                   },
    { "kNUMPAD5",               SDL_SCANCODE_KP_5,               "Numpad 5"                   },
    { "kNUMPAD6",               SDL_SCANCODE_KP_6,               "Numpad 6"                   },
    { "kNUMPAD7",               SDL_SCANCODE_KP_7,               "Numpad 7"                   },
    { "kNUMPAD8",               SDL_SCANCODE_KP_8,               "Numpad 8"                   },
    { "kNUMPAD9",               SDL_SCANCODE_KP_9,               "Numpad 9"                   },
    { "kNUMPAD0",               SDL_SCANCODE_KP_0,               "Numpad 0"                   },

    { "kNUMPADPERIOD",          SDL_SCANCODE_KP_PERIOD,          "Numpad Period"              },
    { "kNONUSBACKSLASH",        SDL_SCANCODE_NONUSBACKSLASH,     "Non US Backslash"           },
    { "kAPPLICATION",           SDL_SCANCODE_APPLICATION,        "Application"                },
    { "kPOWER",                 SDL_SCANCODE_POWER,              "Power"                      },
    { "kNUMPADEQUALS",          SDL_SCANCODE_KP_EQUALS,          "Numpad Equals"              },

    { "kF13",                   SDL_SCANCODE_F13,                "F13"                        },
    { "kF14",                   SDL_SCANCODE_F14,                "F14"                        },
    { "kF15",                   SDL_SCANCODE_F15,                "F15"                        },
    { "kF16",                   SDL_SCANCODE_F16,                "F16"                        },
    { "kF17",                   SDL_SCANCODE_F17,                "F17"                        },
    { "kF18",                   SDL_SCANCODE_F18,                "F18"                        },
    { "kF19",                   SDL_SCANCODE_F19,                "F19"                        },
    { "kF20",                   SDL_SCANCODE_F20,                "F20"                        },
    { "kF21",                   SDL_SCANCODE_F21,                "F21"                        },
    { "kF22",                   SDL_SCANCODE_F22,                "F22"                        },
    { "kF23",                   SDL_SCANCODE_F23,                "F23"                        },
    { "kF24",                   SDL_SCANCODE_F24,                "F24"                        },

    { "kEXECUTE",               SDL_SCANCODE_EXECUTE,            "Execute"                    },
    { "kHELP",                  SDL_SCANCODE_HELP,               "Help"                       },
    { "kMENU",                  SDL_SCANCODE_MENU,               "Menu"                       },

    { "kSELECT",                SDL_SCANCODE_SELECT,             "Select"                     },
    { "kSTOP",                  SDL_SCANCODE_STOP,               "Stop"                       },

    { "kREDO",                  SDL_SCANCODE_AGAIN,              "Redo"                       },
    { "kUNDO",                  SDL_SCANCODE_UNDO,               "Undo"                       },

    { "kCUT",                   SDL_SCANCODE_CUT,                "Cut"                        },
    { "kCOPY",                  SDL_SCANCODE_COPY,               "Copy"                       },
    { "kPASTE",                 SDL_SCANCODE_PASTE,              "Paste"                      },

    { "kFIND",                  SDL_SCANCODE_FIND,               "Find"                       },

    { "kMUTE",                  SDL_SCANCODE_MUTE,               "Mute"                       },
    { "kVOLUMEUP",              SDL_SCANCODE_VOLUMEUP,           "Volume Up"                  },
    { "kVOLUMEDOWN",            SDL_SCANCODE_VOLUMEDOWN,         "Volume Down"                },

    { "kNUMPADCOMMA",           SDL_SCANCODE_KP_COMMA,           "Numpad Comma"               },
    { "kNUMPADEQUALSAS400",     SDL_SCANCODE_KP_EQUALSAS400,     "Equals AS400"               },

    { "kINTERNATIONAL1",        SDL_SCANCODE_INTERNATIONAL1      /* Give a better name? */    },
    { "kINTERNATIONAL2",        SDL_SCANCODE_INTERNATIONAL2      /* Give a better name? */    },
    { "kYEN",                   SDL_SCANCODE_INTERNATIONAL3,     "Yen"                        },
    { "kINTERNATIONAL4",        SDL_SCANCODE_INTERNATIONAL4      /* Give a better name? */    },
    { "kINTERNATIONAL5",        SDL_SCANCODE_INTERNATIONAL5      /* Give a better name? */    },
    { "kINTERNATIONAL6",        SDL_SCANCODE_INTERNATIONAL6      /* Give a better name? */    },
    { "kINTERNATIONAL7",        SDL_SCANCODE_INTERNATIONAL7      /* Give a better name? */    },
    { "kINTERNATIONAL8",        SDL_SCANCODE_INTERNATIONAL8      /* Give a better name? */    },
    { "kINTERNATIONAL9",        SDL_SCANCODE_INTERNATIONAL9      /* Give a better name? */    },

    { "kHANGUL",                SDL_SCANCODE_LANG1,              "Hangul"                     },
    { "kHANJA",                 SDL_SCANCODE_LANG2,              "Hanja"                      },
    { "kKATAKANA",              SDL_SCANCODE_LANG3,              "Katakana"                   },
    { "kHIRAGANA",              SDL_SCANCODE_LANG4,              "Hiragana"                   },
    { "kZENHANKAKU",            SDL_SCANCODE_LANG5,              "Zen-Han-kaku"               },
    { "kLANG6",                 SDL_SCANCODE_LANG6               /* Give a better name? */    },
    { "kLANG7",                 SDL_SCANCODE_LANG7               /* Give a better name? */    },
    { "kLANG8",                 SDL_SCANCODE_LANG8               /* Give a better name? */    },
    { "kLANG9",                 SDL_SCANCODE_LANG9               /* Give a better name? */    },

    { "kALTERASE",              SDL_SCANCODE_ALTERASE,           "Alterase"                   },
    { "kCANCEL",                SDL_SCANCODE_CANCEL,             "Cancel"                     },
    { "kCLEAR",                 SDL_SCANCODE_CLEAR,              "Clear"                      },
    { "kPRIOR",                 SDL_SCANCODE_PRIOR,              "Prior"                      },
    { "kRETURN2",               SDL_SCANCODE_RETURN2,            "Return 2"                   },
    { "kSEPARATOR",             SDL_SCANCODE_SEPARATOR,          "Separator"                  },
    { "kOUT",                   SDL_SCANCODE_OUT,                "Out"                        },
    { "kOPER",                  SDL_SCANCODE_OPER,               "Oper"                       },
    { "kCLEARAGAIN",            SDL_SCANCODE_CLEARAGAIN,         "Clear Again"                },
    { "kCRSEL",                 SDL_SCANCODE_CRSEL,              "Crsel"                      },
    { "kEXSEL",                 SDL_SCANCODE_EXSEL,              "Excel"                      },

    { "kNUMPAD_00",             SDL_SCANCODE_KP_00,              "Numpad 00"                  },
    { "kNUMPAD_000",            SDL_SCANCODE_KP_000,             "Numpad 000"                 },
    { "kTHOUSANDSSEPARATOR",    SDL_SCANCODE_THOUSANDSSEPARATOR, "Thousand Separator"         },
    { "kDECIMALSEPARATOR",      SDL_SCANCODE_DECIMALSEPARATOR,   "Decimal Separator"          },
    { "kCURRENCYUNIT",          SDL_SCANCODE_CURRENCYUNIT,       "Currency Unit"              },
    { "kCURRENCYSUBUNIT",       SDL_SCANCODE_CURRENCYSUBUNIT,    "Currency Subunit"           },
    { "kNUMPAD_LEFTPAREN",      SDL_SCANCODE_KP_LEFTPAREN,       "Numpad Left Paren"          },
    { "kNUMPAD_RIGHTPAREN",     SDL_SCANCODE_KP_RIGHTPAREN,      "Numpad Right Paren"         },
    { "kNUMPAD_LEFTBRACE",      SDL_SCANCODE_KP_LEFTBRACE,       "Numpad Left Brace"          },
    { "kNUMPAD_RIGHTBRACE",     SDL_SCANCODE_KP_RIGHTBRACE,      "Numpad Right Brace"         },
    { "kNUMPAD_TAB",            SDL_SCANCODE_KP_TAB,             "Numpad Tab"                 },
    { "kNUMPAD_BACKSPACE",      SDL_SCANCODE_KP_BACKSPACE,       "Numpad Backspace"           },
    { "kNUMPAD_A",              SDL_SCANCODE_KP_A,               "Numpad A"                   },
    { "kNUMPAD_B",              SDL_SCANCODE_KP_B,               "Numpad B"                   },
    { "kNUMPAD_C",              SDL_SCANCODE_KP_C,               "Numpad C"                   },
    { "kNUMPAD_D",              SDL_SCANCODE_KP_D,               "Numpad D"                   },
    { "kNUMPAD_E",              SDL_SCANCODE_KP_E,               "Numpad E"                   },
    { "kNUMPAD_F",              SDL_SCANCODE_KP_F,               "Numpad F"                   },

    { "kNUMPAD_XOR",            SDL_SCANCODE_KP_XOR,             "Numpad XOR"                 },

    { "kNUMPAD_POWER",          SDL_SCANCODE_KP_POWER,           "Numpad Power"               },
    { "kNUMPAD_PERCENT",        SDL_SCANCODE_KP_PERCENT,         "Numpad Percent"             },

    { "kNUMPAD_LESS",           SDL_SCANCODE_KP_LESS,            "Numpad Less"                },
    { "kNUMPAD_GREATER",        SDL_SCANCODE_KP_GREATER,         "Numpad Greater"             },

    { "kNUMPAD_AMPERSAND",      SDL_SCANCODE_KP_AMPERSAND,       "Numpad Ampersand"           },
    { "kNUMPAD_DBLAMPERSAND",   SDL_SCANCODE_KP_DBLAMPERSAND,    "Numpad Double Ampersand"    },

    { "kNUMPAD_VERTICALBAR",    SDL_SCANCODE_KP_VERTICALBAR,     "Numpad Vertical Bar"        },
    { "kNUMPAD_DBLVERTICALBAR", SDL_SCANCODE_KP_DBLVERTICALBAR,  "Numpad Double Vertical Bar" },

    { "kNUMPAD_COLON",          SDL_SCANCODE_KP_COLON,           "Numpad Colon"               },
    { "kNUMPAD_HASH",           SDL_SCANCODE_KP_HASH,            "Numpad Hash"                },
    { "kNUMPAD_SPACE",          SDL_SCANCODE_KP_SPACE,           "Numpad Space"               },
    { "kNUMPAD_AT",             SDL_SCANCODE_KP_AT,              "Numpad At"                  },
    { "kNUMPAD_EXCLAM",         SDL_SCANCODE_KP_EXCLAM,          "Numpad Exclam"              },

    { "kNUMPAD_MEMSTORE",       SDL_SCANCODE_KP_MEMSTORE,        "Numpad Mem Store"           },
    { "kNUMPAD_MEMRECALL",      SDL_SCANCODE_KP_MEMRECALL,       "Numpad Mem Recall"          },
    { "kNUMPAD_MEMCLEAR",       SDL_SCANCODE_KP_MEMCLEAR,        "Numpad Mem Clear"           },
    { "kNUMPAD_MEMADD",         SDL_SCANCODE_KP_MEMADD,          "Numpad Mem Add"             },
    { "kNUMPAD_MEMSUBTRACT",    SDL_SCANCODE_KP_MEMSUBTRACT,     "Numpad Mem Subtract"        },
    { "kNUMPAD_MEMMULTIPLY",    SDL_SCANCODE_KP_MEMMULTIPLY,     "Numpad Mem Multiply"        },
    { "kNUMPAD_MEMDIVIDE",      SDL_SCANCODE_KP_MEMDIVIDE,       "Numpad Mem Divide"          },

    { "kNUMPAD_PLUSMINUS",      SDL_SCANCODE_KP_PLUSMINUS,       "Numpad Plus-Minus"          },
    { "kNUMPAD_CLEAR",          SDL_SCANCODE_KP_CLEAR,           "Numpad Clear"               },
    { "kNUMPAD_CLEARENTRY",     SDL_SCANCODE_KP_CLEARENTRY,      "Numpad Clear Entry"         },
    { "kNUMPAD_BINARY",         SDL_SCANCODE_KP_BINARY,          "Numpad Binary"              },
    { "kNUMPAD_OCTAL",          SDL_SCANCODE_KP_OCTAL,           "Numpad Octal"               },
    { "kNUMPAD_DECIMAL",        SDL_SCANCODE_KP_DECIMAL,         "Numpad Decimal"             },
    { "kNUMPAD_HEXADECIMAL",    SDL_SCANCODE_KP_HEXADECIMAL,     "Numpad Hexadecimal"         },

    { "kLCONTROL",              SDL_SCANCODE_LCTRL,              "Left Ctrl"                  },
    { "kLSHIFT",                SDL_SCANCODE_LSHIFT,             "Left shift"                 },
    { "kLMENU",                 SDL_SCANCODE_LALT,               "Left Alt"                   },
    { "kLWIN",                  SDL_SCANCODE_LGUI,               "Left Windows"               },
    { "kRCONTROL",              SDL_SCANCODE_RCTRL,              "Right Ctrl"                 },
    { "kRSHIFT",                SDL_SCANCODE_RSHIFT,             "Right Shift"                },
    { "kRMENU",                 SDL_SCANCODE_RALT,               "Right Alt"                  },
    { "kRWIN",                  SDL_SCANCODE_RGUI,               "Right Windows"              },

    { "kMODE",                  SDL_SCANCODE_MODE,               "Mode"                       },

    { "kAUDIONEXT",             SDL_SCANCODE_AUDIONEXT,          "Audio Next"                 },
    { "kAUDIOPREV",             SDL_SCANCODE_AUDIOPREV,          "Audio Prev"                 },
    { "kAUDIOSTOP",             SDL_SCANCODE_AUDIOSTOP,          "Audio Stop"                 },
    { "kAUDIOPLAY",             SDL_SCANCODE_AUDIOPLAY,          "Audio Play"                 },
    { "kAUDIOMUTE",             SDL_SCANCODE_AUDIOMUTE,          "Audio Mute"                 },

    { "kMEDIASELECT",           SDL_SCANCODE_MEDIASELECT,        "Media Select"               },
    { "kWWW",                   SDL_SCANCODE_WWW,                "WWW"                        },
    { "kMAIL",                  SDL_SCANCODE_MAIL,               "Mail"                       },
    { "kCALCULATOR",            SDL_SCANCODE_CALCULATOR,         "Calculator"                 },
    { "kCOMPUTER",              SDL_SCANCODE_COMPUTER,           "My Computer"                },

    { "kNUMPAD_AC_SEARCH",      SDL_SCANCODE_AC_SEARCH,          "AC Search"                  },
    { "kNUMPAD_AC_HOME",        SDL_SCANCODE_AC_HOME,            "AC Home"                    },
    { "kNUMPAD_AC_BACK",        SDL_SCANCODE_AC_BACK,            "AC Back"                    },
    { "kNUMPAD_AC_FORWARD",     SDL_SCANCODE_AC_FORWARD,         "AC Forward"                 },
    { "kNUMPAD_AC_STOP",        SDL_SCANCODE_AC_STOP,            "AC Stop"                    },
    { "kNUMPAD_AC_REFRESH",     SDL_SCANCODE_AC_REFRESH,         "AC Refresh"                 },
    { "kNUMPAD_AC_BOOKMARKS",   SDL_SCANCODE_AC_BOOKMARKS,       "AC Bookmarks"               },

    { "kBRIGHTNESSDOWN",        SDL_SCANCODE_BRIGHTNESSDOWN,     "Brightness Down"            },
    { "kBRIGHTNESSUP",          SDL_SCANCODE_BRIGHTNESSUP,       "Brightness Up"              },
    { "kDISPLAYSWITCH",         SDL_SCANCODE_DISPLAYSWITCH,      "Display Switch"             },

    { "kKBDILLUMTOGGLE)",       SDL_SCANCODE_KBDILLUMTOGGLE,     "Illum Toogle"               },
    { "kKBDILLUMDOWN",          SDL_SCANCODE_KBDILLUMDOWN,       "Illum Down"                 },
    { "kKBDILLUMUP",            SDL_SCANCODE_KBDILLUMUP,         "Illum Up"                   },

    { "kEJECT",                 SDL_SCANCODE_EJECT,              "Eject"                      },
    { "kSLEEP",                 SDL_SCANCODE_SLEEP,              "Sleep"                      },

    { "kAPP1",                  SDL_SCANCODE_APP1,               "App 1"                      },
    { "kAPP2",                  SDL_SCANCODE_APP2,               "App 2"                      },
    { nullptr,                  -1,                              "(null)"                     }
};

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
    // Log("Keys remap:");
    while (keyboards[idx].key_name)
    {
        buff[0] = 0;
        _keyboard& kb = keyboards[idx];
        const bool res = pInput->get_dik_name(kb.dik, buff, sizeof(buff));
        if (res)
            kb.key_local_name = buff;
        else if (kb.key_local_name.empty())
            kb.key_local_name = kb.key_name;

        // Msg("[%s]-[%s]", kb.key_name, kb.key_local_name.c_str());
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
    return SDL_SCANCODE_UNKNOWN;
}

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
