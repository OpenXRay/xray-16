#pragma once
#include "xrCore/xrstring.h"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_map.h"

class CUIOptionsItem;

class XRUICORE_API CUIOptionsManager
{
public:
    CUIOptionsManager();

    void SaveBackupValues(const shared_str& group);
    void SetCurrentValues(const shared_str& group);
    void SaveValues(const shared_str& group);
    void UndoGroup(const shared_str& group);

    void OptionsPostAccept();
    void DoVidRestart();
    void DoSndRestart();
    void DoUIRestart();
    void DoSystemRestart();

    bool NeedSystemRestart() { return 0 != (m_restart_flags & e_system_restart); }
    bool NeedVidRestart() { return 0 != (m_restart_flags & e_vid_restart); }
    void SendMessage2Group(const shared_str& group, const char* message);

    void RegisterItem(CUIOptionsItem* item, const shared_str& group);
    void UnRegisterItem(CUIOptionsItem* item);

protected:
    typedef shared_str group_name;
    typedef xr_vector<CUIOptionsItem*> items_list;
    typedef xr_map<group_name, items_list> groups;
    typedef xr_map<group_name, items_list>::iterator groups_it;

    groups m_groups;

    enum
    {
        e_vid_restart = (1 << 0),
        e_snd_restart = (1 << 1),
        e_ui_restart = (1 << 2),
        e_system_restart = (1 << 3)
    };
    u16 m_restart_flags;
};
