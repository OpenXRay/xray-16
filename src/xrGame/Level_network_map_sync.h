#ifndef LEVEL_NETWORK_MAP_SYNC_H
#define LEVEL_NETWORK_MAP_SYNC_H

struct LevelMapSyncData
{
    bool m_map_sync_received;
    bool m_map_loaded;
    shared_str m_name; // map name that currently loaded
    shared_str m_map_version; // map version that currently loaded
    shared_str m_map_download_url;

    LevelMapSyncData()
    {
        m_map_sync_received = false;
        m_map_loaded = false;
    }
    ~LevelMapSyncData() {}
}; // class LevelMapSyncData

#endif
