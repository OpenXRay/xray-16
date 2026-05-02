#pragma once

namespace xray::render::fg
{
// Main blenders for level
constexpr CLASS_ID B_DEFAULT        = make_clsid("LM      ");
constexpr CLASS_ID B_DEFAULT_AREF   = make_clsid("LM_AREF ");
constexpr CLASS_ID B_VERT           = make_clsid("V       ");
constexpr CLASS_ID B_VERT_AREF      = make_clsid("V_AREF  ");
constexpr CLASS_ID B_LmBmmD         = make_clsid("LmBmmD  ");
constexpr CLASS_ID B_LmEbB          = make_clsid("LmEbB   ");
constexpr CLASS_ID B_B              = make_clsid("BmmD    ");
constexpr CLASS_ID B_BmmD           = make_clsid("BmmDold ");

constexpr CLASS_ID B_PARTICLE       = make_clsid("PARTICLE");

constexpr CLASS_ID B_SCREEN_SET     = make_clsid("S_SET   ");

constexpr CLASS_ID B_DETAIL         = make_clsid("D_STILL ");
constexpr CLASS_ID B_TREE           = make_clsid("D_TREE  ");

constexpr CLASS_ID B_MODEL          = make_clsid("MODEL   ");
constexpr CLASS_ID B_MODEL_EbB      = make_clsid("MODELEbB");

// Editor
constexpr CLASS_ID B_EDITOR_WIRE    = make_clsid("E_WIRE  ");
constexpr CLASS_ID B_EDITOR_SEL     = make_clsid("E_SEL   ");
} // namespace xray::render::fg
