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
        use_flags.set(eViewDist, TRUE);
    }
    if (M.use_flags.test(eFogColor))
    {
        fog_color.mad(M.fog_color, _power);
        use_flags.set(eFogColor, TRUE);
    }
    if (M.use_flags.test(eFogDensity))
    {
        fog_density += M.fog_density * _power;
        use_flags.set(eFogDensity, TRUE);
    }

    if (M.use_flags.test(eAmbientColor))
    {
        ambient.mad(M.ambient, _power);
        use_flags.set(eAmbientColor, TRUE);
    }

    if (M.use_flags.test(eSkyColor))
    {
        sky_color.mad(M.sky_color, _power);
        use_flags.set(eSkyColor, TRUE);
    }

    if (M.use_flags.test(eHemiColor))
    {
        hemi_color.mad(M.hemi_color, _power);
        use_flags.set(eHemiColor, TRUE);
    }

    return _power;
}

//-----------------------------------------------------------------------------
// Environment ambient
//-----------------------------------------------------------------------------
void CEnvAmbient::SSndChannel::load(CInifile& config, LPCSTR sect)
{
    m_load_section = sect;

    m_sound_dist.x = config.r_float(m_load_section, "min_distance");
    m_sound_dist.y = config.r_float(m_load_section, "max_distance");
    m_sound_period.x = config.r_s32(m_load_section, "period0");
    m_sound_period.y = config.r_s32(m_load_section, "period1");
    m_sound_period.z = config.r_s32(m_load_section, "period2");
    m_sound_period.w = config.r_s32(m_load_section, "period3");

    // m_sound_period = config.r_ivector4(sect,"sound_period");
    R_ASSERT(m_sound_period.x <= m_sound_period.y && m_sound_period.z <= m_sound_period.w);
    // m_sound_period.mul (1000);// now in ms
    // m_sound_dist = config.r_fvector2(sect,"sound_dist");
    R_ASSERT2(m_sound_dist.y > m_sound_dist.x, sect);

    LPCSTR snds = config.r_string(sect, "sounds");
    u32 cnt = _GetItemCount(snds);
    string_path tmp;
    R_ASSERT3(cnt, "sounds empty", sect);

    m_sounds.resize(cnt);

    for (u32 k = 0; k < cnt; ++k)
    {
        _GetItem(snds, k, tmp);
        m_sounds[k].create(tmp, st_Effect, sg_SourceType);
    }
}

