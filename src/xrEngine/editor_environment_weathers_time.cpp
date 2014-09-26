////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_weathers_time.cpp
//	Created 	: 12.01.2008
//  Modified 	: 12.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment weathers time class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_weathers_time.hpp"
#include "ide.hpp"
#include "editor_environment_weathers_weather.hpp"
#include "editor_environment_detail.hpp"
#include "editor_environment_manager.hpp"
#include "editor_environment_ambients_manager.hpp"
#include "editor_environment_suns_manager.hpp"
#include "editor_environment_thunderbolts_manager.hpp"
#include "xr_efflensflare.h"
#include "thunderbolt.h"

using editor::environment::weathers::time;
using editor::environment::weathers::weather;

static inline editor::color create_color(float const& r, float const& g, float const& b)
{
	editor::color			result;
	result.r				= r;
	result.g				= g;
	result.b				= b;
	return					(result);
}

time::time					(
		editor::environment::manager* manager,
		weather const* weather,
		shared_str const& id
	) :
	CEnvDescriptorMixer		(id),
	m_manager				(*manager),
	m_weather				(weather),
	m_property_holder		(0),
	m_ambient				(""),
	m_sun					(""),
	m_thunderbolt_collection("")
{
}

time::~time					()
{
	if (!Device.editor())
		return;

	::ide().destroy		(m_property_holder);
}

void time::load_from		(shared_str const& id, CInifile& config, shared_str const& new_id)
{
	m_identifier		= id;
	load				(config);
	m_identifier		= new_id;
}

void time::load				(CInifile& config)
{
//	Ivector3 tm					={0,0,0};
//	sscanf						(m_identifier.c_str(),"%d:%d:%d",&tm.x,&tm.y,&tm.z);
//	R_ASSERT3					((tm.x>=0)&&(tm.x<24)&&(tm.y>=0)&&(tm.y<60)&&(tm.z>=0)&&(tm.z<60),"Incorrect weather time",m_identifier.c_str());
//	exec_time					= tm.x*3600.f+tm.y*60.f+tm.z;
//	exec_time_loaded			= exec_time;

	m_ambient					= config.r_string(m_identifier, "ambient");
//	ambient						= config.r_fvector3(m_identifier, "ambient_color");
//	clouds_texture_name			= config.r_string(m_identifier, "clouds_texture");
//	far_plane					= config.r_float(m_identifier, 	"far_plane");
//	fog_distance				= config.r_float(m_identifier, 	"fog_distance");
//	fog_density					= config.r_float(m_identifier, 	"fog_density");
//	fog_color					= config.r_fvector3(m_identifier, "fog_color");
//	rain_color					= config.r_fvector3(m_identifier, "rain_color");
//	rain_density				= config.r_float(m_identifier,	"rain_density");
//	sky_color					= config.r_fvector3(m_identifier, "sky_color");
//	sky_rotation				= config.r_float(m_identifier,	"sky_rotation");
	
//	sky_texture_name			= config.r_string(m_identifier, "sky_texture");
//	string_path					st_env;
//	strconcat					(sizeof(st_env), st_env, sky_texture_name.c_str(), "#small");
//	sky_texture_env_name		= st_env;

//	sun_color					= config.r_fvector3(m_identifier, "sun_color");
//	m_fSunShaftsIntensity		= config.r_float(m_identifier,	"sun_shafts_intensity");
	m_sun						= config.r_string(m_identifier, "sun");
	m_thunderbolt_collection	= config.r_string(m_identifier, "thunderbolt_collection");
//	bolt_duration				= config.r_float(m_identifier, 	"bolt_duration");
//	bolt_period					= config.r_float(m_identifier, 	"bolt_period");
//	m_fWaterIntensity			= config.r_float(m_identifier, 	"water_intensity");
//	wind_direction				= config.r_float(m_identifier, 	"wind_direction");
//	wind_velocity				= config.r_float(m_identifier, 	"wind_velocity");

//	hemi_color					= config.r_fvector4(m_identifier, "hemi_color");

//	Fvector2					coords = config.r_fvector2(m_identifier, "sun_dir");
//	sun_dir.setHP				(deg2rad(coords.y), deg2rad(coords.x));

//	clouds_color				= config.r_fvector4(m_identifier, "clouds_color");

//	LPCSTR						clouds = config.r_string(m_identifier, "clouds_color");
//	VERIFY						(_GetItemCount(clouds) == 5);
//	string256					temp;
//	((Fvector&)clouds_color).mul(.5f*(float)atof(_GetItem(clouds,4,temp)));

//	on_device_create			();
	inherited::load				(m_manager, config);
}

