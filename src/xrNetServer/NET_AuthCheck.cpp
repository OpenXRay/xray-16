#include "stdafx.h"
#include "NET_AuthCheck.h"

void XRNETSERVER_API fill_auth_check_params(xr_auth_strings_t& ignore, xr_auth_strings_t& check)
{
    string_path config;
    pcstr pth = FS.get_path("$app_data_root$")->m_Path;
    ignore.push_back(shared_str(pth));
    ignore.push_back(shared_str(FS.update_path(config, "$game_config$", "localization.ltx")));
    ignore.push_back(shared_str(FS.update_path(config, "$game_config$", "fonts.ltx")));
    ignore.push_back(shared_str(FS.update_path(config, "$game_config$", "items.ltx")));
    ignore.push_back(shared_str(FS.update_path(config, "$game_config$", "text")));
    ignore.push_back(shared_str(FS.update_path(config, "$game_config$", "gameplay")));
    ignore.push_back(shared_str(FS.update_path(config, "$game_config$", "ui")));
    ignore.push_back(shared_str(FS.update_path(config, "$game_config$", "scripts")));
    ignore.push_back(shared_str(FS.update_path(config, "$game_config$", "misc\\script_sound_pripyat.ltx")));
    ignore.push_back(shared_str(FS.update_path(config, "$game_scripts$", "state_mgr_pri_a15.script")));

    check.push_back(shared_str(FS.update_path(config, "$game_config$", "")));
    check.push_back(shared_str(FS.update_path(config, "$game_scripts$", "")));
    check.push_back(shared_str(FS.update_path(config, "$game_shaders$", "")));
    // sounds
    check.push_back(shared_str(FS.update_path(config, "$game_sounds$", "material")));
    check.push_back(shared_str(FS.update_path(config, "$game_sounds$", "weapons")));

    // check scopes
    check.push_back(shared_str(FS.update_path(config, "$game_textures$", "wpn\\wpn_crosshair.dds")));
    check.push_back(shared_str(FS.update_path(config, "$game_textures$", "wpn\\wpn_crosshair_bino.dds")));
    check.push_back(shared_str(FS.update_path(config, "$game_textures$", "wpn\\wpn_crosshair_g36.dds")));
    check.push_back(shared_str(FS.update_path(config, "$game_textures$", "wpn\\wpn_crosshair_l85.dds")));
    check.push_back(shared_str(FS.update_path(config, "$game_textures$", "wpn\\wpn_crosshair_rpg.dds")));

    check.push_back(shared_str("xrD3D9-Null"));
    check.push_back(shared_str("ODE"));
    check.push_back(shared_str("xrCDB"));
    check.push_back(shared_str("xrCore"));
    //check.push_back(shared_str("xrGame"));
    check.push_back(shared_str("xrGamespy"));
    check.push_back(shared_str("xrNetserver"));
    check.push_back(shared_str("xrParticles"));
    check.push_back(shared_str("xrRender_R1"));
    check.push_back(shared_str("xrRender_R2"));
    check.push_back(shared_str("xrSound"));
    check.push_back(shared_str("xrXMLParser"));
    //check.push_back(shared_str("xrEngine.exe"));
}

bool XRNETSERVER_API allow_to_include_path(xr_auth_strings_t const& ignore, pcstr path)
{
    for (xr_auth_strings_t::const_iterator i = ignore.begin(), ie = ignore.end(); i != ie; ++i)
    {
        if (!strncmp(i->c_str(), path, i->size()))
            return false;
    }
    return true;
}
