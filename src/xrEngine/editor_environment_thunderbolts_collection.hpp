////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_thunderbolts_collection.hpp
//	Created 	: 10.01.2008
//  Modified 	: 10.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment thunderbolts collection identifier class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_THUNDERBOLTS_COLLECTION_ID_HPP_INCLUDED
#define EDITOR_WEATHER_THUNDERBOLTS_COLLECTION_ID_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "../include/editor/property_holder.hpp"
#include "property_collection_forward.hpp"
#include "thunderbolt.h"

namespace editor {

class property_holder_collection;

namespace environment {
namespace thunderbolts {

class manager;
class thunderbolt_id;

class collection :
	public SThunderboltCollection,
	public editor::property_holder_holder,
	private boost::noncopyable {
public:
							collection		(manager const& manager, shared_str const& id);
	virtual					~collection		();
			void			load			(CInifile& config);
			void			save			(CInifile& config);
			void			fill			(editor::property_holder_collection* collection);
	inline	LPCSTR			id				() const { return section.c_str(); }


private:
			LPCSTR	xr_stdcall	id_getter	() const;
			void	xr_stdcall	id_setter	(LPCSTR value);
private:
	typedef editor::property_holder			property_holder_type;

public:
	virtual	property_holder_type* object	();

private:
	typedef xr_vector<thunderbolt_id*>						container_type;
	typedef property_collection<container_type, collection>	collection_type;

private:
	container_type			m_ids;
	collection_type*		m_collection;
	property_holder_type*	m_property_holder;

public:
	manager const&			m_manager;
}; // class collection
} // namespace thunderbolts
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_THUNDERBOLTS_COLLECTION_ID_HPP_INCLUDED