void time::save				(CInifile& config)
{
	config.w_string				(m_identifier.c_str(),"ambient", m_ambient.c_str());
	config.w_fvector3			(m_identifier.c_str(),"ambient_color", ambient);
	config.w_string				(m_identifier.c_str(),"clouds_texture", clouds_texture_name.c_str());
	config.w_float				(m_identifier.c_str(),"far_plane", far_plane);
	config.w_float				(m_identifier.c_str(),"fog_distance", fog_distance);
	config.w_float				(m_identifier.c_str(),"fog_density", fog_density);
	config.w_fvector3			(m_identifier.c_str(),"fog_color", fog_color);
	config.w_fvector3			(m_identifier.c_str(),"rain_color", rain_color);
	config.w_float				(m_identifier.c_str(),"rain_density", rain_density);
	config.w_fvector3			(m_identifier.c_str(),"sky_color", sky_color);
	config.w_float				(m_identifier.c_str(),"sky_rotation", rad2deg(sky_rotation));
	config.w_string				(m_identifier.c_str(),"sky_texture", sky_texture_name.c_str());
	config.w_fvector3			(m_identifier.c_str(),"sun_color", sun_color);
	config.w_float				(m_identifier.c_str(),"sun_shafts_intensity", m_fSunShaftsIntensity);
	config.w_string				(m_identifier.c_str(),"sun", m_sun.c_str());
	config.w_string				(m_identifier.c_str(),"thunderbolt_collection", m_thunderbolt_collection.c_str());
	config.w_float				(m_identifier.c_str(),"thunderbolt_duration", bolt_duration);
	config.w_float				(m_identifier.c_str(),"thunderbolt_period", bolt_period);
	config.w_float				(m_identifier.c_str(),"water_intensity", m_fWaterIntensity);
	config.w_float				(m_identifier.c_str(),"wind_direction", rad2deg(wind_direction));
	config.w_float				(m_identifier.c_str(),"wind_velocity", wind_velocity);
	config.w_fvector4			(m_identifier.c_str(),"hemisphere_color", hemi_color);
	config.w_float				(m_identifier.c_str(),"sun_altitude", rad2deg(sun_dir.getH()));
	config.w_float				(m_identifier.c_str(),"sun_longitude", rad2deg(sun_dir.getP()));
	config.w_fvector4			(m_identifier.c_str(),"clouds_color", clouds_color);
}

LPCSTR time::id_getter		() const
{
	return						(m_identifier.c_str());
}

void time::id_setter		(LPCSTR value_)
{
	shared_str					value = value_;
	if (m_identifier._get() == value._get())
		return;

	if (m_weather)
		m_identifier			= m_weather->unique_id(m_identifier, value);
	else
		m_identifier			= value;
}

LPCSTR const* time::ambients_collection		()
{
	return				(&*m_manager.ambients().ambients_ids().begin());
}

u32 time::ambients_collection_size			()
{
	return				(m_manager.ambients().ambients_ids().size());
}

LPCSTR const* time::suns_collection			()
{
	return				(&*m_manager.suns().suns_ids().begin());
}

u32 time::suns_collection_size				()
{
	return				(m_manager.suns().suns_ids().size());
}

LPCSTR const* time::thunderbolts_collection	()
{
	return				(&*m_manager.thunderbolts().collections_ids().begin());
}

u32 time::thunderbolts_collection_size		()
{
	return				(m_manager.thunderbolts().collections_ids().size());
}

float time::sun_altitude_getter				() const
{
	float				y, x;
	sun_dir.getHP		(y, x);
	return				(rad2deg(y));
}

void time::sun_altitude_setter				(float value)
{
	float				y, x;
	sun_dir.getHP		(y, x);
	sun_dir.setHP		(deg2rad(value), x);
}

float time::sun_longitude_getter			() const
{
	float				y, x;
	sun_dir.getHP		(y, x);
	return				(rad2deg(x));
}

void time::sun_longitude_setter				(float value)
{
	float				y, x;
	sun_dir.getHP		(y, x);
	sun_dir.setHP		(y, deg2rad(value));
}

