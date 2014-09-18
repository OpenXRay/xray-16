////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_thunderbolts_thunderbolt_id.hpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment thunderbolts thunderbolt identifier class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_THUNDERBOLTS_THUNDERBOLT_ID_HPP_INCLUDED
#define EDITOR_WEATHER_THUNDERBOLTS_THUNDERBOLT_ID_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "../include/editor/property_holder.hpp"

namespace editor {

class property_holder_collection;

namespace environment {
namespace thunderbolts {

class manager;

class thunderbolt_id :
	public editor::property_holder_holder,
	private boost::noncopyable {
public:
							thunderbolt_id	(manager const& manager, shared_str const& thunderbolt);
	virtual					~thunderbolt_id	();
			void			fill			(editor::property_holder_collection* collection);
	inline	LPCSTR			id				() const { return m_id.c_str(); }

private:
	typedef editor::property_holder			property_holder_type;

public:
	virtual	property_holder_type* object	();

private:
	LPCSTR const* xr_stdcall collection		();
	u32  xr_stdcall			collection_size	();

private:
	property_holder_type*	m_property_holder;
	manager const&			m_manager;
	shared_str				m_id;
}; // class thunderbolt_id
} // namespace thunderbolts
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_THUNDERBOLTS_THUNDERBOLT_ID_HPP_INCLUDED