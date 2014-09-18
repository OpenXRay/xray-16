#pragma once
#include "monster_event_manager_defs.h"

typedef fastdelegate::FastDelegate1<IEventData*> typeEvent;

class CMonsterEventManager {
	
	// delayed remove
	struct event_struc {
		typeEvent	delegate;
		bool		need_remove;
		
		event_struc		(typeEvent e) : delegate(e) {need_remove = false;}
	};
	
	struct pred_remove {
		bool operator() (const event_struc &param) {return param.need_remove;}
	};

	DEFINE_VECTOR	(event_struc,		EVENT_VECTOR, EVENT_VECTOR_IT);
	DEFINE_MAP		(EEventType,		EVENT_VECTOR, EVENT_MAP, EVENT_MAP_IT);

	EVENT_MAP		m_event_storage;
public:
				CMonsterEventManager	();
				~CMonsterEventManager	();

	void		add_delegate			(EEventType event, typeEvent delegate);
	void		remove_delegate			(EEventType event, typeEvent delegate);
	
	void		raise					(EEventType, IEventData *data = 0);

private:
	void		clear		();
};