LPCSTR time::ambient_getter					() const
{
	return				(m_ambient.c_str());
}

void time::ambient_setter					(LPCSTR value)
{
	if (m_ambient._get() == shared_str(value)._get())
		return;

	m_ambient			= value;
	env_ambient			= m_manager.AppendEnvAmb(value);
}

LPCSTR time::sun_getter					() const
{
	return				(m_sun.c_str());
}

void time::sun_setter					(LPCSTR value)
{
	if (m_sun._get() == shared_str(value)._get())
		return;

	m_sun			= value;
	lens_flare_id		= m_manager.eff_LensFlare->AppendDef(m_manager, m_manager.m_suns_config, value);
}

LPCSTR time::thunderbolt_getter				() const
{
	return				(m_thunderbolt_collection.c_str());
}

void time::thunderbolt_setter				(LPCSTR value)
{
	if (m_thunderbolt_collection._get() == shared_str(value)._get())
		return;

	m_thunderbolt_collection	= value;
	tb_id				= m_manager.eff_Thunderbolt->AppendDef(m_manager, m_manager.m_thunderbolt_collections_config, m_manager.m_thunderbolts_config, value);
}

LPCSTR time::sky_texture_getter				() const
{
	return				(sky_texture_name.c_str());
}

void time::sky_texture_setter				(LPCSTR value)
{
	if (sky_texture_name._get() == shared_str(value)._get())
		return;

	sky_texture_name	= value;

	string_path			st_env;
	strconcat			(sizeof(st_env), st_env, value, "#small");
	sky_texture_env_name= st_env;
	m_pDescriptor->OnDeviceCreate(*this);
}

LPCSTR time::clouds_texture_getter			() const
{
	return				(clouds_texture_name.c_str());
}

void time::clouds_texture_setter			(LPCSTR value)
{
	if (clouds_texture_name._get() == shared_str(value)._get())
		return;

	clouds_texture_name	= value;
	m_pDescriptor->OnDeviceCreate(*this);
}

float time::sky_rotation_getter				() const
{
	return				(rad2deg(sky_rotation));
}

void time::sky_rotation_setter				(float value)
{
	sky_rotation		= deg2rad(value);
}

float time::wind_direction_getter			() const
{
	return				(rad2deg(wind_direction));
}

void time::wind_direction_setter			(float value)
{
	wind_direction		= deg2rad(value);
}

