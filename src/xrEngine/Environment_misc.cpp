#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#include "xr_efflensflare.h"
#include "thunderbolt.h"
#include "Rain.h"

#include "IGame_Level.h"
#include "Common/object_broker.h"
#include "Common/LevelGameDef.h"

ENGINE_API float SunshaftsIntensity = 0.f;
extern Fvector3 ssfx_wetness_multiplier;

void CEnvModifier::load(IReader* fs, u32 version)
{
    use_flags.one();
    fs->r_fvector3(position);
    radius = fs->r_float();
    power = fs->r_float();
    far_plane = fs->r_float();
    fs->r_fvector3(fog_color);
    fog_density = fs->r_float();
    fs->r_fvector3(ambient);
    fs->r_fvector3(sky_color);
    fs->r_fvector3(hemi_color);

    if (version >= 0x0016)
    {
        use_flags.assign(fs->r_u16());
    }
}

float CEnvModifier::sum(CEnvModifier& M, Fvector3& view)
{
    float _dist_sq = view.distance_to_sqr(M.position);
    if (_dist_sq >= (M.radius * M.radius))
        return 0;

    float _att = 1 - _sqrt(_dist_sq) / M.radius; //[0..1];
    float _power = M.power * _att;

    if (M.use_flags.test(eViewDist))
    {
        far_plane += M.far_plane * _power;
        use_flags.set(eViewDist, true);
    }
    if (M.use_flags.test(eFogColor))
    {
        fog_color.mad(M.fog_color, _power);
        use_flags.set(eFogColor, true);
    }
    if (M.use_flags.test(eFogDensity))
    {
        fog_density += M.fog_density * _power;
        use_flags.set(eFogDensity, true);
    }

    if (M.use_flags.test(eAmbientColor))
    {
        ambient.mad(M.ambient, _power);
        use_flags.set(eAmbientColor, true);
    }

    if (M.use_flags.test(eSkyColor))
    {
        sky_color.mad(M.sky_color, _power);
        use_flags.set(eSkyColor, true);
    }

    if (M.use_flags.test(eHemiColor))
    {
        hemi_color.mad(M.hemi_color, _power);
        use_flags.set(eHemiColor, true);
    }

    return _power;
}

//-----------------------------------------------------------------------------
// Environment ambient
//-----------------------------------------------------------------------------
void CEnvAmbient::SSndChannel::load(const CInifile& config, pcstr sect, pcstr sectionToReadFrom)
{
    m_load_section = sectionToReadFrom ? sectionToReadFrom : sect;

    if (config.read_if_exists(m_sound_dist, m_load_section, "sound_dist"))
    {
        if (m_sound_dist.x > m_sound_dist.y)
            std::swap(m_sound_dist.x, m_sound_dist.y);
        config.read_if_exists(m_sound_dist.x, m_load_section, "min_distance");
        config.read_if_exists(m_sound_dist.y, m_load_section, "max_distance");
    }
    else
    {
        m_sound_dist.x = config.r_float(m_load_section, "min_distance");
        m_sound_dist.y = config.r_float(m_load_section, "max_distance");
        R_ASSERT2(m_sound_dist.y > m_sound_dist.x, sect);
    }

    Ivector2 staticPeriod;
    Ivector4 period;
    if (config.try_read_if_exists(period, m_load_section, "sound_period")) // Pre Clear Sky
    {
        config.read_if_exists(period.x, m_load_section, "period0");
        config.read_if_exists(period.y, m_load_section, "period1");
        config.read_if_exists(period.z, m_load_section, "period2");
        config.read_if_exists(period.w, m_load_section, "period3");
        m_sound_period.set(period.mul(1000));
    }
    else if (config.read_if_exists(staticPeriod, m_load_section, "sound_period")) // SOC
    {
        period = { staticPeriod.x, staticPeriod.y, staticPeriod.x, staticPeriod.y };
        config.read_if_exists(period.x, m_load_section, "period0");
        config.read_if_exists(period.y, m_load_section, "period1");
        config.read_if_exists(period.z, m_load_section, "period2");
        config.read_if_exists(period.w, m_load_section, "period3");
        m_sound_period.set(period.mul(1000));
    }
    else // COP
    {
        m_sound_period.x = config.r_s32(m_load_section, "period0");
        m_sound_period.y = config.r_s32(m_load_section, "period1");
        m_sound_period.z = config.r_s32(m_load_section, "period2");
        m_sound_period.w = config.r_s32(m_load_section, "period3");
    }

    R_ASSERT(m_sound_period.x <= m_sound_period.y && m_sound_period.z <= m_sound_period.w);

    pcstr snds = config.r_string(m_load_section, "sounds");
    const int cnt = _GetItemCount(snds);
    string_path tmp;
    R_ASSERT3(cnt, "sounds empty", m_load_section.c_str());

    m_sounds.resize(cnt);

    for (int k = 0; k < cnt; ++k)
    {
        _GetItem(snds, k, tmp);
        m_sounds[k].create(tmp, st_Effect, sg_SourceType);
    }
}

