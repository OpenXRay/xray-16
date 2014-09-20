#pragma once

class CUIOptConCom {
public:
	void Init();
	CUIOptConCom();
protected:
	enum{
		flNetSrvDedicated				= 1,
		flNetConPublicServer			= 2,
		flNetConSpectatorOn				= 8,
	};
	enum{
		fl_empty						= (1<<0),
		fl_full							= (1<<1),
		fl_pass							= (1<<2),
		fl_wo_pass						= (1<<3),
		fl_wo_ff						= (1<<4),
		fl_listen						= (1<<5),
	};
    int			m_iMaxPlayers;
	Flags32		m_uNetSrvParams;
	Flags32		m_uNetFilter;
	u32			m_curGameMode;
	string64	m_playerName;
	string64	m_serverName;
	int			m_iNetConSpectator;
	int			m_iReinforcementType;
	//string64	m_sReinforcementType;
	float		m_fNetWeatherRate;

	void		ReadPlayerNameFromRegistry		();
	void		WritePlayerNameToRegistry		();
};