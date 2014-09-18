////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_weathers_time.hpp
//	Created 	: 12.01.2008
//  Modified 	: 12.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment weathers time class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_ENVIRONMENT_WEATHERS_TIME_HPP_INCLUDED
#define EDITOR_ENVIRONMENT_WEATHERS_TIME_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include <boost/noncopyable.hpp>
#include "../include/editor/property_holder.hpp"
#include "environment.h"

namespace editor {
namespace environment {

class manager;

namespace weathers {

class weather;

class time :
	public CEnvDescriptorMixer,
	public editor::property_holder_holder,
	private boost::noncopyable
{
private:
	typedef CEnvDescriptorMixer			inherited;

public:
	typedef editor::property_holder		property_holder_type;

public:
										time						(
											editor::environment::manager* manager,
											weather const* weather,
											shared_str const& id
										);
	virtual								~time						();
			void						load						(CInifile& config);
			void						load_from					(shared_str const& id, CInifile& config, shared_str const& new_id);
			void						save						(CInifile& config);
			void						fill						(::editor::property_holder_collection* holder);
	inline	shared_str const&			id							() const { return m_identifier; }
	virtual	property_holder_type*		object						() { return m_property_holder; }
	virtual	void						lerp						(CEnvironment* parent, CEnvDescriptor& A, CEnvDescriptor& B, float f, CEnvModifier& M, float m_power);

private:
	LPCSTR const*	xr_stdcall			ambients_collection			();
	u32				xr_stdcall			ambients_collection_size	();
	LPCSTR const*	xr_stdcall			suns_collection				();
	u32				xr_stdcall			suns_collection_size		();
	LPCSTR const*	xr_stdcall			thunderbolts_collection		();
	u32				xr_stdcall			thunderbolts_collection_size();

private:
	LPCSTR			xr_stdcall			id_getter					() const;
	void			xr_stdcall			id_setter					(LPCSTR value);
	float			xr_stdcall			sun_altitude_getter			() const;
	void			xr_stdcall			sun_altitude_setter			(float value);
	float			xr_stdcall			sun_longitude_getter		() const;
	void			xr_stdcall			sun_longitude_setter		(float value);
	float			xr_stdcall			sky_rotation_getter			() const;
	void			xr_stdcall			sky_rotation_setter			(float value);
	float			xr_stdcall			wind_direction_getter		() const;
	void			xr_stdcall			wind_direction_setter		(float value);
	LPCSTR			xr_stdcall			ambient_getter				() const;
	void			xr_stdcall			ambient_setter				(LPCSTR value);
	LPCSTR			xr_stdcall			sun_getter					() const;
	void			xr_stdcall			sun_setter					(LPCSTR value);
	LPCSTR			xr_stdcall			thunderbolt_getter			() const;
	void			xr_stdcall			thunderbolt_setter			(LPCSTR value);
	LPCSTR			xr_stdcall			sky_texture_getter			() const;
	void			xr_stdcall			sky_texture_setter			(LPCSTR value);
	LPCSTR			xr_stdcall			clouds_texture_getter		() const;
	void			xr_stdcall			clouds_texture_setter		(LPCSTR value);

private:
	shared_str							m_ambient;
	shared_str							m_sun;
	shared_str							m_thunderbolt_collection;

private:
	editor::environment::manager&		m_manager;
	weather const*						m_weather;
	property_holder_type*				m_property_holder;
}; // class time
} // namespace weathers
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_ENVIRONMENT_WEATHERS_TIME_HPP_INCLUDED