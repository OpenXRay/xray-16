#ifndef ACTOR_MP_SERVER_H
#define ACTOR_MP_SERVER_H

#include "xrServer_Objects_ALife_Monsters.h"
#include "actor_mp_state.h"

class CSE_ActorMP : public CSE_ALifeCreatureActor {
private:
	typedef CSE_ALifeCreatureActor	inherited;

private:
	actor_mp_state_holder	m_state_holder;
	bool					m_ready_to_update;

private:
			void			fill_state		(actor_mp_state &state);

public:
							CSE_ActorMP		(LPCSTR		section);
	virtual void 			UPDATE_Read		(NET_Packet &packet);
	virtual void 			UPDATE_Write	(NET_Packet &packet);
	virtual void 			STATE_Read		(NET_Packet &packet, u16 size);
	virtual void 			STATE_Write		(NET_Packet &packet);
	virtual BOOL			Net_Relevant	();

#ifdef XRGAME_EXPORTS
	virtual	void			on_death				(CSE_Abstract *killer);	
#endif
};

#endif // ACTOR_MP_SERVER_H