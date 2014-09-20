////////////////////////////////////////////////////////////////////////////
//	Module 		: hit_memory_manager.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Hit memory manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "memory_space.h"

#ifdef DEBUG
#	define USE_SELECTED_HIT
#endif

namespace MemorySpace {
	struct CHitObject;
};

class CEntityAlive;
class CCustomMonster;
class CAI_Stalker;

class CHitMemoryManager {
public:
	typedef MemorySpace::CHitObject					CHitObject;
	typedef xr_vector<CHitObject>					HITS;

private:
	struct CDelayedHitObject {
		ALife::_OBJECT_ID		m_object_id;
		CHitObject				m_hit_object;
	};

private:
	typedef xr_vector<CDelayedHitObject>			DELAYED_HIT_OBJECTS;

private:
	CCustomMonster				*m_object;
	CAI_Stalker					*m_stalker;
	HITS						*m_hits;
	DELAYED_HIT_OBJECTS			m_delayed_objects;
	u32							m_max_hit_count;
#ifdef USE_SELECTED_HIT
	CHitObject					*m_selected_hit;
#endif
	ALife::_OBJECT_ID			m_last_hit_object_id;
	u32							m_last_hit_time;

public:
	IC							CHitMemoryManager	(CCustomMonster *object, CAI_Stalker *stalker);
	virtual						~CHitMemoryManager	();
	virtual	void				Load				(LPCSTR section);
	virtual	void				reinit				();
	virtual	void				reload				(LPCSTR section);
	virtual void				update				();
			void				remove_links		(CObject *object);

public:
			void				add					(const CEntityAlive	*who);
			void				add					(float amount, const Fvector &local_direction, const CObject *who, s16 element);
			void				add					(const CHitObject &hit_object);

public:
	IC		const HITS			&objects			() const;
#ifdef USE_SELECTED_HIT
	IC		const CHitObject	*hit				() const;
#endif
			const CHitObject	*hit				(const CEntityAlive *object) const;
	IC		void				set_squad_objects	(HITS *squad_objects);
	IC		const ALife::_OBJECT_ID	&last_hit_object_id	() const;
	IC		u32 const &			last_hit_time			() const;

public:
			void				enable				(const CObject *object, bool enable);
			CCustomMonster		&object				() const;

public:
			void				save				(NET_Packet &packet) const;
			void				load				(IReader &packet);
			void				on_requested_spawn	(CObject *object);

private:
			void				clear_delayed_objects	();
};

#include "hit_memory_manager_inline.h"