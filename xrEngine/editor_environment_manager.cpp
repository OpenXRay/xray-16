////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_manager.cpp
//	Created 	: 12.12.2007
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_manager.hpp"
#include "editor_environment_suns_manager.hpp"
#include "editor_environment_levels_manager.hpp"
#include "editor_environment_effects_manager.hpp"
#include "editor_environment_sound_channels_manager.hpp"
#include "editor_environment_ambients_manager.hpp"
#include "editor_environment_thunderbolts_manager.hpp"
#include "editor_environment_weathers_manager.hpp"
#include "editor_environment_detail.hpp"
#include "ide.hpp"
#include "../xrServerEntities/object_broker.h"
#include "LightAnimLibrary.h"
#include "editor_environment_weathers_time.hpp"
#include "../include/xrrender/particles_systems_library_interface.hpp"
#include "../Include/xrRender/RenderDeviceRender.h"
#include "editor_environment_ambients_ambient.hpp"
#include "xr_efflensflare.h"

using editor::environment::manager;
using editor::environment::detail::logical_string_predicate;
using particles_systems::library_interface;

manager::manager												() :
	m_suns						(0),
	m_levels					(0),
	m_effects					(0),
	m_sound_channels			(0),
	m_ambients					(0),
	m_thunderbolts				(0),
	m_weathers					(0)
{
	m_effects					= xr_new<editor::environment::effects::manager>(this);
	m_sound_channels			= xr_new<editor::environment::sound_channels::manager>();
	m_ambients					= xr_new<editor::environment::ambients::manager>(*this);
	m_weathers					= xr_new<editor::environment::weathers::manager>(this);
	m_suns						= xr_new<editor::environment::suns::manager>(this);
	m_levels					= xr_new<editor::environment::levels::manager>(m_weathers);
	m_thunderbolts				= xr_new<editor::environment::thunderbolts::manager>(this);

	load_internal				();
	fill						();
}

manager::~manager												()
{
	xr_delete					(m_thunderbolts);
	xr_delete					(m_ambients);
	xr_delete					(m_sound_channels);
	xr_delete					(m_effects);
	xr_delete					(m_levels);
	xr_delete					(m_weathers);
	xr_delete					(m_suns);

	delete_data					(m_shader_ids);
	delete_data					(m_light_animator_ids);

	WeatherCycles.clear			();
	WeatherFXs.clear			();

	if (!Device.editor())
		return;

	::ide().destroy				(m_property_holder);
}

void manager::load												()
{
}

void manager::load_internal										()
{
	m_thunderbolts->load		();
	m_suns->load				();
	m_levels->load				();
	m_effects->load				();
	m_sound_channels->load		();
	m_ambients->load			();

	inherited::load				();
}

void manager::save			()
{
	m_weathers->save			();
//	m_suns->save				();
	m_ambients->save			();
	m_effects->save				();
	m_sound_channels->save		();
	m_thunderbolts->save		();
//	m_levels->save				();
}

void manager::fill												()
{
	m_property_holder			= ::ide().create_property_holder("environment");

	m_weathers->fill			(m_property_holder);
	m_suns->fill				(m_property_holder);
	m_ambients->fill			(m_property_holder);
	m_effects->fill				(m_property_holder);
	m_sound_channels->fill		(m_property_holder);
	m_thunderbolts->fill		(m_property_holder);
	m_levels->fill				();

	::ide().environment_weathers(m_property_holder);
}

void manager::load_weathers										()
{
	m_weathers->load			();

	// sorting weather envs
	EnvsMapIt _I=WeatherCycles.begin();
	EnvsMapIt _E=WeatherCycles.end();
	for (; _I!=_E; _I++){
		R_ASSERT3	(_I->second.size()>1,"Environment in weather must >=2",*_I->first);
		std::sort(_I->second.begin(),_I->second.end(),sort_env_etl_pred);
	}
	R_ASSERT2	(!WeatherCycles.empty(),"Empty weathers.");
	SetWeather	((*WeatherCycles.begin()).first.c_str());
}