CEnvAmbient::SEffect* CEnvAmbient::create_effect(CInifile& config, LPCSTR id)
{
    SEffect* result = new SEffect();
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

CEnvAmbient::SSndChannel* CEnvAmbient::create_sound_channel(CInifile& config, LPCSTR id)
{
    SSndChannel* result = new SSndChannel();
    result->load(config, id);
    return (result);
}

CEnvAmbient::~CEnvAmbient() { destroy(); }
void CEnvAmbient::destroy()
{
    delete_data(m_effects);
    delete_data(m_sound_channels);
}

void CEnvAmbient::load(
    CInifile& ambients_config, CInifile& sound_channels_config, CInifile& effects_config, const shared_str& sect)
{
    m_ambients_config_filename = ambients_config.fname();
    m_load_section = sect;
    string_path tmp;

    // sounds
    pcstr channels = ambients_config.r_string(sect, "sound_channels");
    u32 cnt = _GetItemCount(channels);
    // R_ASSERT3 (cnt,"sound_channels empty", sect.c_str());
    m_sound_channels.resize(cnt);

    for (u32 i = 0; i < cnt; ++i)
        m_sound_channels[i] = create_sound_channel(sound_channels_config, _GetItem(channels, i, tmp));

    // effects
    m_effect_period.set(iFloor(ambients_config.r_float(sect, "min_effect_period") * 1000.f),
        iFloor(ambients_config.r_float(sect, "max_effect_period") * 1000.f));
    pcstr effs = ambients_config.r_string(sect, "effects");
    cnt = _GetItemCount(effs);
    // R_ASSERT3 (cnt,"effects empty", sect.c_str());

    m_effects.resize(cnt);
    for (u32 k = 0; k < cnt; ++k)
        m_effects[k] = create_effect(effects_config, _GetItem(effs, k, tmp));

    R_ASSERT(!m_sound_channels.empty() || !m_effects.empty());
}

//-----------------------------------------------------------------------------
// Environment descriptor
//-----------------------------------------------------------------------------
CEnvDescriptor::CEnvDescriptor(shared_str const& identifier) : m_identifier(identifier)
{
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

    m_fSunShaftsIntensity = 0;
    m_fWaterIntensity = 1;

    m_fTreeAmplitudeIntensity = 0.01;

    lens_flare_id = "";
    tb_id = "";

    env_ambient = nullptr;
}

#define C_CHECK(C)                                                           \
    if (C.x < 0 || C.x > 2 || C.y < 0 || C.y > 2 || C.z < 0 || C.z > 2)      \
    {                                                                        \
        Msg("! Invalid '%s' in env-section '%s'", #C, m_identifier.c_str()); \
    }
void CEnvDescriptor::load(CEnvironment& environment, CInifile& config)
{
    Ivector3 tm = {0, 0, 0};
    sscanf(m_identifier.c_str(), "%d:%d:%d", &tm.x, &tm.y, &tm.z);
    R_ASSERT3((tm.x >= 0) && (tm.x < 24) && (tm.y >= 0) && (tm.y < 60) && (tm.z >= 0) && (tm.z < 60),
        "Incorrect weather time", m_identifier.c_str());
    exec_time = tm.x * 3600.f + tm.y * 60.f + tm.z;
    exec_time_loaded = exec_time;
    string_path st, st_env;
    xr_strcpy(st, config.r_string(m_identifier.c_str(), "sky_texture"));
    strconcat(sizeof(st_env), st_env, st, "#small");
    sky_texture_name = st;
    sky_texture_env_name = st_env;
    clouds_texture_name = config.r_string(m_identifier.c_str(), "clouds_texture");
    LPCSTR cldclr = config.r_string(m_identifier.c_str(), "clouds_color");
    float multiplier = 0, save = 0;
    sscanf(cldclr, "%f,%f,%f,%f,%f", &clouds_color.x, &clouds_color.y, &clouds_color.z, &clouds_color.w, &multiplier);
    save = clouds_color.w;
    clouds_color.mul(.5f * multiplier);
    clouds_color.w = save;

    sky_color = config.r_fvector3(m_identifier.c_str(), "sky_color");

    if (config.line_exist(m_identifier.c_str(), "sky_rotation"))
        sky_rotation = deg2rad(config.r_float(m_identifier.c_str(), "sky_rotation"));
    else
        sky_rotation = 0;
    far_plane = config.r_float(m_identifier.c_str(), "far_plane");
    fog_color = config.r_fvector3(m_identifier.c_str(), "fog_color");
    fog_density = config.r_float(m_identifier.c_str(), "fog_density");
    fog_distance = config.r_float(m_identifier.c_str(), "fog_distance");
    rain_density = config.r_float(m_identifier.c_str(), "rain_density");
    clamp(rain_density, 0.f, 1.f);
    rain_color = config.r_fvector3(m_identifier.c_str(), "rain_color");
    wind_velocity = config.r_float(m_identifier.c_str(), "wind_velocity");
    wind_direction = deg2rad(config.r_float(m_identifier.c_str(), "wind_direction"));
    ambient = config.r_fvector3(m_identifier.c_str(), "ambient_color");
    hemi_color = config.r_fvector4(m_identifier.c_str(), "hemisphere_color");
    sun_color = config.r_fvector3(m_identifier.c_str(), "sun_color");
    // if (config.line_exist(m_identifier.c_str(),"sun_altitude"))
    sun_dir.setHP(deg2rad(config.r_float(m_identifier.c_str(), "sun_altitude")),
        deg2rad(config.r_float(m_identifier.c_str(), "sun_longitude")));
    R_ASSERT(_valid(sun_dir));
    // else
    // sun_dir.setHP (
    // deg2rad(config.r_fvector2(m_identifier.c_str(),"sun_dir").y),
    // deg2rad(config.r_fvector2(m_identifier.c_str(),"sun_dir").x)
    // );
    VERIFY2(sun_dir.y < 0, "Invalid sun direction settings while loading");

    lens_flare_id = environment.eff_LensFlare->AppendDef(
        environment, environment.m_suns_config, config.r_string(m_identifier.c_str(), "sun"));
    tb_id = environment.eff_Thunderbolt->AppendDef(environment, environment.m_thunderbolt_collections_config,
        environment.m_thunderbolts_config, config.r_string(m_identifier.c_str(), "thunderbolt_collection"));
    bolt_period = (tb_id.size()) ? config.r_float(m_identifier.c_str(), "thunderbolt_period") : 0.f;
    bolt_duration = (tb_id.size()) ? config.r_float(m_identifier.c_str(), "thunderbolt_duration") : 0.f;
    env_ambient = config.line_exist(m_identifier.c_str(), "ambient") ?
        environment.AppendEnvAmb(config.r_string(m_identifier.c_str(), "ambient")) :
        0;



    m_fSunShaftsIntensity = config.read_if_exists<float>(m_identifier.c_str(), "sun_shafts_intensity", 0.0);
    m_fWaterIntensity = config.read_if_exists<float>(m_identifier.c_str(), "water_intensity", 1.0);
    m_fTreeAmplitudeIntensity = config.read_if_exists<float>(m_identifier.c_str(), "tree_amplitude_intensity", 0.01);

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
CEnvDescriptorMixer::CEnvDescriptorMixer(shared_str const& identifier) : CEnvDescriptor(identifier) {}
void CEnvDescriptorMixer::destroy()
{
    m_pDescriptorMixer->Destroy();

    // Reuse existing code
    on_device_destroy();
}

void CEnvDescriptorMixer::clear()
{
    m_pDescriptorMixer->Clear();
}

void CEnvDescriptorMixer::lerp(
    CEnvironment*, CEnvDescriptor& A, CEnvDescriptor& B, float f, CEnvModifier& Mdf, float modifier_power)
{
    float modif_power = 1.f / (modifier_power + 1); // the environment itself
    float fi = 1 - f;

    m_pDescriptorMixer->lerp(&*A.m_pDescriptor, &*B.m_pDescriptor);

    weight = f;

    clouds_color.lerp(A.clouds_color, B.clouds_color, f);

    sky_rotation = (fi * A.sky_rotation + f * B.sky_rotation);

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

    R_ASSERT(_valid(A.sun_dir));
    R_ASSERT(_valid(B.sun_dir));
    sun_dir.lerp(A.sun_dir, B.sun_dir, f).normalize();
    R_ASSERT(_valid(sun_dir));

    VERIFY2(sun_dir.y < 0, "Invalid sun direction settings while lerp");
}

//-----------------------------------------------------------------------------
// Environment IO
//-----------------------------------------------------------------------------
CEnvAmbient* CEnvironment::AppendEnvAmb(const shared_str& sect)
{
    for (const auto& ambient : Ambients)
        if (ambient->name().equal(sect))
            return ambient;

    Ambients.emplace_back(new CEnvAmbient());
    Ambients.back()->load(*m_ambients_config, *m_sound_channels_config, *m_effects_config, sect);
    return Ambients.back();
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
    CInifile* level_ambients = new CInifile(FS.update_path(full_path, "$game_config$", path), TRUE, TRUE, FALSE);

    for (const auto& ambient : Ambients)
    {
        shared_str section_name = ambient->name();

        // choose a source ini file
        CInifile* source =
            (level_ambients && level_ambients->section_exist(section_name)) ? level_ambients : m_ambients_config;

        // check and reload if needed
        if (xr_strcmp(ambient->get_ambients_config_filename().c_str(), source->fname()))
        {
            ambient->destroy();
            ambient->load(*source, *m_sound_channels_config, *m_effects_config, section_name);
        }
    }

    xr_delete(level_ambients);
}

CEnvDescriptor* CEnvironment::create_descriptor(shared_str const& identifier, CInifile* config)
{
    CEnvDescriptor* result = new CEnvDescriptor(identifier);
    if (config)
        result->load(*this, *config);
    return (result);
}

void CEnvironment::load_weathers()
{
    if (!WeatherCycles.empty())
        return;

    auto file_list = FS.file_list_open("$game_weathers$", "");

    xr_string id;
    for (const auto& file : *file_list)
    {
        const size_t length = xr_strlen(file);
        VERIFY(length >= 4);
        VERIFY(file[length - 4] == '.');
        VERIFY(file[length - 3] == 'l');
        VERIFY(file[length - 2] == 't');
        VERIFY(file[length - 1] == 'x');
        id.assign(file, length - 4);
        EnvVec& env = WeatherCycles[id.c_str()];

        string_path file_name;
        FS.update_path(file_name, "$game_weathers$", file);
        CInifile* config = CInifile::Create(file_name);

        auto& sections = config->sections();
        env.reserve(sections.size());

        for (const auto& section : sections)
            env.emplace_back(create_descriptor(section->Name, config));

        CInifile::Destroy(config);
    }

    FS.file_list_close(file_list);

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

    auto file_list = FS.file_list_open("$game_weather_effects$", "");
    VERIFY(file_list);
    xr_string id;
    for (const auto& file : *file_list)
    {
        const size_t length = xr_strlen(file);
        VERIFY(length >= 4);
        VERIFY(file[length - 4] == '.');
        VERIFY(file[length - 3] == 'l');
        VERIFY(file[length - 2] == 't');
        VERIFY(file[length - 1] == 'x');
        id.assign(file, length - 4);
        EnvVec& env = WeatherFXs[id.c_str()];

        string_path file_name;
        FS.update_path(file_name, "$game_weather_effects$", file);
        CInifile* config = CInifile::Create(file_name);

        auto& sections = config->sections();
        env.reserve(sections.size() + 2);

        env.emplace_back(create_descriptor("00:00:00", nullptr));

        for (const auto& section : sections)
            env.emplace_back(create_descriptor(section->Name, config));

        CInifile::Destroy(config);

        env.emplace_back(create_descriptor("24:00:00", nullptr));
        env.back()->exec_time_loaded = DAY_LENGTH;
    }

    FS.file_list_close(file_list);

#if 0
    int line_count = pSettings->line_count("weather_effects");
    for (int w_idx = 0; w_idx < line_count; w_idx++)
    {
        LPCSTR weather, sect_w;
        if (pSettings->r_line("weather_effects", w_idx, &weather, &sect_w))
        {
            EnvVec& env = WeatherFXs[weather];
            env.emplace_back(new CEnvDescriptor("00:00:00"));
            env.back()->exec_time_loaded = 0;
            //. why? env.emplace_back (new CEnvDescriptor("00:00:00")); env.back()->exec_time_loaded = 0;
            int env_count = pSettings->line_count(sect_w);
            LPCSTR exec_tm, sect_e;
            for (int env_idx = 0; env_idx < env_count; env_idx++)
            {
                if (pSettings->r_line(sect_w, env_idx, &exec_tm, &sect_e))
                    env.emplace_back(create_descriptor(sect_e));
            }
            env.emplace_back(create_descriptor("23:59:59"));
            env.back()->exec_time_loaded = DAY_LENGTH;
        }
    }
#endif // #if 0

    // sorting weather envs
    for (auto& fx : WeatherFXs)
    {
        R_ASSERT3(fx.second.size() > 1, "Environment in weather must >=2", fx.first.c_str());
        std::sort(fx.second.begin(), fx.second.end(), sort_env_etl_pred);
    }
}

void CEnvironment::load()
{
    if (!CurrentEnv)
        create_mixer();

    m_pRender->OnLoad();

    if (!eff_Rain)
        eff_Rain = new CEffect_Rain();
    if (!eff_LensFlare)
        eff_LensFlare = new CLensFlare();
    if (!eff_Thunderbolt)
        eff_Thunderbolt = new CEffect_Thunderbolt();

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
    CurrentEnv->clear();
    Invalidate();

    m_pRender->OnUnload();
}
