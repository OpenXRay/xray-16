#include "stdafx.h"
#include "monster_event_manager.h"

CMonsterEventManager::CMonsterEventManager()
{
}

CMonsterEventManager::~CMonsterEventManager()
{
	clear	();
}

void CMonsterEventManager::add_delegate(EEventType event, typeEvent delegate) 
{
	EVENT_MAP_IT it = m_event_storage.find(event);	
	if (it == m_event_storage.end()) {
		std::pair< EVENT_MAP_IT, bool > res;
		res = m_event_storage.insert(mk_pair(event, EVENT_VECTOR()));
		it = res.first;
	}
	
	it->second.push_back(event_struc(delegate));
}

void CMonsterEventManager::remove_delegate(EEventType event, typeEvent delegate) 
{
	EVENT_MAP_IT it = m_event_storage.find(event);	
	if (it == m_event_storage.end()) return;
	
	for (EVENT_VECTOR_IT it_del = it->second.begin(); it_del != it->second.end(); ++it_del) {
		if (it_del->delegate == delegate) it_del->need_remove = true;
	}
}


void CMonsterEventManager::raise(EEventType event, IEventData *data)
{
	EVENT_MAP_IT it = m_event_storage.find(event);
	if (it == m_event_storage.end()) return;

	for (EVENT_VECTOR_IT I=it->second.begin(); I != it->second.end(); I++) {
		if (!I->need_remove) (I->delegate)(data);
	}

	EVENT_VECTOR_IT it_del = std::remove_if(it->second.begin(),it->second.end(), pred_remove());
	it->second.erase(it_del,it->second.end());
}

void CMonsterEventManager::clear()
{
	for (EVENT_MAP_IT I_map = m_event_storage.begin(); I_map != m_event_storage.end(); I_map++) {
		I_map->second.clear();
	}
	
	m_event_storage.clear();
}


