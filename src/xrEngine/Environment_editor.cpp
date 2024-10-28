#include "stdafx.h"

#include "Environment.h"
#include "thunderbolt.h"
#include "xr_efflensflare.h"

#include "IGame_Level.h"

#include "editor_helper.h"

#ifndef MASTER_GOLD
namespace
{
bool window_weather_cycle = false;
bool window_suns = false;
bool window_ambients = false;
bool window_thunderbolts = false;
bool window_level_weathers = false;

struct combo_raii
{
    ~combo_raii()
    {
        ImGui::EndCombo();
    }
};

bool TimeFrameCombo(pcstr label, CEnvDescriptor*& descriptor, const CEnvironment::EnvVec* currentWeather)
{
    if (ImGui::BeginCombo(label, descriptor ? descriptor->m_identifier.c_str() : "##"))
    {
        combo_raii raii;

        if (ImGui::Selectable("##", !descriptor))
        {
            descriptor = nullptr;
            return true;
        }
        if (currentWeather)
        {
            for (const auto& desc : *currentWeather)
            {
                if (ImGui::Selectable(desc->m_identifier.c_str(), descriptor ? descriptor->m_identifier == desc->m_identifier : false))
                {
                    descriptor = desc;
                    return true;
                }
            }
        }
    }
    return false;
}

bool ConfigStyleSelector(pcstr label, bool& soc_style)
{
    using namespace xray::imgui;

    bool result = false;
    enum ConfigStyle
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
        switch (static_cast<ConfigStyle>(selected))
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
}
#endif

void CEnvDescriptor::ed_show_params(const CEnvironment& env)
{
#ifndef MASTER_GOLD
    using namespace xray::imgui;

    ImGui::PushID(m_identifier.c_str());
    if (ImGui::CollapsingHeader("sun##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("sun##lensflareid", lens_flare ? lens_flare->section.c_str() : ""))
        {
            if (ImGui::Selectable("<edit>", false))
            {
                window_suns = true;
            }
            if (ImGui::Selectable("##", !lens_flare))
                lens_flare = nullptr;
            for (CLensFlareDescriptor* desc : env.eff_LensFlare->GetDescriptors())
            {
                if (ImGui::Selectable(desc->section.c_str(), desc == lens_flare))
                    lens_flare = desc;
            }
            ImGui::EndCombo();
        }
        ItemHelp("Name in configs: \n"
                 "CS/COP: sun\n"
                 "   SOC: flares");

        ImGui::ColorEdit3("sun color", reinterpret_cast<float*>(&sun_color));

        ImGui::Checkbox("Dynamic sun direction", &use_dynamic_sun_dir);
        ItemHelp("If enabled, engine will automatically calculate sun direction (and ignore your values) on full dynamic lighting renderers.\n"
                 "You still need to provide sun altitude and longitude for static and (not full) dynamic lighting.");

        if (ImGui::Button("Calculate sun direction"))
        {
            std::tie(sun_dir, std::ignore) = CEnvDescriptorMixer::calculate_dynamic_sun_dir(exec_time, sun_azimuth);
        }
        ItemHelp("Calculates realistic sun direction, uses sun azimuth");

        float azimuth = rad2deg(sun_azimuth);
        if (ImGui::DragFloat("azimuth", &azimuth, 0.5f, -360.0f, 360.f))
            sun_azimuth = deg2rad(azimuth);
        ItemHelp("Dynamic sun direction correction.\n"
            "Name in configs: \n"
            "sun_azimuth");

        float altitude  = rad2deg(sun_dir.getH());
        float longitude = rad2deg(sun_dir.getP());

        if (ImGui::DragFloat("altitude", &altitude, 0.5f, -360.0f, 360.f))
            sun_dir.setHP(deg2rad(altitude), deg2rad(longitude));
        ItemHelp("Name in configs: \n"
                 "CS/COP: sun_altitude\n"
                 "   SOC: sun_dir (vector2 with longitude (x) and altitude (y)"
                 "\n\n"
                 "If dynamic sun direction is disabled, values will be saved like in SOC: as sun_dir");

        if (ImGui::DragFloat("longitude", &longitude, 0.5f, -360.0f, 360.f))
            sun_dir.setHP(deg2rad(altitude), deg2rad(longitude));
        ItemHelp("Name in configs: \n"
                 "CS/COP: sun_longitude\n"
                 "   SOC: sun_dir (vector2 with longitude (x) and altitude (y)"
                 "\n\n"
                 "If dynamic sun direction is disabled, values will be saved like in SOC: as sun_dir");

        ImGui::DragFloat("shafts intensity", &m_fSunShaftsIntensity, 0.001f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("hemisphere##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (InputText("sky texture", sky_texture_name))
        {
            string_path temp;
            strconcat(temp, sky_texture_name.c_str(), "#small");
            sky_texture_env_name = temp;

            on_device_create();
        }

        ImGui::ColorEdit3("sky color", reinterpret_cast<float*>(&sky_color));
        ImGui::ColorEdit4("hemi color", reinterpret_cast<float*>(&hemi_color), ImGuiColorEditFlags_AlphaBar);
        ItemHelp("Name in configs: \n"
                 "hemisphere_color");

        float rotation = rad2deg(sky_rotation);
        if (ImGui::DragFloat("sky rotation", &rotation, 0.5f, -360.0f, 360.f))
            sky_rotation = deg2rad(rotation);
    }
    if (ImGui::CollapsingHeader("clouds##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (InputText("clouds texture", clouds_texture_name))
            on_device_create();

        ImGui::ColorEdit4("clouds color", reinterpret_cast<float*>(&clouds_color), ImGuiColorEditFlags_AlphaBar);

        float rotation = rad2deg(clouds_rotation);
        if (ImGui::DragFloat("clouds rotation", &rotation, 0.5f, -360.0f, 360.f))
            clouds_rotation = deg2rad(rotation);
    }
    if (ImGui::CollapsingHeader("ambient##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("ambient##env_ambient", env_ambient ? env_ambient->name().c_str() : ""))
        {
            if (ImGui::Selectable("<edit>", false))
            {
                window_ambients = true;
            }
            if (ImGui::Selectable("##", !env_ambient))
                env_ambient = nullptr;
            const shared_str current = env_ambient ? env_ambient->name() : nullptr;
            for (const auto& amb : env.Ambients)
            {
                if (ImGui::Selectable(amb->name().c_str(), amb->name() == current))
                    env_ambient = amb;
            }
            ImGui::EndCombo();
        }
        ItemHelp("Name in configs: \n"
                 "CS/COP: ambient\n"
                 "   SOC: env_ambient");

        ImGui::ColorEdit3("ambient color", reinterpret_cast<float*>(&ambient));
        ItemHelp("Name in configs: \n"
                 "CS/COP: ambient_color\n"
                 "   SOC: ambient");
    }
    if (ImGui::CollapsingHeader("fog##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("far plane", &far_plane);
        ImGui::ColorEdit3("fog color", reinterpret_cast<float*>(&fog_color));
        ImGui::DragFloat("fog distance", &fog_distance, 1.0f, 0.0f, far_plane);
        ItemHelp("Fog distance can't be higher than far plane.");
        ImGui::DragFloat("fog density", &fog_density, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("water intensity", &m_fWaterIntensity, 0.001f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("rain##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("rain color", reinterpret_cast<float*>(&rain_color));
        ImGui::DragFloat("rain density", &rain_density, 0.001f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("thunderbolts##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("thunderbolts", thunderbolt ? thunderbolt->section.c_str() : ""))
        {
            if (ImGui::Selectable("<edit>", false))
            {
                window_thunderbolts = true;
            }
            if (ImGui::Selectable("##", !thunderbolt))
                thunderbolt = nullptr;
            for (const auto& bolts : env.eff_Thunderbolt->GetCollections())
            {
                if (ImGui::Selectable(bolts->section.c_str(), bolts == thunderbolt))
                    thunderbolt = bolts;
            }
            ImGui::EndCombo();
        }
        ItemHelp("Name in configs: \n"
                 "CS/COP: thunderbolts_collection\n"
                 "   SOC: thunderbolt");

        ImGui::DragFloat("duration", &bolt_duration);
        ImGui::DragFloat("period", &bolt_period);
    }
    if (ImGui::CollapsingHeader("wind##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float direction = rad2deg(wind_direction);
        if (ImGui::DragFloat("wind direction", &direction, 0.5f, -360.0f, 360.f))
            wind_direction = deg2rad(direction);

        ImGui::DragFloat("wind velocity", &wind_velocity, 1.0f, 0.0f, 1000.0f);
    }
    ImGui::PopID();
#endif
}

void CEnvDescriptorMixer::ed_show_params(const CEnvironment& env)
{
#ifndef MASTER_GOLD
    using namespace xray::imgui;

    CEnvDescriptor::ed_show_params(env);

    ImGui::PushID(m_identifier.c_str());
    if (ImGui::CollapsingHeader("mixer##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("environment color", reinterpret_cast<float*>(&env_color));
        ItemHelp("CS/COP: takes hemisphere color as the base value.\n"
                 "SOC   : takes sky color as the base value.");

        ImGui::DragFloat("weight", &weight, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("modifier power", &modif_power);
        ImGui::DragFloat("fog near", &fog_near);
        ImGui::DragFloat("fog far", &fog_far);
    }
    ImGui::PopID();
#endif
}

void CEffect_Thunderbolt::ED_ShowParams()
{
#ifndef MASTER_GOLD
    using namespace xray::imgui;

    float altitude[2] = { rad2deg(p_var_alt.x) , rad2deg(p_var_alt.y) };
    if (ImGui::DragFloat2("altitude", altitude, 0.5f, -360.0f, 360.f))
        p_var_alt = { rad2deg(altitude[0]) , rad2deg(altitude[1]) };

    float deltalongitude = rad2deg(p_var_long);
    if (ImGui::DragFloat("wind direction", &deltalongitude, 0.5f, -360.0f, 360.f))
        p_var_long = deg2rad(deltalongitude);

    ImGui::DragFloat("minimum distance factor", &p_min_dist, 0.001f, 0.0f, MAX_DIST_FACTOR);
    ItemHelp("Distance from far plane");

    float tilt = rad2deg(p_tilt);
    if (ImGui::DragFloat("tilt", &tilt, 0.01f, 15.0f, 30.f))
        p_tilt = deg2rad(tilt);

    ImGui::DragFloat("second probability", &p_second_prop, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("sky color", &p_sky_color, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("sun color", &p_sun_color, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("fog color", &p_fog_color, 0.001f, 0.0f, 1.0f);
#endif
}

void CEnvironment::on_tool_frame()
{
#ifndef MASTER_GOLD
    if (!get_open_state())
        return;

    using namespace xray::imgui;

    if (ImGui::Begin(tool_name(), &get_open_state(), get_default_window_flags()))
    {
        if (ImGui::BeginMenuBar())
        {
            const bool paused = Device.Paused();
            if (ImGui::RadioButton("Pause", paused))
                Device.Pause(!paused, TRUE, TRUE, "editor query");

            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::Button("Reset all"))
                    ED_Reload();

                if (ImGui::Button("Save all"))
                    save();

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

        float time_factor = g_pGameLevel ? g_pGameLevel->GetEnvironmentTimeFactor() : fTimeFactor;

        if (ImGui::CollapsingHeader("Environment time", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float time = g_pGameLevel ? g_pGameLevel->GetEnvironmentGameDayTimeSec() : GetGameTime();

            u32 hours, minutes, seconds;
            SplitTime(time, hours, minutes, seconds);

            string128 temp;
            xr_sprintf(temp, "Current time: %02d:%02d:%02d###environment_time", hours, minutes, seconds);

            if (ImGui::SliderFloat(temp, &time, 0, DAY_LENGTH, "", ImGuiSliderFlags_NoInput))
            {
                Invalidate();
                if (g_pGameLevel)
                    g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(time * 1000.f), time_factor);
                SetGameTime(time, time_factor);
                lerp();
            }
            if (ImGui::DragFloat("Time factor", &time_factor, 1.f, 1.f, 1000.f))
            {
                if (g_pGameLevel)
                    g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(time * 1000.f), time_factor);
                SetGameTime(time, time_factor);
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
                        SetWeather(CurrentCycleName, true);
                }
                ItemHelp("Force change weather", false);
                ImGui::SameLine();

                if (ImGui::BeginCombo("Weather cycle", CurrentCycleName.c_str()))
                {
                    if (ImGui::Selectable("<edit>", false)) {}
                    for (const auto& [identifier, env_descriptors] : WeatherCycles)
                    {
                        if (ImGui::Selectable(identifier.c_str(), CurrentCycleName == identifier))
                            SetWeather(identifier, force_change_weather);
                    }
                    ImGui::EndCombo();
                }
                //ImGui::SameLine();
                if (ConfigStyleSelector("Weather config style", CurrentWeather->soc_style))
                    CurrentEnv.soc_style = CurrentWeather->soc_style;

                ImGui::TableNextColumn();
                if (ImGui::BeginCombo("Effect", IsWFXPlaying() ? CurrentWeatherName.c_str() : ""))
                {
                    if (ImGui::Selectable("<edit>", false)) {}

                    if (ImGui::Selectable("##", !IsWFXPlaying()))
                        StopWFX();

                    for (const auto& [identifier, env_descriptors] : WeatherFXs)
                    {
                        if (ImGui::Selectable(identifier.c_str(), CurrentWeatherName == identifier))
                        {
                            if (IsWFXPlaying())
                                StopWFX();
                            SetWeatherFX(identifier);
                        }
                    }
                    ImGui::EndCombo();
                }
                //ImGui::SameLine();
                ImGui::BeginDisabled(!IsWFXPlaying());
                if (ConfigStyleSelector("Effect config style", CurrentWeather->soc_style))
                    CurrentEnv.soc_style = CurrentWeather->soc_style;
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
                TimeFrameCombo("Time frame##current0", Current[0], CurrentWeather);

                ImGui::TableNextColumn();
                u32 hours, minutes, seconds;
                SplitTime(CurrentEnv.exec_time, hours, minutes, seconds);

                string128 temp;
                xr_sprintf(temp, "%02d:%02d:%02d###frame_time", hours, minutes, seconds);

                const float current0_time = Current[0] ? Current[0]->exec_time : CurrentEnv.exec_time;
                const float current1_time = Current[1] ? Current[1]->exec_time : CurrentEnv.exec_time;

                ImGui::BeginDisabled(!Current[0] || !Current[1]);
                if (ImGui::SliderFloat(temp, &CurrentEnv.exec_time,
                    current0_time, current1_time, "", ImGuiSliderFlags_NoInput))
                {
                    //Invalidate();
                    if (g_pGameLevel)
                        g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(CurrentEnv.exec_time * 1000.f), time_factor);
                    SetGameTime(CurrentEnv.exec_time, time_factor);
                    lerp();
                }
                ImGui::EndDisabled();

                ImGui::TableNextColumn();
                TimeFrameCombo("Time frame##current1", Current[1], CurrentWeather);

                ImGui::EndTable();
            }

            if (ImGui::BeginTable("Environment lerp", 3, flags))
            {
                ImGui::TableSetupColumn("current");
                ImGui::TableSetupColumn("blend");
                ImGui::TableSetupColumn("target");
                ImGui::TableHeadersRow();

                ImGui::TableNextColumn();
                if (Current[0])
                    Current[0]->ed_show_params(*this);
                else
                    ImGui::Text("Please, load a level or select time frame manually.");

                ImGui::TableNextColumn();
                CurrentEnv.ed_show_params(*this);

                ImGui::TableNextColumn();
                if (Current[1])
                    Current[1]->ed_show_params(*this);
                else
                    ImGui::Text("Please, load a level or select time frame manually.");

                ImGui::EndTable();
            } // if (ImGui::BeginTable("Environment lerp", 3, flags))
        } // if (ImGui::CollapsingHeader("Live edit", ImGuiTreeNodeFlags_DefaultOpen))
    }
    ImGui::End();
#endif
}

#ifndef MASTER_GOLD
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
#endif
