#ifndef LEVEL_NETWORK_MAP_SYNC_H
#define LEVEL_NETWORK_MAP_SYNC_H


struct LevelMapSyncData
{
	bool						m_sended_map_name_request;
	bool						m_map_sync_received;
	bool						m_map_loaded;
	shared_str					m_name;				//map name that currently loaded
	shared_str					m_map_version;		//map version that currently loaded
	shared_str					m_map_download_url;
	u32							m_level_geom_crc32;
	

	u32							m_wait_map_time;
	
	//next members changed by level message receiver
	bool						invalid_geom_checksum;
	bool						invalid_map_or_version;
	
	LevelMapSyncData()
	{
		m_sended_map_name_request = false;
		m_map_sync_received	= false;
		invalid_map_or_version = false;
		m_map_loaded = false;
		invalid_geom_checksum = false;
		m_level_geom_crc32 = 0;
		m_wait_map_time = 0;
	}
	~LevelMapSyncData()
	{
	}
	void	CheckToSendMapSync();
	void	ReceiveServerMapSync(NET_Packet& P);
	inline bool IsInvalidMapOrVersion()		{ return invalid_map_or_version; }
	inline bool	IsInvalidClientChecksum()	{ return invalid_geom_checksum; }


}; //class LevelMapSyncData

#endif