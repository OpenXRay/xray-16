////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_thunderbolts_thunderbolt.hpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment thunderbolts thunderbolt class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_THUNDERBOLTS_THUNDERBOLT_HPP_INCLUDED
#define EDITOR_WEATHER_THUNDERBOLTS_THUNDERBOLT_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "../include/editor/property_holder.hpp"
#include "editor_environment_thunderbolts_gradient.hpp"
#include "thunderbolt.h"

namespace editor {
namespace environment {

class manager;

namespace thunderbolts {

class manager;

class thunderbolt :
	public SThunderboltDesc,
	public editor::property_holder_holder,
	private boost::noncopyable
{
private:
	typedef SThunderboltDesc			inherited;

public:
							thunderbolt				(manager* manager, shared_str const& id);
	virtual					~thunderbolt			();
			void			load					(CInifile& config);
			void			save					(CInifile& config);
			void			fill					(::editor::environment::manager& environment, editor::property_holder_collection* collection);
	inline	LPCSTR			id						() const { return m_id.c_str(); }
	virtual	void			create_top_gradient		(CInifile& pIni, shared_str const& sect);
	virtual	void			create_center_gradient	(CInifile& pIni, shared_str const& sect);

private:
			LPCSTR xr_stdcall id_getter	() const;
			void   xr_stdcall id_setter	(LPCSTR value);
private:
	typedef editor::property_holder		property_holder_type;

public:
	virtual	property_holder_type* object();

private:
	shared_str				m_id;
	manager&				m_manager;
	property_holder_type*	m_property_holder;

private:
	gradient*				m_center;
	gradient*				m_top;
	shared_str				m_color_animator;
	shared_str				m_lighting_model;
	shared_str				m_sound;
}; // class thunderbolt

} // namespace thunderbolts
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_THUNDERBOLTS_THUNDERBOLT_HPP_INCLUDED