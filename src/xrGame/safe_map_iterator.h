////////////////////////////////////////////////////////////////////////////
//	Module 		: safe_map_iterator.h
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Safe map iterator template
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
	typename _key_type,
	typename _data_type,
	typename _predicate = std::less<_key_type>,
	bool	 use_time_limit = true,
	typename _cycle_type = u64,
	bool	 use_first_update = true
>
class CSafeMapIterator {
public:
	typedef xr_map<_key_type,_data_type*,_predicate>	_REGISTRY;
	typedef typename _REGISTRY::iterator				_iterator;
	typedef typename _REGISTRY::const_iterator			_const_iterator;

protected:
	_REGISTRY				m_objects;
	_cycle_type				m_cycle_count;
	_iterator				m_next_iterator;
	CTimer					m_timer;
	float					m_max_process_time;
	bool					m_first_update;

protected:
	IC		void			update_next			();
	IC		_iterator		&next				();
	IC		void			start_timer			();
	IC		bool			time_over			();

public:
	IC						CSafeMapIterator	();
	virtual					~CSafeMapIterator	();
	IC		void			add					(const _key_type &id, _data_type *value, bool no_assert = false);
	IC		void			remove				(const _key_type &id, bool no_assert = false);
	template <typename _update_predicate>
	IC		u32				update				(const _update_predicate &predicate, bool const iterate_as_first_time_next_time);
	IC		void			set_process_time	(const float &process_time);
	IC		const _REGISTRY	&objects			() const;
	IC		void			clear				();
	IC		bool			empty				() const;
	IC		void			begin				();
};

#include "safe_map_iterator_inline.h"