#include "stdafx.h"

#ifndef MASTER_GOLD
#include "editor_base.h"
#include "editor_helper.h"

#include "Environment.h"
#include "thunderbolt.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"
#include "xr_efflensflare.h"

namespace xray::editor
{
using namespace imgui;

static bool window_weather_cycle = false;
static bool window_suns = false;
static bool window_ambients = false;
static bool window_thunderbolts = false;
static bool window_level_weathers = false;

struct combo_raii
{
    ~combo_raii()
    {
        ImGui::EndCombo();
    }
};

bool TimeFrameCombo(pcstr label, CEnvDescriptor*& descriptor)
{
    const auto& env = g_pGamePersistent->Environment();

    if (ImGui::BeginCombo(label, descriptor ? descriptor->m_identifier.c_str() : "##"))
    {
        combo_raii raii;

        if (ImGui::Selectable("##", !descriptor))
        {
            descriptor = nullptr;
            return true;
        }
        for (const auto& desc : *env.CurrentWeather)
        {
            if (ImGui::Selectable(desc->m_identifier.c_str(), descriptor ? descriptor->m_identifier == desc->m_identifier : false))
            {
                descriptor = desc;
                return true;
            }
        }
    }
    return false;
}

bool ConfigStyleSelector(pcstr label, bool& soc_style)
{
    bool result = false;
    enum ConfigStyle : int
    {
        ConfigStyle_SOC,
        ConfigStyle_CSCOP,
        ConfigStyle_COUNT
    };
    constexpr pcstr styles[ConfigStyle_COUNT] =
    {
        "SOC",
        "CS/COP"
    };
    int selected = soc_style ? ConfigStyle_SOC : ConfigStyle_CSCOP;
    if (ImGui::SliderInt(label, &selected, 0, ConfigStyle_COUNT - 1, styles[selected]))
    {
        switch ((ConfigStyle)selected)
        {
        case ConfigStyle_SOC:   soc_style = true; break;
        case ConfigStyle_CSCOP: soc_style = false; break;
        }
        result = true;
    }
    ItemHelp("This affects how environment color will be calculated in the mixer. "
             "(look at the environment color in the mixer category)");
    return result;
}

template <typename T>
void display_property(T&) {}

template <>
void display_property(CEnvDescriptor& descriptor)
{
    auto& env = g_pGamePersistent->Environment();

    ImGui::PushID(descriptor.m_identifier.c_str());
    if (ImGui::CollapsingHeader("sun##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("sun##lensflareid", descriptor.lens_flare ? descriptor.lens_flare->section.c_str() : ""))
        {
            if (ImGui::Selectable("<edit>", false))
            {
                window_suns = true;
            }
            if (ImGui::Selectable("##", !descriptor.lens_flare))
                descriptor.lens_flare = nullptr;
            for (CLensFlareDescriptor* desc : env.eff_LensFlare->GetDescriptors())
            {
                if (ImGui::Selectable(desc->section.c_str(), desc == descriptor.lens_flare))
                    descriptor.lens_flare = desc;
            }
            ImGui::EndCombo();
        }
        ItemHelp("Name in configs: \n"
                 "CS/COP: sun\n"
                 "   SOC: flares");

        ImGui::ColorEdit3("sun color", (float*)&descriptor.sun_color);

        ImGui::Checkbox("Dynamic sun direction", &descriptor.use_dynamic_sun_dir);
        ItemHelp("If enabled, engine will automatically calculate sun direction (and ignore your values) on full dynamic lighting renderers.\n"
                 "You still need to provide sun altitude and longitude for static and (not full) dynamic lighting.");

        if (ImGui::Button("Calculate sun direction"))
        {
            std::tie(descriptor.sun_dir, std::ignore) = env.CurrentEnv.calculate_dynamic_sun_dir(descriptor.exec_time, descriptor.sun_azimuth);
        }
        ItemHelp("Calculates realistic sun direction, uses sun azimuth");

        float azimuth = rad2deg(descriptor.sun_azimuth);
        if (ImGui::DragFloat("azimuth", &azimuth, 0.5f, -360.0f, 360.f))
            descriptor.sun_azimuth = deg2rad(azimuth);
        ItemHelp("Dynamic sun direction correction.\n"
            "Name in configs: \n"
            "sun_azimuth");

        float altitude  = rad2deg(descriptor.sun_dir.getH());
        float longitude = rad2deg(descriptor.sun_dir.getP());

        if (ImGui::DragFloat("altitude", &altitude, 0.5f, -360.0f, 360.f))
            descriptor.sun_dir.setHP(deg2rad(altitude), deg2rad(longitude));
        ItemHelp("Name in configs: \n"
                 "CS/COP: sun_altitude\n"
                 "   SOC: sun_dir (vector2 with longitude (x) and altitude (y)"
                 "\n\n"
                 "If dynamic sun direction is disabled, values will be saved like in SOC: as sun_dir");

        if (ImGui::DragFloat("longitude", &longitude, 0.5f, -360.0f, 360.f))
            descriptor.sun_dir.setHP(deg2rad(altitude), deg2rad(longitude));
        ItemHelp("Name in configs: \n"
                 "CS/COP: sun_longitude\n"
                 "   SOC: sun_dir (vector2 with longitude (x) and altitude (y)"
                 "\n\n"
                 "If dynamic sun direction is disabled, values will be saved like in SOC: as sun_dir");

        ImGui::DragFloat("shafts intensity", &descriptor.m_fSunShaftsIntensity, 0.001f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("hemisphere##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (InputText("sky texture", descriptor.sky_texture_name))
        {
            string_path temp;
            strconcat(temp, descriptor.sky_texture_name.c_str(), "#small");
            descriptor.sky_texture_env_name = temp;

            descriptor.on_device_create();
        }

        ImGui::ColorEdit3("sky color", (float*)&descriptor.sky_color);
        ImGui::ColorEdit4("hemi color", (float*)&descriptor.hemi_color, ImGuiColorEditFlags_AlphaBar);
        ItemHelp("Name in configs: \n"
                 "hemisphere_color");

        float rotation = rad2deg(descriptor.sky_rotation);
        if (ImGui::DragFloat("sky rotation", &rotation, 0.5f, -360.0f, 360.f))
            descriptor.sky_rotation = deg2rad(rotation);
    }
    if (ImGui::CollapsingHeader("clouds##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (InputText("clouds texture", descriptor.clouds_texture_name))
            descriptor.on_device_create();

        ImGui::ColorEdit4("clouds color", (float*)&descriptor.clouds_color, ImGuiColorEditFlags_AlphaBar);

        float rotation = rad2deg(descriptor.clouds_rotation);
        if (ImGui::DragFloat("clouds rotation", &rotation, 0.5f, -360.0f, 360.f))
            descriptor.clouds_rotation = deg2rad(rotation);
    }
    if (ImGui::CollapsingHeader("ambient##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("ambient##env_ambient", descriptor.env_ambient ? descriptor.env_ambient->name().c_str() : ""))
        {
            if (ImGui::Selectable("<edit>", false))
            {
                window_ambients = true;
            }
            if (ImGui::Selectable("##", !descriptor.env_ambient))
                descriptor.env_ambient = nullptr;
            const shared_str current = descriptor.env_ambient ? descriptor.env_ambient->name() : nullptr;
            for (const auto& ambient : env.Ambients)
            {
                if (ImGui::Selectable(ambient->name().c_str(), ambient->name() == current))
                    descriptor.env_ambient = ambient;
            }
            ImGui::EndCombo();
        }
        ItemHelp("Name in configs: \n"
                 "CS/COP: ambient\n"
                 "   SOC: env_ambient");

        ImGui::ColorEdit3("ambient color", (float*)&descriptor.ambient);
        ItemHelp("Name in configs: \n"
                 "CS/COP: ambient_color\n"
                 "   SOC: ambient");
    }
    if (ImGui::CollapsingHeader("fog##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("far plane", &descriptor.far_plane);
        ImGui::ColorEdit3("fog color", (float*)&descriptor.fog_color);
        ImGui::DragFloat("fog distance", &descriptor.fog_distance, 1.0f, 0.0f, descriptor.far_plane);
        ItemHelp("Fog distance can't be higher than far plane.");
        ImGui::DragFloat("fog density", &descriptor.fog_density, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("water intensity", &descriptor.m_fWaterIntensity, 0.001f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("rain##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("rain color", (float*)&descriptor.rain_color);
        ImGui::DragFloat("rain density", &descriptor.rain_density, 0.001f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("thunderbolts##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("thunderbolts", descriptor.thunderbolt ? descriptor.thunderbolt->section.c_str() : ""))
        {
            if (ImGui::Selectable("<edit>", false))
            {
                window_thunderbolts = true;
            }
            if (ImGui::Selectable("##", !descriptor.thunderbolt))
                descriptor.thunderbolt = nullptr;
            for (const auto& collection : env.eff_Thunderbolt->GetCollections())
            {
                if (ImGui::Selectable(collection->section.c_str(), collection == descriptor.thunderbolt))
                    descriptor.thunderbolt = collection;
            }
            ImGui::EndCombo();
        }
        ItemHelp("Name in configs: \n"
                 "CS/COP: thunderbolts_collection\n"
                 "   SOC: thunderbolt");

        ImGui::DragFloat("duration", &descriptor.bolt_duration);
        ImGui::DragFloat("period", &descriptor.bolt_period);
    }
    if (ImGui::CollapsingHeader("wind##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float direction = rad2deg(descriptor.wind_direction);
        if (ImGui::DragFloat("wind direction", &direction, 0.5f, -360.0f, 360.f))
            descriptor.wind_direction = deg2rad(direction);

        ImGui::DragFloat("wind velocity", &descriptor.wind_velocity, 1.0f, 0.0f, 1000.0f);
    }
    ImGui::PopID();
}

template <>
void display_property(CEnvDescriptorMixer& descriptor)
{
    display_property<CEnvDescriptor>(descriptor);

    ImGui::PushID(descriptor.m_identifier.c_str());
    if (ImGui::CollapsingHeader("mixer##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("environment color", (float*)&descriptor.env_color);
        ItemHelp("CS/COP: takes hemisphere color as the base value.\n"
                 "SOC   : takes sky color as the base value.");

        ImGui::DragFloat("weight", &descriptor.weight, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("modifier power", &descriptor.modif_power);
        ImGui::DragFloat("fog near", &descriptor.fog_near);
        ImGui::DragFloat("fog far", &descriptor.fog_far);
    }
    ImGui::PopID();
}

template <>
void display_property(CEffect_Thunderbolt& bolt)
{
    float altitude[2] = { rad2deg(bolt.p_var_alt.x) , rad2deg(bolt.p_var_alt.y) };
    if (ImGui::DragFloat2("altitude", altitude, 0.5f, -360.0f, 360.f))
        bolt.p_var_alt = { rad2deg(altitude[0]) , rad2deg(altitude[1]) };

    float deltalongitude = rad2deg(bolt.p_var_long);
    if (ImGui::DragFloat("wind direction", &deltalongitude, 0.5f, -360.0f, 360.f))
        bolt.p_var_long = deg2rad(deltalongitude);

    ImGui::DragFloat("minimum distance factor", &bolt.p_min_dist, 0.001f, 0.0f, CEffect_Thunderbolt::MAX_DIST_FACTOR);
    ItemHelp("Distance from far plane");

    float tilt = rad2deg(bolt.p_tilt);
    if (ImGui::DragFloat("tilt", &tilt, 0.01f, 15.0f, 30.f))
        bolt.p_tilt = deg2rad(tilt);

    ImGui::DragFloat("second probability", &bolt.p_second_prop, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("sky color", &bolt.p_sky_color, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("sun color", &bolt.p_sun_color, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("fog color", &bolt.p_fog_color, 0.001f, 0.0f, 1.0f);
}

void ide::ShowWeatherEditor()
{
    if (ImGui::Begin("Weather editor", &m_show_weather_editor, get_default_window_flags()))
    {
        auto& env = g_pGamePersistent->Environment();

        if (ImGui::BeginMenuBar())
        {
            const bool paused = Device.Paused();
            if (ImGui::RadioButton("Pause", paused))
                Device.Pause(!paused, TRUE, TRUE, "editor query");

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::Button("Reset all"))
                    env.ED_Reload();

                if (ImGui::Button("Save all"))
                    env.save();

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows"))
            {
                if (ImGui::MenuItem("Weather cycles", nullptr, &window_weather_cycle)) {}
                if (ImGui::MenuItem("Suns", nullptr, &window_suns)) {}
                if (ImGui::MenuItem("Ambients", nullptr, &window_ambients)) {}
                if (ImGui::MenuItem("Thunderbolts", nullptr, &window_thunderbolts)) {}
                if (ImGui::MenuItem("Level weathers", nullptr, &window_level_weathers)) {}

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        auto& current = env.CurrentEnv;
        float time_factor = g_pGameLevel ? g_pGameLevel->GetEnvironmentTimeFactor() : env.fTimeFactor;

        if (ImGui::CollapsingHeader("Environment time", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float time = g_pGameLevel ? g_pGameLevel->GetEnvironmentGameDayTimeSec() : env.GetGameTime();

            u32 hours, minutes, seconds;
            env.SplitTime(time, hours, minutes, seconds);

            string128 temp;
            xr_sprintf(temp, "Current time: %02d:%02d:%02d###environment_time", hours, minutes, seconds);

            if (ImGui::SliderFloat(temp, &time, 0, DAY_LENGTH, "", ImGuiSliderFlags_NoInput))
            {
                env.Invalidate();
                if (g_pGameLevel)
                    g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(time * 1000.f), time_factor);
                env.SetGameTime(time, time_factor);
                env.lerp();
            }
            if (ImGui::DragFloat("Time factor", &time_factor, 1.f, 1.f, 1000.f))
            {
                if (g_pGameLevel)
                    g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(time * 1000.f), time_factor);
                env.SetGameTime(time, time_factor);
            }
        }

        if (ImGui::CollapsingHeader("Current environment##header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginTable("Current environment##table", 2))
            {
                ImGui::TableNextColumn();
                static bool force_change_weather = true;
                if (ImGui::Checkbox("##force_change_weather", &force_change_weather))
                {
                    if (force_change_weather)
                        env.SetWeather(env.CurrentCycleName, true);
                }
                ItemHelp("Force change weather", false);
                ImGui::SameLine();

                if (ImGui::BeginCombo("Weather cycle", env.CurrentCycleName.c_str()))
                {
                    if (ImGui::Selectable("<edit>", false)) {}
                    for (const auto& [identifier, env_descriptors] : env.WeatherCycles)
                    {
                        if (ImGui::Selectable(identifier.c_str(), env.CurrentCycleName == identifier))
                            env.SetWeather(identifier, force_change_weather);
                    }
                    ImGui::EndCombo();
                }
                //ImGui::SameLine();
                if (ConfigStyleSelector("Weather config style", env.CurrentWeather->soc_style))
                    current.soc_style = env.CurrentWeather->soc_style;

                ImGui::TableNextColumn();
                if (ImGui::BeginCombo("Effect", env.IsWFXPlaying() ? env.CurrentWeatherName.c_str() : ""))
                {
                    if (ImGui::Selectable("<edit>", false)) {}

                    if (ImGui::Selectable("##", !env.IsWFXPlaying()))
                        env.StopWFX();

                    for (const auto& [identifier, env_descriptors] : env.WeatherFXs)
                    {
                        if (ImGui::Selectable(identifier.c_str(), env.CurrentWeatherName == identifier))
                        {
                            if (env.IsWFXPlaying())
                                env.StopWFX();
                            env.SetWeatherFX(identifier);
                        }
                    }
                    ImGui::EndCombo();
                }
                //ImGui::SameLine();
                ImGui::BeginDisabled(!env.IsWFXPlaying());
                if (ConfigStyleSelector("Effect config style", env.CurrentWeather->soc_style))
                    current.soc_style = env.CurrentWeather->soc_style;
                ImGui::EndDisabled();
                ImGui::EndTable();
            }
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Live edit##header", ImGuiTreeNodeFlags_DefaultOpen))
        {
            constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner;

            if (ImGui::BeginTable("Environment lerp", 3, flags))
            {
                ImGui::TableNextColumn();
                TimeFrameCombo("Time frame##current0", env.Current[0]);

                ImGui::TableNextColumn();
                u32 hours, minutes, seconds;
                env.SplitTime(current.exec_time, hours, minutes, seconds);

                string128 temp;
                xr_sprintf(temp, "%02d:%02d:%02d###frame_time", hours, minutes, seconds);

                const float current0_time = env.Current[0] ? env.Current[0]->exec_time : current.exec_time;
                const float current1_time = env.Current[1] ? env.Current[1]->exec_time : current.exec_time;

                ImGui::BeginDisabled(!env.Current[0] || !env.Current[1]);
                if (ImGui::SliderFloat(temp, &current.exec_time,
                    current0_time, current1_time, "", ImGuiSliderFlags_NoInput))
                {
                    //env.Invalidate();
                    if (g_pGameLevel)
                        g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(current.exec_time * 1000.f), time_factor);
                    env.SetGameTime(current.exec_time, time_factor);
                    env.lerp();
                }
                ImGui::EndDisabled();

                ImGui::TableNextColumn();
                TimeFrameCombo("Time frame##current1", env.Current[1]);

                ImGui::EndTable();
            }

            if (ImGui::BeginTable("Environment lerp", 3, flags))
            {
                ImGui::TableSetupColumn("current");
                ImGui::TableSetupColumn("blend");
                ImGui::TableSetupColumn("target");
                ImGui::TableHeadersRow();

                ImGui::TableNextColumn();
                if (env.Current[0])
                    display_property(*env.Current[0]);
                else
                    ImGui::Text("Please, load a level or select time frame manually.");

                ImGui::TableNextColumn();
                display_property(current);

                ImGui::TableNextColumn();
                if (env.Current[1])
                    display_property(*env.Current[1]);
                else
                    ImGui::Text("Please, load a level or select time frame manually.");

                ImGui::EndTable();
            } // if (ImGui::BeginTable("Environment lerp", 3, flags))
        } // if (ImGui::CollapsingHeader("Live edit", ImGuiTreeNodeFlags_DefaultOpen))
    }
    ImGui::End();
}

void ShowLevelWeathers()
{
    ImGui::Begin("level weathers");

    if (ImGui::TreeNode("single"))
    {
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("multiplayer"))
    {
        ImGui::TreePop();
    }

    ImGui::End();
}
} // namespace xray::editor
#else
namespace xray::editor
{
void ide::ShowWeatherEditor() {}
} // namespace xray::editor
#endif // !MASTER_GOLD
