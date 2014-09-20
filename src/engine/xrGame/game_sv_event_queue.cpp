#include "stdafx.h"
#include "game_sv_event_queue.h"


// 
GameEventQueue::GameEventQueue()		
#ifdef PROFILE_CRITICAL_SECTIONS
	:cs(MUTEX_PROFILE_ID(GameEventQueue))
#endif // PROFILE_CRITICAL_SECTIONS
{
	unused.reserve	(128);
	for (int i=0; i<16; i++)
		unused.push_back	(xr_new<GameEvent>());
}
GameEventQueue::~GameEventQueue()
{
	cs.Enter		();
	u32				it;
	for				(it=0; it<unused.size(); it++)	xr_delete(unused[it]);
	for				(it=0; it<ready.size(); it++)	xr_delete(ready[it]);
	cs.Leave		();
}

static u32 LastTimeCreate = 0;
GameEvent*		GameEventQueue::Create	()
{
	GameEvent*	ge			= 0;
	cs.Enter		();
	if (unused.empty())	
	{
		ready.push_back		(xr_new<GameEvent> ());
		ge					= ready.back	();
		//---------------------------------------------
#ifdef _DEBUG
//		Msg ("* GameEventQueue::Create - ready %d, unused %d", ready.size(), unused.size());
#endif
		LastTimeCreate = GetTickCount();
		//---------------------------------------------
	} else {
		ready.push_back		(unused.back());
		unused.pop_back		();
		ge					= ready.back	();
	}
	cs.Leave		();
	return	ge;
}

GameEvent*		GameEventQueue::CreateSafe	(NET_Packet& P, u16 type, u32 time, ClientID clientID)
{
	if (m_blocked_clients.size())
	{
		if (m_blocked_clients.find(clientID) != m_blocked_clients.end())
		{
#ifdef DEBUG
			Msg("--- Ignoring event type[%d] time[%d] clientID[0x%08x]", type, time, clientID);
#endif // #ifdef DEBUG
			return NULL;
		}
	}
	return Create(P, type, time, clientID);
}

GameEvent*		GameEventQueue::Create	(NET_Packet& P, u16 type, u32 time, ClientID clientID)
{
	GameEvent*	ge			= 0;
	cs.Enter		();
	if (unused.empty())	
	{
		ready.push_back		(xr_new<GameEvent> ());
		ge					= ready.back	();
		//---------------------------------------------
#ifdef _DEBUG
//		Msg ("* GameEventQueue::Create - ready %d, unused %d", ready.size(), unused.size());
#endif
		LastTimeCreate = GetTickCount();
		//---------------------------------------------
	} else {
		ready.push_back		(unused.back());
		unused.pop_back		();
		ge					= ready.back	();
	}
	CopyMemory	(&(ge->P),&P,sizeof(NET_Packet));
	ge->sender	= clientID;
	ge->time	= time;
	ge->type	= type;

	cs.Leave		();
	return			ge;
}
GameEvent*		GameEventQueue::Retreive	()
{
	GameEvent*	ge			= 0;
	cs.Enter		();
	if (!ready.empty())		ge = ready.front();
	//---------------------------------------------	
	else
	{
		u32 tmp_time = GetTickCount()-60000;
		u32 size = unused.size();
		if ((LastTimeCreate < tmp_time) &&  (size > 32))
		{
			xr_delete(unused.back());
			unused.pop_back();
#ifdef _DEBUG
//			Msg ("GameEventQueue::Retreive - ready %d, unused %d", ready.size(), unused.size());
#endif
		}		
	}
	//---------------------------------------------	
	cs.Leave		();
	return	ge;
}

void			GameEventQueue::Release	()
{
	cs.Enter		();
	R_ASSERT		(!ready.empty());
	//---------------------------------------------
	u32 tmp_time = GetTickCount()-60000;
	u32 size = unused.size();
	if ((LastTimeCreate < tmp_time) &&  (size > 32))
	{
		xr_delete(ready.front());
#ifdef _DEBUG
//		Msg ("GameEventQueue::Release - ready %d, unused %d", ready.size(), unused.size());
#endif
	}
	else
		unused.push_back(ready.front());
	//---------------------------------------------		
	ready.pop_front	();
	cs.Leave		();
}

void GameEventQueue::SetIgnoreEventsFor(bool ignore, ClientID clientID)
{
	if (ignore)
	{
#ifdef DEBUG
		Msg("--- Setting ignore messages for client 0x%08x", clientID);
#endif // #ifdef DEBUG
		m_blocked_clients.insert(clientID);	
	}
	else
	{
#ifdef DEBUG
		Msg("--- Setting receive messages for client 0x%08x", clientID);
#endif // #ifdef DEBUG
		m_blocked_clients.erase(clientID);
	}
}

u32 GameEventQueue::EraseEvents(event_predicate to_del)
{
	u32 ret_val = 0;
	cs.Enter();
	if (ready.empty())	//read synchronization...
	{
		cs.Leave();
		return 0;
	}
	typedef xr_deque<GameEvent*>	event_queue;
	typedef event_queue::iterator	eq_iterator;
	
	eq_iterator need_to_erase = std::find_if(ready.begin(), ready.end(), to_del);
	while (need_to_erase != ready.end())
	{
		//-----
		u32 tmp_time = GetTickCount() - 60000;
		u32 size = unused.size();
		if ((LastTimeCreate < tmp_time) &&  (size > 32))
		{
			xr_delete(*need_to_erase);
#ifdef _DEBUG
//			Msg ("GameEventQueue::EraseEvents - ready %d, unused %d", ready.size(), unused.size());
#endif
		} else
		{
			unused.push_back(*need_to_erase);
		}
		//-----
#ifdef DEBUG
		Msg("! GameEventQueue::EraseEvents - destroying event type[%d], sender[0x%08x]", (*need_to_erase)->type, (*need_to_erase)->sender);
#endif
		ready.erase(need_to_erase);
		++ret_val;
		need_to_erase = std::find_if(ready.begin(), ready.end(), to_del);
	}
	cs.Leave();
	return ret_val;
}
