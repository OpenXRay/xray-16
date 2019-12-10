#include "StdAfx.h"
#include "VersionSwitcher.h"
#include "xrEngine/XR_IOConsole.h"

static CVersionSwitcher s_switcher;

extern ENGINE_API string512 g_sLaunchOnExit_app;
extern ENGINE_API string512 g_sLaunchOnExit_params;
extern ENGINE_API string_path g_sLaunchWorkingFolder;

size_t CVersionSwitcher::GetVerCount()
{
    if (!s_switcher.inited)
        s_switcher.Init();
    return s_switcher.GetVerCountInternal();
}

size_t CVersionSwitcher::GetVerCountInternal() const { return versions.size(); }

const SVersionDescription& CVersionSwitcher::GetVerDesc(size_t idx)
{
    if (!s_switcher.inited)
        s_switcher.Init();
    return s_switcher.GetVerDescInternal(idx);
}

const SVersionDescription& CVersionSwitcher::GetVerDescInternal(size_t idx) const
{
    R_ASSERT(idx < versions.size());
    return versions[idx];
}

void CVersionSwitcher::SwitchToGameVer(size_t idx, CVersionSwitcher::EVersionSwitchMode mode)
{
    R_ASSERT(s_switcher.inited);
    const SVersionDescription& desc = s_switcher.GetVerDescInternal(idx);
    pcstr args = (mode == SWITCH_TO_SERVER) ? desc.arguments_mp.c_str() : desc.arguments_mm.c_str();
    s_switcher.SwitchToGameVerInternal(desc.exe_path.c_str(), desc.working_dir.c_str(), args);
}

void CVersionSwitcher::SwitchToGameVer(pcstr name, EVersionSwitchMode mode)
{
    size_t idx = FindVersionIdByName(name);
    R_ASSERT(idx != CVersionSwitcher::VERSION_NOT_FOUND);
    SwitchToGameVer(idx, mode);
}

void CVersionSwitcher::SwitchToGameVerInternal(xr_string appexe, xr_string working_dir, xr_string args) const
{
    // Command line specifiers:
    // %SERVER% - address and port of the selected server (for multiplayer mode)
    // %PLAYERNAME% - nickname of the player in multiplayer
    // %SERVERPASSWORD% - password for connection to the server
    // %USERPASSWORD% - password for connection to the server with user's list

    args = xr_substrreplace(args, "%SERVER%", server);
    args = xr_substrreplace(args, "%PLAYERNAME%", name);
    args = xr_substrreplace(args, "%SERVERPASSWORD%", server_password);
    args = xr_substrreplace(args, "%USERPASSWORD%", user_password);

    xr_strcpy(g_sLaunchWorkingFolder, working_dir.c_str());
    xr_strcpy(g_sLaunchOnExit_app, appexe.c_str());
    xr_strcpy(g_sLaunchOnExit_params, appexe.c_str());
    xr_strcpy(g_sLaunchOnExit_params, " ");
    xr_strcat(g_sLaunchOnExit_params, args.c_str());

    Console->Execute("quit");
}

size_t CVersionSwitcher::FindVersionIdByName(pcstr version)
{
    if (!s_switcher.inited)
        s_switcher.Init();

    return s_switcher.FindVersionIdByNameInternal(version);
}

size_t CVersionSwitcher::FindVersionIdByNameInternal(pcstr version)
{
    size_t result = VERSION_NOT_FOUND;
    for (size_t i = 0; i < versions.size(); ++i)
    {
        if (xr_strcmp(versions[i].name.c_str(), version) == 0)
        {
            result = i;
            break;
        }
    }
    return result;
}

void CVersionSwitcher::SetupMPParams(pcstr name, pcstr srvpsw, pcstr userpsw, pcstr server)
{
    R_ASSERT(s_switcher.inited);
    s_switcher.SetupMPParamsInternal(name, srvpsw, userpsw, server);
}

void CVersionSwitcher::SetupMPParamsInternal(pcstr nick, pcstr srvpsw, pcstr userpsw, pcstr srv)
{
    server = srv;
    name = nick;
    server_password = srvpsw;
    user_password = userpsw;
}

void CVersionSwitcher::ParseVersionConfig(const string_path& cfg)
{
    const char* SECTION = "ver_desc";
    CInifile ini(cfg);
    SVersionDescription desc;

    desc.name = READ_IF_EXISTS(&ini, r_string, SECTION, "name", "");
    desc.description = READ_IF_EXISTS(&ini, r_string_wb, SECTION, "description", "");
    desc.exe_path = READ_IF_EXISTS(&ini, r_string_wb, SECTION, "exe_path", "");
    desc.arguments_mm = READ_IF_EXISTS(&ini, r_string_wb, SECTION, "arguments_mm", "");
    desc.arguments_mp = READ_IF_EXISTS(&ini, r_string_wb, SECTION, "arguments_mp", "");

    if (desc.name.size() > 0)
    {
        xr_string dir = cfg;
        size_t pos = dir.rfind(_DELIMITER);
        dir = dir.substr(0, pos);

        if (desc.exe_path.size() < 1)
        {
            // Use current engine
            xr_string exe_path = Core.ApplicationPath;

            // TODO: Create a cross-platform way to restart the current engine, also should be useful for dedicated
            // server (maybe use xrCore class)
            exe_path += "xrEngine.exe";

            desc.exe_path = exe_path.c_str();
            desc.working_dir = dir.c_str();
        }
        else
        {
            xr_string exe_path = dir + _DELIMITER + desc.exe_path.c_str();
            desc.exe_path = exe_path.c_str();
            desc.working_dir = dir.c_str();
        }

        versions.push_back(desc);
    }
}

void CVersionSwitcher::ReloadInternal()
{
    const char* VERSIONS_ROOT = "$game_versions$";

    if (FS.path_exist(VERSIONS_ROOT))
    {
        FS_FileSet vers;
        FS.file_list(vers, VERSIONS_ROOT, FS_ListFiles, "*verdesc.ltx");

        for (const FS_File& cfg : vers)
        {
            string_path file_name;
            FS.update_path(file_name, VERSIONS_ROOT, cfg.name.c_str());
            ParseVersionConfig(file_name);
        }
    }
}

void CVersionSwitcher::Init()
{
    ReloadInternal();
    inited = true;
}