void time::fill				(editor::property_holder_collection* collection)
{
	VERIFY							(!m_property_holder);
	m_property_holder				= ::ide().create_property_holder(m_identifier.c_str(), collection, this);

	typedef editor::property_holder::string_getter_type	string_getter_type;
	string_getter_type				string_getter;
	string_getter.bind				(this, &time::id_getter);

	typedef editor::property_holder::string_setter_type	string_setter_type;
	string_setter_type				string_setter;
	string_setter.bind				(this, &time::id_setter);

	m_property_holder->add_property	(
		"id",
		"properties",
		"this option is resposible for time interval",
		m_identifier.c_str(),
		string_getter,
		string_setter
	);
	
	m_property_holder->add_property	(
		"color",
		"sun",
		"this option is resposible for sun color",
		(editor::color const&)sun_color,
		(editor::color&)sun_color
	);
	m_property_holder->add_property	(
		"shafts intensity",
		"sun",
		"this option is resposible for sun shafts intensity",
		m_fSunShaftsIntensity,
		m_fSunShaftsIntensity,
		0.f,
		1.f
	);

	typedef editor::property_holder::float_getter_type	float_getter_type;
	float_getter_type				sun_altitude_getter;
	sun_altitude_getter.bind		(this, &time::sun_altitude_getter);

	typedef editor::property_holder::float_setter_type	float_setter_type;
	float_setter_type				sun_altitude_setter;
	sun_altitude_setter.bind		(this, &time::sun_altitude_setter);

	m_property_holder->add_property	(
		"altitude",
		"sun",
		"this option is resposible for sun altitude (in degrees)",
		sun_altitude_getter(),
		sun_altitude_getter,
		sun_altitude_setter,
		-360.f,
		 360.f
	);

	float_getter_type				sun_longitude_getter;
	sun_longitude_getter.bind		(this, &time::sun_longitude_getter);

	float_setter_type				sun_longitude_setter;
	sun_longitude_setter.bind		(this, &time::sun_longitude_setter);

	m_property_holder->add_property	(
		"longitude",
		"sun",
		"this option is resposible for sun longitude (in degrees)",
		sun_longitude_getter(),
		sun_longitude_getter,
		sun_longitude_setter,
		-360.f,
		 360.f
	);

	typedef editor::property_holder::string_collection_getter_type	collection_getter_type;
	collection_getter_type			collection_getter;

	typedef editor::property_holder::string_collection_size_getter_type	collection_size_getter_type;
	collection_size_getter_type		collection_size_getter;

	collection_getter.bind			(this, &time::suns_collection);
	collection_size_getter.bind		(this, &time::suns_collection_size);
	m_property_holder->add_property	(
		"sun",
		"sun",
		"this option is resposible for ambient",
		m_sun.c_str(),
		m_sun,
		collection_getter,
		collection_size_getter,
		editor::property_holder::value_editor_combo_box,
		editor::property_holder::cannot_enter_text
	);

	string_getter_type		sky_texture_getter;
	sky_texture_getter.bind	(this, &time::sky_texture_getter);

	string_setter_type		sky_texture_setter;
	sky_texture_setter.bind	(this, &time::sky_texture_setter);

	m_property_holder->add_property	(
		"texture",
		"hemisphere",
		"this option is resposible for sky texture",
		sky_texture_name.c_str(),
		sky_texture_getter,
		sky_texture_setter,
		".dds",
		"Texture files (*.dds)|*.dds",
		detail::real_path("$game_textures$", "").c_str(),
		"Select texture...",
		editor::property_holder::cannot_enter_text,
		editor::property_holder::remove_extension
	);

	m_property_holder->add_property	(
		"sky color",
		"hemisphere",
		"this option is resposible for sky color",
		(editor::color const&)sky_color,
		(editor::color&)sky_color
	);

	m_property_holder->add_property	(
		"hemi color",
		"hemisphere",
		"this option is resposible for hemisphere color",
		(editor::color const&)hemi_color,
		(editor::color&)hemi_color
	);

	typedef ::editor::property_holder::float_getter_type	float_getter_type;
	float_getter_type				float_getter;

	typedef ::editor::property_holder::float_setter_type	float_setter_type;
	float_setter_type				float_setter;

	float_getter.bind				(this, &time::sky_rotation_getter);
	float_setter.bind				(this, &time::sky_rotation_setter);
	m_property_holder->add_property	(
		"sky rotation",
		"hemisphere",
		"this option is resposible for sky rotation",
		sky_rotation,
		float_getter,
		float_setter,
		-360.0f,
		 360.f
	);

	string_getter.bind				(this, &time::clouds_texture_getter);
	string_setter.bind				(this, &time::clouds_texture_setter);
	m_property_holder->add_property	(
		"texture",
		"clouds",
		"this option is resposible for clouds texture",
		clouds_texture_name.c_str(),
		string_getter,
		string_setter,
		".dds",
		"Texture files (*.dds)|*.dds",
		detail::real_path("$game_textures$", "").c_str(),
		"Select texture...",
		editor::property_holder::cannot_enter_text,
		editor::property_holder::remove_extension
	);

	m_property_holder->add_property	(
		"color",
		"clouds",
		"this option is resposible for clouds color",
		(editor::color const&)clouds_color,
		(editor::color&)clouds_color
	);

	m_property_holder->add_property	(
		"transparency",
		"clouds",
		"this option is resposible for clouds transparency",
		clouds_color.w,
		clouds_color.w,
		0.f,
		1.f
	);

	m_property_holder->add_property	(
		"color",
		"ambient",
		"this option is resposible for ambient color",
		(editor::color const&)ambient,
		(editor::color&)ambient
	);

	collection_getter.bind			(this, &time::ambients_collection);
	collection_size_getter.bind		(this, &time::ambients_collection_size);

	string_getter.bind				(this, &time::ambient_getter);
	string_setter.bind				(this, &time::ambient_setter);
	m_property_holder->add_property	(
		"ambient",
		"ambient",
		"this option is resposible for ambient",
		m_ambient.c_str(),
		string_getter,
		string_setter,
		collection_getter,
		collection_size_getter,
		editor::property_holder::value_editor_combo_box,
		editor::property_holder::cannot_enter_text
	);

	m_property_holder->add_property	(
		"color",
		"fog",
		"this option is resposible for fog density (0..1)",
		(editor::color const&)fog_color,
		(editor::color&)fog_color
	);
	m_property_holder->add_property	(
		"far plane",
		"fog",
		"this option is resposible for far plane",
		far_plane,
		far_plane
	);
	m_property_holder->add_property	(
		"distance",
		"fog",
		"this option is resposible for fog distance (shoudl be less than far plane)",
		fog_distance,
		fog_distance
	);
	m_property_holder->add_property	(
		"density",
		"fog",
		"this option is resposible for fog density (0..1)",
		fog_density,
		fog_density,
		0.f,
		1.f
	);
	m_property_holder->add_property	(
		"water intensity",
		"fog",
		"this option is resposible for water intensity (0..1)",
		m_fWaterIntensity,
		m_fWaterIntensity,
		0.f,
		1.f
	);

	m_property_holder->add_property	(
		"rain color",
		"rain",
		"this option is resposible for rain color",
		(editor::color const&)rain_color,
		(editor::color&)rain_color
	);
	m_property_holder->add_property	(
		"rain density",
		"rain",
		"this option is resposible for rain density (0..1)",
		rain_density,
		rain_density,
		0.f,
		1.f
	);

	collection_getter.bind			(this, &time::thunderbolts_collection);
	collection_size_getter.bind		(this, &time::thunderbolts_collection_size);
	m_property_holder->add_property	(
		"collection",
		"thunderbolts",
		"this option is resposible for ambient",
		m_thunderbolt_collection.c_str(),
		m_thunderbolt_collection,
		collection_getter,
		collection_size_getter,
		editor::property_holder::value_editor_combo_box,
		editor::property_holder::cannot_enter_text
	);

	m_property_holder->add_property	(
		"duration",
		"thunderbolts",
		"this option is resposible for thunderbolt duration",
		bolt_duration,
		bolt_duration
	);
	m_property_holder->add_property	(
		"period",
		"thunderbolts",
		"this option is resposible for thunderbolt period",
		bolt_period,
		bolt_period
	);
	
	float_getter.bind				(this, &time::wind_direction_getter);
	float_setter.bind				(this, &time::wind_direction_setter);
	m_property_holder->add_property	(
		"direction",
		"wind",
		"this option is resposible for wind direction (in degrees)",
		wind_direction,
		float_getter,
		float_setter,
		-360.f,
		 360.f
	);
	m_property_holder->add_property	(
		"velocity",
		"wind",
		"this option is resposible for wind velocity (meters per second)",
		wind_velocity,
		wind_velocity,
		0.f,
		1000.f
	);
}