CEnvAmbient::SEffect* CEnvAmbient::create_effect(const CInifile& config, pcstr id)
{
    SEffect* result = xr_new<SEffect>();
    result->life_time = iFloor(config.r_float(id, "life_time") * 1000.f);
    result->particles = config.r_string(id, "particles");
    VERIFY(result->particles.size());
    result->offset = config.r_fvector3(id, "offset");
    result->wind_gust_factor = config.r_float(id, "wind_gust_factor");

    if (config.line_exist(id, "sound"))
        result->sound.create(config.r_string(id, "sound"), st_Effect, sg_SourceType);

    if (config.line_exist(id, "wind_blast_strength"))
    {
        result->wind_blast_strength = config.r_float(id, "wind_blast_strength");
        result->wind_blast_direction.setHP(deg2rad(config.r_float(id, "wind_blast_longitude")), 0.f);
        result->wind_blast_in_time = config.r_float(id, "wind_blast_in_time");
        result->wind_blast_out_time = config.r_float(id, "wind_blast_out_time");
        return (result);
    }

    result->wind_blast_strength = 0.f;
    result->wind_blast_direction.set(0.f, 0.f, 1.f);
    result->wind_blast_in_time = 0.f;
    result->wind_blast_out_time = 0.f;

    return (result);
}

CEnvAmbient::SSndChannel* CEnvAmbient::create_sound_channel(const CInifile& config, pcstr id, pcstr sectionToReadFrom)
{
    SSndChannel* result = xr_new<SSndChannel>();
    result->load(config, id, sectionToReadFrom);
    return (result);
}

CEnvAmbient::~CEnvAmbient() { destroy(); }
void CEnvAmbient::destroy()
{
    delete_data(m_effects);
    delete_data(m_sound_channels);
}

void CEnvAmbient::load(
    const CInifile& ambients_config, const CInifile& sound_channels_config, const CInifile& effects_config, const shared_str& sect)
{
    m_ambients_config_filename = ambients_config.fname();
    m_load_section = sect;
    string_path tmp;

    // sounds
    pcstr channels = nullptr;
    bool overrideReadingSection = false;
    if (ambients_config.line_exist(sect, "sounds")) // SOC
    {
        channels = ambients_config.r_string(sect, "sounds");
        overrideReadingSection = true;
    }
    if (ambients_config.line_exist(sect, "snd_channels")) // Pre Clear Sky
    {
        channels = ambients_config.r_string(sect, "snd_channels");
        overrideReadingSection = false;
    }
    if (ambients_config.line_exist(sect, "sound_channels")) // COP, highest priority
    {
        channels = ambients_config.r_string(sect, "sound_channels");
        overrideReadingSection = false;
    }

    // SOC has no separate sound channels in env ambient section
    // Env ambient IS the sound channel in SOC
    size_t cnt = overrideReadingSection ? 1 : _GetItemCount(channels);
    m_sound_channels.resize(cnt);

    for (size_t i = 0; i < cnt; ++i)
        m_sound_channels[i] = create_sound_channel(sound_channels_config, _GetItem(channels, i, tmp),
            overrideReadingSection ? m_load_section.c_str() : nullptr);

    // effects
    Fvector2 period;
    if (ambients_config.read_if_exists(period, sect, "effect_period"))
    {
        ambients_config.read_if_exists(period.x, sect, "min_effect_period");
        ambients_config.read_if_exists(period.y, sect, "max_effect_period");
    }
    else
    {
        period.x = ambients_config.r_float(sect, "min_effect_period");
        period.y = ambients_config.r_float(sect, "max_effect_period");
    }
    m_effect_period.set(iFloor(period.x * 1000.f), iFloor(period.y * 1000.f));

    if (ambients_config.line_exist(sect, "effects"))
    {
        pcstr effs = ambients_config.r_string(sect, "effects");
        cnt = _GetItemCount(effs);

        m_effects.resize(cnt);
        for (size_t k = 0; k < cnt; ++k)
            m_effects[k] = create_effect(effects_config, _GetItem(effs, k, tmp));
    }

    R_ASSERT(!m_sound_channels.empty() || !m_effects.empty());
}

//-----------------------------------------------------------------------------
// Environment descriptor
//-----------------------------------------------------------------------------
CEnvDescriptor::CEnvDescriptor(shared_str const& identifier) : m_identifier(identifier)
{
    dont_save = false;

    exec_time = 0.0f;
    exec_time_loaded = 0.0f;

    clouds_color.set(1, 1, 1, 1);
    sky_color.set(1, 1, 1);
    sky_rotation = 0.0f;

    far_plane = 400.0f;
    ;

    fog_color.set(1, 1, 1);
    fog_density = 0.0f;
    fog_distance = 400.0f;

    rain_density = 0.0f;
    rain_color.set(0, 0, 0);

    bolt_period = 0.0f;
    bolt_duration = 0.0f;

    wind_velocity = 0.0f;
    wind_direction = 0.0f;

    ambient.set(0, 0, 0);
    hemi_color.set(1, 1, 1, 1);
    sun_color.set(1, 1, 1);
    sun_dir.set(0, -1, 0);
    use_dynamic_sun_dir = true;

    m_fSunShaftsIntensity = 0;
    m_fWaterIntensity = 1;

    m_fTreeAmplitudeIntensity = 0.01f;

    lens_flare = nullptr;
    thunderbolt = nullptr;

    env_ambient = nullptr;
}

