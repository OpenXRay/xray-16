#pragma once
#include "xrCore/xrstring.h"
#include "xrCommon/xr_vector.h"

struct SVersionDescription
{
    shared_str name;
    shared_str description;
    shared_str exe_path;
    shared_str working_dir;
    shared_str arguments_mm;
    shared_str arguments_mp;
};

class CVersionSwitcher
{
public:
    static size_t GetVerCount();
    static const SVersionDescription& GetVerDesc(size_t idx);

    enum EVersionSwitchMode
    {
        SWITCH_TO_MAINMENU,
        SWITCH_TO_SERVER,
        SWITCH_COUNT,
    };
    static void SwitchToGameVer(size_t idx, EVersionSwitchMode mode);
    static void SwitchToGameVer(pcstr name, EVersionSwitchMode mode);
    static void SetupMPParams(pcstr name, pcstr srvpsw, pcstr userpsw, pcstr server);

    static size_t FindVersionIdByName(pcstr version);
    static const size_t VERSION_NOT_FOUND = size_t(-1);

private:
    size_t GetVerCountInternal() const;
    const SVersionDescription& GetVerDescInternal(size_t idx) const;
    void SwitchToGameVerInternal(xr_string appexe, xr_string working_dir, xr_string args) const;
    void SetupMPParamsInternal(pcstr nick, pcstr srvpsw, pcstr userpsw, pcstr srv);
    size_t FindVersionIdByNameInternal(pcstr version);

    void ReloadInternal();
    void Init();
    void ParseVersionConfig(const string_path& cfg);

    bool inited;
    xr_vector<SVersionDescription> versions;

    xr_string server;
    xr_string name;
    xr_string server_password;
    xr_string user_password;
};