void time::lerp	(CEnvironment* parent, CEnvDescriptor& A, CEnvDescriptor& B, float f, CEnvModifier& M, float m_power)
{
	float					start_time = m_manager.Current[0]->exec_time;
	float					stop_time = m_manager.Current[1]->exec_time;
	float					current_time = m_manager.GetGameTime();
	if (start_time >= stop_time) {
		if (current_time >= start_time)
			clamp			(current_time, start_time, 24.f*60.f*60.f);
		else
			clamp			(current_time, 0.f, stop_time);

		if (current_time <= stop_time)
			current_time	+= 24.f*60.f*60.f;

		stop_time			+= 24.f*60.f*60.f;
	}
	else
		clamp				(current_time, start_time, stop_time);

	VERIFY					(start_time < stop_time);
	
	u32						current_time_u32 = iFloor(current_time);
	current_time_u32		= current_time_u32%(24*60*60);

	u32						hours = current_time_u32/(60*60);
	current_time_u32		%= (60*60);

	u32						minutes = current_time_u32/60;
	u32						seconds = current_time_u32%60;

	string16				temp;
	xr_sprintf				(temp, "%02d:%02d:%02d", hours, minutes, seconds);
	m_identifier			= temp;

	time&					a = static_cast<time&>(A);
	m_ambient				= a.m_ambient;
	clouds_texture_name		= a.clouds_texture_name;
	sky_texture_name		= a.sky_texture_name;
	m_sun					= a.m_sun;
	m_thunderbolt_collection= a.m_thunderbolt_collection;

	inherited::lerp			(parent, A, B, f, M, m_power);
}

#endif // #ifdef INGAME_EDITOR