#define C_CHECK(C)                                                           \
    if (C.x < 0 || C.x > 2 || C.y < 0 || C.y > 2 || C.z < 0 || C.z > 2)      \
    {                                                                        \
        Msg("! Invalid '%s' in env-section '%s'", #C, identifier); \
    }

void CEnvDescriptor::load(CEnvironment& environment, const CInifile& config, pcstr section /*= nullptr*/)
{
    const bool old_style               = section;

    cpcstr ambient_name                = old_style ? "env_ambient"   : "ambient";
    cpcstr ambient_color_name          = old_style ? "ambient"       : "ambient_color";
    cpcstr sun_name                    = old_style ? "flares"        : "sun";
    cpcstr thunderbolt_collection_name = old_style ? "thunderbolt"   : "thunderbolt_collection";
    cpcstr thunderbolt_duration_name   = old_style ? "bolt_duration" : "thunderbolt_duration";
    cpcstr thunderbolt_period_name     = old_style ? "bolt_period"   : "thunderbolt_period";

    cpcstr identifier = section ? section : m_identifier.c_str();

    Ivector3 tm = {0, 0, 0};
    const int result = sscanf(m_identifier.c_str(), "%d:%d:%d", &tm.x, &tm.y, &tm.z);
    R_ASSERT3(result == 3 && (tm.x >= 0) && (tm.x < 24) && (tm.y >= 0) && (tm.y < 60) && (tm.z >= 0) && (tm.z < 60),
        "Incorrect weather time", m_identifier.c_str());
    exec_time = tm.x * 3600.f + tm.y * 60.f + tm.z;
    exec_time_loaded = exec_time;

    string_path skyTexture, skyTextureEnv;
    xr_strcpy(skyTexture, config.r_string(identifier, "sky_texture"));
    strconcat(sizeof(skyTextureEnv), skyTextureEnv, skyTexture, "#small");
    sky_texture_name = skyTexture;
    sky_texture_env_name = skyTextureEnv;

    clouds_texture_name = config.r_string(identifier, "clouds_texture");

    pcstr cloudsColor = config.r_string(identifier, "clouds_color");

    float multiplier = 0, save = 0;
    sscanf(cloudsColor, "%f,%f,%f,%f,%f", &clouds_color.x, &clouds_color.y, &clouds_color.z, &clouds_color.w, &multiplier);

    save = clouds_color.w;
    clouds_color.mul(.5f * multiplier);
    clouds_color.w = save;

    sky_color = config.r_fvector3(identifier, "sky_color");
    if (old_style)
        sky_color.mul(0.5f);

    if (config.line_exist(identifier, "sky_rotation"))
        sky_rotation = deg2rad(config.r_float(identifier, "sky_rotation"));
    else
        sky_rotation = 0.0f;

    if (config.line_exist(identifier, "clouds_rotation"))
        clouds_rotation = deg2rad(config.r_float(identifier, "clouds_rotation"));
    else
        clouds_rotation = sky_rotation;

    far_plane = config.r_float(identifier, "far_plane");
    fog_color = config.r_fvector3(identifier, "fog_color");
    fog_density = config.r_float(identifier, "fog_density");
    fog_distance = config.r_float(identifier, "fog_distance");
    rain_density = config.r_float(identifier, "rain_density");
    clamp(rain_density, 0.f, 1.f);
    rain_color = config.r_fvector3(identifier, "rain_color");
    wind_velocity = config.r_float(identifier, "wind_velocity");
    wind_direction = deg2rad(config.r_float(identifier, "wind_direction"));

    config.read_if_exists(hemi_color, identifier, "hemisphere_color", "hemi_color", true);

    sun_color = config.r_fvector3(identifier, "sun_color");

    ambient = config.r_fvector3(identifier, ambient_color_name);
    if (config.line_exist(identifier, ambient_name))
    {
        CInifile const* pIni = &config == pSettings ? pSettings : nullptr;
        env_ambient = environment.AppendEnvAmb(config.r_string(identifier, ambient_name), pIni);
    }

    Fvector2 sunVec{};

    if (config.read_if_exists(sunVec, identifier, "sun_dir"))
        use_dynamic_sun_dir = false;
    else
    {
        sunVec.y = config.r_float(identifier, "sun_altitude");
        sunVec.x = config.r_float(identifier, "sun_longitude");
    }
    sun_dir.setHP(deg2rad(sunVec.y), deg2rad(sunVec.x));

    R_ASSERT(_valid(sun_dir));

    float degrees;
    if (!config.read_if_exists(degrees, identifier, "sun_azimuth"))
        degrees = pSettingsOpenXRay->read_if_exists<float>("environment", "sun_dir_azimuth", 0.0f);

    clamp(degrees, 0.0f, 360.0f);
    sun_azimuth = deg2rad(degrees);

    lens_flare = environment.eff_LensFlare->AppendDef(config.r_string(identifier, sun_name));
    thunderbolt = environment.eff_Thunderbolt->AppendDef(config.r_string(identifier, thunderbolt_collection_name));

    if (thunderbolt)
    {
        bolt_period = config.r_float(identifier, thunderbolt_period_name);
        bolt_duration = config.r_float(identifier, thunderbolt_duration_name);
    }

    m_fSunShaftsIntensity = config.read_if_exists<float>(identifier, "sun_shafts_intensity", 0.0);
    m_fWaterIntensity = config.read_if_exists<float>(identifier, "water_intensity", 1.0);
    m_fTreeAmplitudeIntensity = config.read_if_exists<float>(identifier, "tree_amplitude_intensity", 0.01);

    C_CHECK(clouds_color);
    C_CHECK(sky_color);
    C_CHECK(fog_color);
    C_CHECK(rain_color);
    C_CHECK(ambient);
    C_CHECK(hemi_color);
    C_CHECK(sun_color);
    on_device_create();
}

#undef C_CHECK

void CEnvDescriptor::save(CInifile& config, pcstr section /*= nullptr*/) const
{
    if (dont_save)
        return;

    const bool old_style               = section;

    cpcstr ambient_name                = old_style ? "env_ambient"   : "ambient";
    cpcstr ambient_color_name          = old_style ? "ambient"       : "ambient_color";
    cpcstr hemisphere_color            = old_style ? "hemi_color"    : "hemisphere_color";
    cpcstr sun_name                    = old_style ? "flares"        : "sun";
    cpcstr thunderbolt_collection_name = old_style ? "thunderbolt"   : "thunderbolt_collection";
    cpcstr thunderbolt_duration_name   = old_style ? "bolt_duration" : "thunderbolt_duration";
    cpcstr thunderbolt_period_name     = old_style ? "bolt_period"   : "thunderbolt_period";

    cpcstr identifier = section ? section : m_identifier.c_str();

    if (env_ambient)
        config.w_string(identifier, ambient_name,               env_ambient->name().c_str());
    config.w_fvector3 (identifier, ambient_color_name,          ambient);

    config.w_fvector4 (identifier, "clouds_color",              clouds_color);
    config.w_string   (identifier, "clouds_texture",            clouds_texture_name.c_str());
    config.w_float    (identifier, "clouds_rotation",           rad2deg(clouds_rotation));

    config.w_float    (identifier, "far_plane",                 far_plane);

    config.w_fvector3 (identifier, "fog_color",                 fog_color);
    config.w_float    (identifier, "fog_density",               fog_density);
    config.w_float    (identifier, "fog_distance",              fog_distance);

    config.w_fvector4 (identifier, hemisphere_color,            hemi_color);

    config.w_fvector3 (identifier, "rain_color",                rain_color);
    config.w_float    (identifier, "rain_density",              rain_density);

    config.w_fvector3 (identifier, "sky_color",                 sky_color);
    config.w_float    (identifier, "sky_rotation",              rad2deg(sky_rotation));
    config.w_string   (identifier, "sky_texture",               sky_texture_name.c_str());

    config.w_string   (identifier, sun_name,                    lens_flare ? lens_flare->section.c_str() : "");
    config.w_fvector3 (identifier, "sun_color",                 sun_color);
    if (use_dynamic_sun_dir)
    {
        float altutude, longitude;
        sun_dir.getHP(altutude, longitude);
        config.w_fvector2(identifier, "sun_dir", { rad2deg(longitude), rad2deg(altutude) });
    }
    else
    {
        config.w_float(identifier, "sun_altitude",              rad2deg(sun_dir.getH()));
        config.w_float(identifier, "sun_longitude",             rad2deg(sun_dir.getP()));
    }
    config.w_float    (identifier, "sun_azimuth",               sun_azimuth);
    config.w_float    (identifier, "sun_shafts_intensity",      m_fSunShaftsIntensity);

    config.w_string   (identifier, thunderbolt_collection_name, thunderbolt ? thunderbolt->section.c_str() : "");
    config.w_float    (identifier, thunderbolt_duration_name,   bolt_duration);
    config.w_float    (identifier, thunderbolt_period_name,     bolt_period);

    config.w_float    (identifier, "water_intensity",           m_fWaterIntensity);

    config.w_float    (identifier, "wind_direction",            rad2deg(wind_direction));
    config.w_float    (identifier, "wind_velocity",             wind_velocity);
}

void CEnvDescriptor::on_device_create()
{
    m_pDescriptor->OnDeviceCreate(*this);
}

void CEnvDescriptor::on_device_destroy()
{
    m_pDescriptor->OnDeviceDestroy();
}

//-----------------------------------------------------------------------------
// Environment Mixer
//-----------------------------------------------------------------------------
CEnvDescriptorMixer::CEnvDescriptorMixer()
    : CEnvDescriptor("00:00:00"), soc_style(false)
{
    use_dynamic_sun_dir = pSettingsOpenXRay->read_if_exists<bool>("environment", "dynamic_sun_dir", true);
}

void CEnvDescriptorMixer::lerp(CEnvironment& parent, CEnvDescriptor& A, CEnvDescriptor& B,
    float f, CEnvModifier& Mdf, float modifier_power)
{
    const float fi = 1 - f;

    exec_time = fi * A.exec_time + f * B.exec_time;

    weight = f;
    modif_power = 1.f / (modifier_power + 1); // the environment itself

    clouds_color.lerp(A.clouds_color, B.clouds_color, f);

    sky_rotation = (fi * A.sky_rotation + f * B.sky_rotation);
    clouds_rotation = (fi * A.clouds_rotation + f * B.clouds_rotation);

    //. far_plane = (fi*A.far_plane + f*B.far_plane + Mdf.far_plane)*psVisDistance*modif_power;
    if (Mdf.use_flags.test(eViewDist))
        far_plane = (fi * A.far_plane + f * B.far_plane + Mdf.far_plane) * psVisDistance * modif_power;
    else
        far_plane = (fi * A.far_plane + f * B.far_plane) * psVisDistance;

    //. fog_color.lerp (A.fog_color,B.fog_color,f).add(Mdf.fog_color).mul(modif_power);
    fog_color.lerp(A.fog_color, B.fog_color, f);
    if (Mdf.use_flags.test(eFogColor))
        fog_color.add(Mdf.fog_color).mul(modif_power);

    //. fog_density = (fi*A.fog_density + f*B.fog_density + Mdf.fog_density)*modif_power;
    fog_density = (fi * A.fog_density + f * B.fog_density);
    if (Mdf.use_flags.test(eFogDensity))
    {
        fog_density += Mdf.fog_density;
        fog_density *= modif_power;
    }

    fog_distance = (fi * A.fog_distance + f * B.fog_distance);
    fog_near = (1.0f - fog_density) * 0.85f * fog_distance;
    fog_far = 0.99f * fog_distance;

    rain_density = fi * A.rain_density + f * B.rain_density;
    rain_color.lerp(A.rain_color, B.rain_color, f);
    bolt_period = fi * A.bolt_period + f * B.bolt_period;
    bolt_duration = fi * A.bolt_duration + f * B.bolt_duration;
    // wind
    wind_velocity = fi * A.wind_velocity + f * B.wind_velocity;
    wind_direction = fi * A.wind_direction + f * B.wind_direction;

#ifdef DEBUG
    if (SunshaftsIntensity > 0.f)
        m_fSunShaftsIntensity = SunshaftsIntensity;
    else
#endif
    {
        m_fSunShaftsIntensity = fi * A.m_fSunShaftsIntensity + f * B.m_fSunShaftsIntensity;
    }

    m_fWaterIntensity = fi * A.m_fWaterIntensity + f * B.m_fWaterIntensity;

    m_fTreeAmplitudeIntensity = fi * A.m_fTreeAmplitudeIntensity + f * B.m_fTreeAmplitudeIntensity;

    // colors
    //. sky_color.lerp (A.sky_color,B.sky_color,f).add(Mdf.sky_color).mul(modif_power);
    sky_color.lerp(A.sky_color, B.sky_color, f);
    if (Mdf.use_flags.test(eSkyColor))
        sky_color.add(Mdf.sky_color).mul(modif_power);

    //. ambient.lerp (A.ambient,B.ambient,f).add(Mdf.ambient).mul(modif_power);
    ambient.lerp(A.ambient, B.ambient, f);
    if (Mdf.use_flags.test(eAmbientColor))
        ambient.add(Mdf.ambient).mul(modif_power);

    hemi_color.lerp(A.hemi_color, B.hemi_color, f);

    if (Mdf.use_flags.test(eHemiColor))
    {
        hemi_color.x += Mdf.hemi_color.x;
        hemi_color.y += Mdf.hemi_color.y;
        hemi_color.z += Mdf.hemi_color.z;
        hemi_color.x *= modif_power;
        hemi_color.y *= modif_power;
        hemi_color.z *= modif_power;
    }

    sun_color.lerp(A.sun_color, B.sun_color, f);

    if (rain_density > 0.f)
        parent.wetness_factor += (rain_density * ssfx_wetness_multiplier.x) / 10000.f;
    else
        parent.wetness_factor -= 0.0001f * ssfx_wetness_multiplier.y;

    clamp(parent.wetness_factor, 0.f, 1.f);

    sun_azimuth = (fi * A.sun_azimuth + f * B.sun_azimuth);

     // Igor. Dynamic sun position.
    if (!GEnv.Render->is_sun_static() && use_dynamic_sun_dir)
    {
        auto [dir, blend] = calculate_dynamic_sun_dir(exec_time, sun_azimuth);
        sun_dir = dir;
        sun_color.mul(blend);
    }
    else
    {
        R_ASSERT(_valid(A.sun_dir));
        R_ASSERT(_valid(B.sun_dir));
        sun_dir.lerp(A.sun_dir, B.sun_dir, f).normalize();
        R_ASSERT(_valid(sun_dir));
    }
    VERIFY2(sun_dir.y < 0, "Invalid sun direction settings while lerp");

    lens_flare = f < 0.5f ? A.lens_flare : B.lens_flare;
    thunderbolt = f < 0.5f ? A.thunderbolt : B.thunderbolt;
    env_ambient = Random.randF() < 1.f - f ? A.env_ambient : B.env_ambient;

    if (soc_style)
    {
        env_color =
        {
            sky_color.x * 2 + EPS, sky_color.y * 2 + EPS,
            sky_color.z * 2 + EPS, weight
        };
    }
    else
    {
        env_color =
        {
            hemi_color.x * 2 + EPS, hemi_color.y * 2 + EPS,
            hemi_color.z * 2 + EPS, weight
        };
    }

    string_path temp_name;
    sky_texture_name = strconcat(temp_name, A.sky_texture_name.c_str(), "; ", B.sky_texture_name.c_str());
    clouds_texture_name = strconcat(temp_name, A.clouds_texture_name.c_str(), "; ", B.clouds_texture_name.c_str());
}

std::pair<Fvector3, float> CEnvDescriptorMixer::calculate_dynamic_sun_dir(float fGameTime, float azimuth)
{
    float g = (360.0f / 365.25f) * (180.0f + fGameTime / DAY_LENGTH);

    g = deg2rad(g);

    // Declination
    float D = 0.396372f - 22.91327f * _cos(g) + 4.02543f * _sin(g) - 0.387205f * _cos(2 * g) + 0.051967f * _sin(2 * g) -
        0.154527f * _cos(3 * g) + 0.084798f * _sin(3 * g);

    // Now calculate the time correction for solar angle:
    float TC =
        0.004297f + 0.107029f * _cos(g) - 1.837877f * _sin(g) - 0.837378f * _cos(2 * g) - 2.340475f * _sin(2 * g);

    // IN degrees
    float Longitude = -30.4f;

    float SHA = (fGameTime / (DAY_LENGTH / 24) - 12) * 15 + Longitude + TC;

    // Need this to correctly determine SHA sign
    if (SHA > 180)
        SHA -= 360;
    if (SHA < -180)
        SHA += 360;

    // IN degrees
    float const Latitude = 50.27f;
    float const LatitudeR = deg2rad(Latitude);

    // Now we can calculate the Sun Zenith Angle (SZA):
    float cosSZA = _sin(LatitudeR) * _sin(deg2rad(D)) + _cos(LatitudeR) * _cos(deg2rad(D)) * _cos(deg2rad(SHA));

    clamp(cosSZA, -1.0f, 1.0f);

    float SZA = acosf(cosSZA);
    float SEA = PI / 2 - SZA;

    // To finish we will calculate the Azimuth Angle (AZ):
    float cosAZ = 0.f;
    float const sin_SZA = _sin(SZA);
    float const cos_Latitude = _cos(LatitudeR);
    float const sin_SZA_X_cos_Latitude = sin_SZA * cos_Latitude;
    if (!fis_zero(sin_SZA_X_cos_Latitude))
        cosAZ = (_sin(deg2rad(D)) - _sin(LatitudeR) * _cos(SZA)) / sin_SZA_X_cos_Latitude;

    clamp(cosAZ, -1.0f, 1.0f);
    float AZ = acosf(cosAZ) + azimuth;

    const Fvector2 minAngle = Fvector2().set(deg2rad(1.0f), deg2rad(3.0f));

    if (SEA < minAngle.x)
        SEA = minAngle.x;

    float fSunBlend = (SEA - minAngle.x) / (minAngle.y - minAngle.x);
    clamp(fSunBlend, 0.0f, 1.0f);

    SEA = -SEA;

    if (SHA < 0)
        AZ = 2 * PI - AZ;

    R_ASSERT(_valid(AZ));
    R_ASSERT(_valid(SEA));

    Fvector3 result;
    result.setHP(AZ, SEA);
    R_ASSERT(_valid(result));

    return
    {
        result,
        fSunBlend
    };
}

//-----------------------------------------------------------------------------
// Environment IO
//-----------------------------------------------------------------------------
CEnvAmbient* CEnvironment::AppendEnvAmb(const shared_str& sect, CInifile const* pIni /*= nullptr*/)
{
    for (const auto& ambient : Ambients)
        if (ambient->name().equal(sect))
            return ambient;

    CEnvAmbient* ambient = Ambients.emplace_back(xr_new<CEnvAmbient>());
    ambient->load(pIni ? *pIni : *m_ambients_config,
        pIni ? *pIni : *m_sound_channels_config,
        pIni ? *pIni : *m_effects_config, sect);
    return ambient;
}

void CEnvironment::mods_load()
{
    Modifiers.clear();
    string_path path;
    if (FS.exist(path, "$level$", "level.env_mod"))
    {
        IReader* fs = FS.r_open(path);
        u32 id = 0;
        u32 ver = 0x0015;
        u32 sz;

        while (0 != (sz = fs->find_chunk(id)))
        {
            if (id == 0 && sz == sizeof(u32))
            {
                ver = fs->r_u32();
            }
            else
            {
                Modifiers.emplace_back(CEnvModifier());
                Modifiers.back().load(fs, ver);
            }
            id++;
        }
        FS.r_close(fs);
    }

    load_level_specific_ambients();
}

void CEnvironment::mods_unload() { Modifiers.clear(); }
void CEnvironment::load_level_specific_ambients()
{
    const shared_str level_name = g_pGameLevel->name();

    string_path path;
    strconcat(sizeof(path), path, "environment" DELIMITER "ambients" DELIMITER, level_name.c_str(), ".ltx");

    string_path full_path;
    CInifile* level_ambients = xr_new<CInifile>(FS.update_path(full_path, "$game_config$", path), true, true, false);

    if (level_ambients->section_count() == 0)
    {
        xr_delete(level_ambients);
        return;
    }

    for (const auto& ambient : Ambients)
    {
        shared_str section_name = ambient->name();

        CInifile const* sounds = m_sound_channels_config;
        CInifile const* effects = m_effects_config;

        // choose a source ini file
        CInifile const* source = nullptr;
        if (level_ambients && level_ambients->section_exist(section_name))
            source = level_ambients;
        else if (m_ambients_config && m_ambients_config->section_exist(section_name))
            source = m_ambients_config;
        else
        {
            source = pSettings;
            sounds = pSettings;
            effects = pSettings;
        }

        // check and reload if needed
        if (xr_strcmp(ambient->get_ambients_config_filename().c_str(), source->fname()))
        {
            ambient->destroy();
            ambient->load(*source, *sounds, *effects, section_name);
        }
    }

    xr_delete(level_ambients);
}

CEnvDescriptor* CEnvironment::create_descriptor(shared_str const& identifier,
    CInifile const* config, pcstr section /*= nullptr*/)
{
    CEnvDescriptor* result = xr_new<CEnvDescriptor>(identifier);
    if (config)
        result->load(*this, *config, section);
    return result;
}

void CEnvironment::load_weathers()
{
    if (!WeatherCycles.empty())
        return;

    FS_FileSet weathers;
    FS.file_list(weathers, "$game_weathers$", FS_ListFiles, "*.ltx");

    // CoP style weather config
    xr_string id;
    for (const auto& file : weathers)
    {
        pcstr fileName = file.name.c_str();
        const size_t length = xr_strlen(fileName);
        id.assign(fileName, length - 4);
        EnvVec& env = WeatherCycles[id.c_str()];

        string_path file_path;
        FS.update_path(file_path, "$game_weathers$", fileName);
        CInifile* config = CInifile::Create(file_path);

        auto& sections = config->sections();
        env.reserve(sections.size());

        for (const auto& section : sections)
            env.emplace_back(create_descriptor(section->Name, config));

        CInifile::Destroy(config);
    }

    // ShoC style weather config
    u32 weatherCount = 0;
    if (pSettings->section_exist("weathers"))
    {
        weatherCount = pSettings->line_count("weathers");
        Log("~ ShoC style weather config detected");
    }

    for (u32 weatherIdx = 0; weatherIdx < weatherCount; ++weatherIdx)
    {
        pcstr weatherName, weatherSection;
        if (pSettings->r_line("weathers", weatherIdx, &weatherName, &weatherSection))
        {
            const u32 envCount = pSettings->line_count(weatherSection);

            EnvVec& env = WeatherCycles[weatherName];
            env.reserve(envCount);
            env.soc_style = true;

            pcstr executionTime, envSection;
            for (u32 envIdx = 0; envIdx < envCount; ++envIdx)
            {
                if (pSettings->r_line(weatherSection, envIdx, &executionTime, &envSection))
                {
                    env.emplace_back(create_descriptor(executionTime, pSettings, envSection));
                }
            }
        }
    }

    R_ASSERT2(!WeatherCycles.empty(), "Empty weathers.");

    // sorting weather envs
    for (auto& cycle : WeatherCycles)
    {
        R_ASSERT3(cycle.second.size() > 1, "Environment in weather must >=2", cycle.first.c_str());
        std::sort(cycle.second.begin(), cycle.second.end(), sort_env_etl_pred);
    }

    SetWeather((*WeatherCycles.begin()).first.c_str());
}

void CEnvironment::load_weather_effects()
{
    if (!WeatherFXs.empty())
        return;

    FS_FileSet weathersEffects;
    FS.file_list(weathersEffects, "$game_weather_effects$", FS_ListFiles, "*.ltx");

    xr_string id;
    for (const auto& file : weathersEffects)
    {
        pcstr fileName = file.name.c_str();
        const size_t length = xr_strlen(fileName);
        id.assign(fileName, length - 4);
        EnvVec& env = WeatherFXs[id.c_str()];

        string_path file_name;
        FS.update_path(file_name, "$game_weather_effects$", fileName);
        CInifile* config = CInifile::Create(file_name);

        auto& sections = config->sections();
        env.reserve(sections.size() + 2);

        env.emplace_back(create_descriptor("00:00:00", nullptr));
        env.back()->dont_save = true;

        for (const auto& section : sections)
            env.emplace_back(create_descriptor(section->Name, config));

        CInifile::Destroy(config);

        env.emplace_back(create_descriptor("24:00:00", nullptr));
        env.back()->exec_time_loaded = DAY_LENGTH;
        env.back()->dont_save = true;
    }

    // ShoC style weather effects config
    u32 weatherEffectsCount = 0;
    if (pSettings->section_exist("weather_effects"))
    {
        weatherEffectsCount = pSettings->line_count("weather_effects");
        Log("~ ShoC style weather effects config detected");
    }

    for (u32 weatherIdx = 0; weatherIdx < weatherEffectsCount; ++weatherIdx)
    {
        pcstr weatherName, weatherSection, envSection;
        if (pSettings->r_line("weather_effects", weatherIdx, &weatherName, &weatherSection))
        {
            EnvVec& env = WeatherFXs[weatherName];
            env.soc_style = true;
            env.emplace_back(create_descriptor("00:00:00", nullptr));
            env.back()->dont_save = true;

            const u32 envCount = pSettings->line_count(weatherSection);
            pcstr executionTime;
            for (u32 envIdx = 0; envIdx < envCount; ++envIdx)
            {
                if (pSettings->r_line(weatherSection, envIdx, &executionTime, &envSection))
                {
                    env.emplace_back(create_descriptor(executionTime, pSettings, envSection));
                }
            }

            env.emplace_back(create_descriptor("24:00:00", nullptr));
            env.back()->exec_time_loaded = DAY_LENGTH;
            env.back()->dont_save = true;
        }
    }

    // sorting weather envs
    for (auto& fx : WeatherFXs)
    {
        R_ASSERT3(fx.second.size() > 1, "Environment in weather must >=2", fx.first.c_str());
        std::sort(fx.second.begin(), fx.second.end(), sort_env_etl_pred);
    }
}

void CEnvironment::load()
{
    if (!eff_Rain)
        eff_Rain = xr_new<CEffect_Rain>();
    if (!eff_LensFlare)
        eff_LensFlare = xr_new<CLensFlare>();
    if (!eff_Thunderbolt)
        eff_Thunderbolt = xr_new<CEffect_Thunderbolt>();

    load_weathers();
    load_weather_effects();
}

void CEnvironment::unload()
{
    // clear weathers
    for (auto& cycle : WeatherCycles)
        for (auto& env : cycle.second)
            xr_delete(env);

    WeatherCycles.clear();

    // clear weather effect
    for (auto& fx : WeatherFXs)
        for (auto& env : fx.second)
            xr_delete(env);

    WeatherFXs.clear();

    // clear ambient
    for (auto& ambient : Ambients)
        xr_delete(ambient);
    Ambients.clear();

    // misc
    xr_delete(eff_Rain);
    xr_delete(eff_LensFlare);
    xr_delete(eff_Thunderbolt);
    CurrentWeather = nullptr;
    CurrentWeatherName = nullptr;
    m_pRender->Clear();
    Invalidate();
}

void CEnvironment::ED_Reload()
{
    unload();
    load();
    OnFrame();
}

void CEnvironment::save() const
{
    string_path environment_config_path;
    FS.update_path(environment_config_path, "$game_config$", "weathers\\environment.ltx");

    CInifile* environment_config = xr_new<CInifile>(environment_config_path, false, false, false);

    save_weathers(environment_config);
    save_weather_effects(environment_config);

    CInifile::Destroy(environment_config);
}

void CEnvironment::save_weathers(CInifile* environment_config /*= nullptr*/) const
{
    string_path weathers_path;
    if (!FS.update_path(weathers_path, "$game_weathers$", "", false))
        FS.update_path(weathers_path, "$game_config$", "environment\\weathers");

    string_path weathers_path_soc;
    FS.update_path(weathers_path_soc, "$game_config$", "weathers\\weather_");

    bool should_save_environment_config = false;
    for (const auto& [name, descriptors] : WeatherCycles)
    {
        const bool soc_style = descriptors.soc_style;
        string_path weather_sect;
        if (soc_style && environment_config)
        {
            should_save_environment_config = true;
            strconcat(weather_sect, "sect_weather_", name.c_str());
            environment_config->w_string("weathers", name.c_str(), weather_sect);
        }

        string_path file_name;
        strconcat(file_name, soc_style ? weathers_path_soc : weathers_path,
            name.c_str(), ".ltx");

        CInifile* config = xr_new<CInifile>(file_name, false, false, true);

        for (const auto& desc : descriptors)
        {
            if (desc->dont_save)
                continue;

            string_path time_sect;
            if (soc_style)
            {
                strconcat(time_sect, "weather_", name.c_str(), "_", desc->m_identifier.c_str());
                std::replace(time_sect, time_sect + xr_strlen(time_sect), ':', '_');
                config->w_string(weather_sect, desc->m_identifier.c_str(), time_sect);
            }
            desc->save(*config, soc_style ? time_sect : nullptr);
        }

        CInifile::Destroy(config);
    }

    if (should_save_environment_config && environment_config)
        environment_config->save_at_end(true);
}

void CEnvironment::save_weather_effects(CInifile* environment_config /*= nullptr*/) const
{
    string_path effects_path;
    if (!FS.update_path(effects_path, "$game_weather_effects$", "", false))
        FS.update_path(effects_path, "$game_config$", "environment\\weather_effects");

    string_path effects_path_soc;
    FS.update_path(effects_path_soc, "$game_config$", "weathers\\weather_");

    bool should_save_environment_config = false;
    for (const auto& [name, descriptors] : WeatherFXs)
    {
        const bool soc_style = descriptors.soc_style;
        string_path weather_sect;
        if (soc_style && environment_config)
        {
            should_save_environment_config = true;
            strconcat(weather_sect, "sect_weather_", name.c_str());
            environment_config->w_string("weather_effects", name.c_str(), weather_sect);
        }

        string_path file_name;
        strconcat(file_name, soc_style ? effects_path_soc : effects_path,
            name.c_str(), ".ltx");

        CInifile* config = xr_new<CInifile>(file_name, false, false, true);

        for (const auto& desc : descriptors)
        {
            if (desc->dont_save)
                continue;

            string_path time_sect;
            if (soc_style)
            {
                strconcat(time_sect, "weather_", name.c_str(), "_", desc->m_identifier.c_str());
                std::replace(time_sect, time_sect + xr_strlen(time_sect), ':', '_');
                config->w_string(weather_sect, desc->m_identifier.c_str(), time_sect);
            }
            desc->save(*config, soc_style ? time_sect : nullptr);
        }

        CInifile::Destroy(config);
    }

    if (should_save_environment_config && environment_config)
        environment_config->save_at_end(true);
}
