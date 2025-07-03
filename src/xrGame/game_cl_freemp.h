#pragma once
#include "game_cl_mp.h"

class CUIGameFMP;

class game_cl_freemp :public game_cl_mp
{
private:
	typedef game_cl_mp inherited;
	CUIGameFMP *m_game_ui;

public:
			game_cl_freemp();
	virtual	~game_cl_freemp();


	virtual CUIGameCustom* createGameUI();
	virtual void SetGameUI(CUIGameCustom*);


	virtual	void net_import_state(NET_Packet& P);
	virtual	void net_import_update(NET_Packet& P);
	
	virtual void shedule_Update(u32 dt);

	virtual	bool OnKeyboardPress(int key);

	virtual void TranslateGameMessage(u32 msg, NET_Packet& P);

	virtual LPCSTR GetGameScore(string32&	score_dest);
	virtual bool Is_Rewarding_Allowed()  const { return false; };

	virtual void OnConnected();

};