manager::shader_ids_type const& manager::shader_ids				() const
{
	if (!m_shader_ids.empty())
		return					(m_shader_ids);

	string_path					path;
	FS.update_path				(path, "$game_data$", "shaders.xr");
	IReader*					reader = FS.r_open(path); 
	IReader*					stream = reader->open_chunk(3);
	R_ASSERT					(stream);

	u32							count = stream->r_u32();
	m_shader_ids.resize			(count);
	shader_ids_type::iterator	i = m_shader_ids.begin();
	shader_ids_type::iterator	e = m_shader_ids.end();
	for ( ; i != e; ++i) {
		string_path				buffer;
		stream->r_stringZ		(buffer, sizeof(buffer)); 
		*i						= xr_strdup(buffer);
	}
	
	stream->close				();
	FS.r_close					(reader);

	std::sort					(m_shader_ids.begin(), m_shader_ids.end(), logical_string_predicate());

	return						(m_shader_ids);
}

manager::particle_ids_type const& manager::particle_ids			() const
{
	if (!m_particle_ids.empty())
		return					(m_particle_ids);

	library_interface const&	library = m_pRender->particles_systems_library();
	PS::CPGDef const* const*	i = library.particles_group_begin();
	PS::CPGDef const* const*	e = library.particles_group_end();
	for ( ; i != e; library.particles_group_next(i))
		m_particle_ids.push_back(library.particles_group_id(**i).c_str());

	std::sort					(m_particle_ids.begin(), m_particle_ids.end(), logical_string_predicate());
	return						(m_particle_ids);
}

manager::light_animator_ids_type const& manager::light_animator_ids	() const
{
	if (!m_light_animator_ids.empty())
		return								(m_light_animator_ids);

	typedef LAItemVec						container_type;
	container_type const&					light_animators = LALib.Objects();
	m_light_animator_ids.resize				(light_animators.size());
	container_type::const_iterator			i = light_animators.begin();
	container_type::const_iterator			e = light_animators.end();
	light_animator_ids_type::iterator		j = m_light_animator_ids.begin();
	for ( ; i != e; ++i, ++j)
		*j									= xr_strdup((*i)->cName.c_str());

	std::sort								(m_light_animator_ids.begin(), m_light_animator_ids.end(), logical_string_predicate());

	return									(m_light_animator_ids);
}

void manager::create_mixer	()
{
	VERIFY								(!CurrentEnv);
	editor::environment::weathers::time	*object = xr_new<editor::environment::weathers::time>(this, (editor::environment::weathers::weather const*)0, "");
	CurrentEnv							= object;
	object->fill						(0);
}

void manager::unload		()
{
	WeatherCycles.clear			();
	WeatherFXs.clear			();
	Modifiers.clear				();
	Ambients.clear				();
}

CEnvAmbient* manager::AppendEnvAmb		(const shared_str& sect)
{
	return						(m_ambients->get_ambient(sect));
}

SThunderboltDesc* manager::thunderbolt_description		(CInifile& config, shared_str const& section)
{
	return						(m_thunderbolts->description(config, section));
}

SThunderboltCollection* manager::thunderbolt_collection	(CInifile* pIni, CInifile* thunderbolts, LPCSTR section)
{
	return						(m_thunderbolts->get_collection(section));
}

SThunderboltCollection* manager::thunderbolt_collection	(xr_vector<SThunderboltCollection*>& collection,  shared_str const& id)
{
	return						(m_thunderbolts->get_collection(id));
}

CLensFlareDescriptor*   manager::add_flare				(xr_vector<CLensFlareDescriptor*>& collection, shared_str const& id)
{
#if 0
//	return						(m_suns->get_flare(id));
	typedef xr_vector<CLensFlareDescriptor*>	container_type;
	container_type::iterator	i = collection.begin();
	container_type::iterator	e = collection.end();
	for ( ; i != e; ++i)
		if ((*i)->section == id)
			return				(*i);

	NODEFAULT;
#ifdef DEBUG
	return						(0);
#endif // #ifdef DEBUG
#endif // #if 0
	return						(inherited::add_flare(collection, id));
}

#endif // #ifdef INGAME_EDITOR