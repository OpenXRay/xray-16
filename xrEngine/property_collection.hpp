////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection.hpp
//	Created 	: 12.12.2007
//  Modified 	: 27.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property collection template class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLLECTION_HPP_INCLUDED
#define PROPERTY_COLLECTION_HPP_INCLUDED

#include "../include/editor/property_holder.hpp"
#include <boost/noncopyable.hpp>
#include "../xrServerEntities/object_broker.h"

template <typename container_type, typename holder_type>
class property_collection :
	public editor::property_holder_collection,
	private boost::noncopyable
{
public:
	typedef editor::property_holder					property_holder;

public:
	inline						property_collection	(container_type* container, holder_type* holder, bool* changed = 0);
	virtual						~property_collection();
	inline	holder_type&		holder				() const;
	virtual	void				clear				();
	virtual	u32					size				();
	virtual	void				insert				(property_holder* holder, u32 const& position);
	virtual	void				erase				(u32 const& position);
	virtual	property_holder*	item				(u32 const& position);
	virtual	int					index				(property_holder* holder);
	virtual	void				destroy				(property_holder* holder);

public:
	// these functions should be implemented for each specializations
	// in the place when they are visible by the specialization user
	virtual	void				display_name		(u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size);
	virtual	property_holder*	create				();

public:
			bool				unique_id			(LPCSTR id) const;
			shared_str			generate_unique_id	(LPCSTR prefix) const;

private:
	inline	void				make_state_changed	();

private:
	struct predicate {
		property_holder*		m_holder;

		inline					predicate			(property_holder* holder);
		inline bool				operator()			(typename container_type::value_type const& value) const;
	};

private:
	struct unique_id_predicate {
		LPCSTR					m_id;

		inline					unique_id_predicate	(LPCSTR id);
		inline bool				operator()			(typename container_type::value_type const& value) const;
	};

private:
	container_type&				m_container;
	holder_type&				m_holder;
	bool*						m_changed;
}; // class property_collection

#include "property_collection_inline.hpp"

#endif // #ifndef PROPERTY_COLLECTION_HPP_INCLUDED