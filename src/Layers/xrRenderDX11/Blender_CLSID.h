#pragma once

namespace xray::render::RENDER_NAMESPACE
{
// Main blenders for level
constexpr CLASS_ID B_DEFAULT        = make_clsid("LM      ");
constexpr CLASS_ID B_DEFAULT_AREF   = make_clsid("LM_AREF ");
constexpr CLASS_ID B_VERT           = make_clsid("V       ");
constexpr CLASS_ID B_VERT_AREF      = make_clsid("V_AREF  ");
constexpr CLASS_ID B_LmBmmD         = make_clsid("LmBmmD  ");
constexpr CLASS_ID B_LaEmB          = make_clsid("LaEmB   ");
constexpr CLASS_ID B_LmEbB          = make_clsid("LmEbB   ");
constexpr CLASS_ID B_B              = make_clsid("BmmD    ");
constexpr CLASS_ID B_BmmD           = make_clsid("BmmDold ");

constexpr CLASS_ID B_PARTICLE       = make_clsid("PARTICLE");

// Screen space blenders
constexpr CLASS_ID B_SCREEN_SET     = make_clsid("S_SET   ");
constexpr CLASS_ID B_SCREEN_GRAY    = make_clsid("S_GRAY  ");

constexpr CLASS_ID B_LIGHT          = make_clsid("LIGHT   ");
constexpr CLASS_ID B_BLUR           = make_clsid("BLUR    ");
constexpr CLASS_ID B_SHADOW_TEX     = make_clsid("SH_TEX  ");
constexpr CLASS_ID B_SHADOW_WORLD   = make_clsid("SH_WORLD");

constexpr CLASS_ID B_DETAIL         = make_clsid("D_STILL ");
constexpr CLASS_ID B_TREE           = make_clsid("D_TREE  ");

constexpr CLASS_ID B_MODEL          = make_clsid("MODEL   ");
constexpr CLASS_ID B_MODEL_EbB      = make_clsid("MODELEbB");

// Editor
constexpr CLASS_ID B_EDITOR_WIRE    = make_clsid("E_WIRE  ");
constexpr CLASS_ID B_EDITOR_SEL     = make_clsid("E_SEL   ");
} // namespace xray::render::RENDER_NAMESPACE
