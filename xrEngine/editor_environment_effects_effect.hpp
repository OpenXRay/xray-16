////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_effects_effect.hpp
//	Created 	: 28.12.2007
//  Modified 	: 28.12.2007
//	Author		: Dmitriy Iassenev
//	Description : editor environment effects effect class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_EFFECTS_EFFECT_HPP_INCLUDED
#define EDITOR_WEATHER_EFFECTS_EFFECT_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "../include/editor/property_holder.hpp"
#include "environment.h"

namespace editor {
namespace environment {

class manager;

namespace effects {

class manager;

class effect :
	public CEnvAmbient::SEffect,
	public editor::property_holder_holder,
	private boost::noncopyable
{
public:
							effect		(manager const& manager, shared_str const& id);
	virtual					~effect		();
			void			load		(CInifile& config);
			void			save		(CInifile& config);
			void			fill		(editor::property_holder_collection* collection);
	inline	LPCSTR			id			() const { return m_id.c_str(); }


private:
	LPCSTR xr_stdcall		id_getter	() const;
	void   xr_stdcall		id_setter	(LPCSTR value);

	float xr_stdcall		wind_blast_longitude_getter	() const;
	void  xr_stdcall		wind_blast_longitude_setter	(float value);


	LPCSTR xr_stdcall sound_getter		();
	void xr_stdcall	  sound_setter		(LPCSTR value);

private:
	typedef editor::property_holder	property_holder_type;

public:
	virtual	property_holder_type*object	();

private:
	shared_str				m_id;
	property_holder_type*	m_property_holder;
	shared_str				m_sound;

public:
	manager const&			m_manager;
}; // class effect

} // namespace effects
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_EFFECTS_EFFECT_HPP_INCLUDED