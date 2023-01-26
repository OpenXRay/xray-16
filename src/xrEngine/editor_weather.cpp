#include "stdafx.h"

#include "editor_base.h"
#include "editor_helper.h"

#include "Environment.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"

namespace xray::editor
{
static void ItemHelp(const char* desc, bool use_separate_marker = true)
{
    if (use_separate_marker)
        ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool TextureSelector(pcstr label, shared_str& texture_name)
{
    bool changed = false;
    string_path temp;
    xr_strcpy(temp, texture_name.empty() ? "" : texture_name.c_str());

    if (ImGui::InputText(label, temp, std::size(temp)))
    {
        texture_name = temp;
        changed = true;
    }

    return changed;
}

template <typename T>
void display_property(T&) {}

template <>
void display_property(CEnvDescriptor& descriptor)
{
    auto& env = g_pGamePersistent->Environment();

    ImGui::PushID(descriptor.m_identifier.c_str());

    ImGui::LabelText("identifier", "%s", descriptor.m_identifier.c_str());
    {
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
        int selected = descriptor.old_style ? ConfigStyle_SOC : ConfigStyle_CSCOP;
        if (ImGui::SliderInt("config style", &selected, 0, ConfigStyle_COUNT - 1, styles[selected]))
        {
            switch ((ConfigStyle)selected)
            {
                case ConfigStyle_SOC:   descriptor.old_style = true; break;
                case ConfigStyle_CSCOP: descriptor.old_style = false; break;
            }
        }
        ImGui::SameLine();
        ItemHelp("This affects how environment color will be calculated in the mixer. "
                 "(look for details in the environment color help marker in the mixer category)");
    }

    if (ImGui::CollapsingHeader("sun##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("sun##lensflareid", descriptor.lens_flare_id.c_str()))
        {
            if (ImGui::Selectable("##", descriptor.lens_flare_id.empty()))
                descriptor.lens_flare_id = "";
            for (const auto& section : env.m_suns_config->sections())
            {
                if (ImGui::Selectable(section->Name.c_str(), section->Name == descriptor.lens_flare_id))
                    descriptor.lens_flare_id = section->Name;
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ItemHelp("Name in configs: \n"
            "CS/COP: sun\n"
            "   SOC: flares");

        ImGui::ColorEdit3("sun color", (float*)&descriptor.sun_color);

        bool result = false;
        float y, x;
        descriptor.sun_dir.getHP(y, x);
        result  = ImGui::DragFloat("altitude", &y, 0.5f, -360.0f, 360.f);
        result |= ImGui::DragFloat("longitude", &x, 0.5f, -360.0f, 360.f);
        if (result)
        {
            descriptor.sun_dir.setHP(deg2rad(y), deg2rad(x));
        }

        float azimuth = rad2deg(descriptor.sun_dir_azimuth);
        if (ImGui::DragFloat("azimuth", &azimuth, 0.5f, -360.0f, 360.f))
            descriptor.sun_dir_azimuth = deg2rad(azimuth);
        ImGui::SameLine();
        ItemHelp("Dynamic sun dir azimuth correction.\n"
                 "Name in configs: \n"
                 "sun_dir_azimuth");

        ImGui::DragFloat("shafts intensity", &descriptor.m_fSunShaftsIntensity, 0.001f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("hemisphere##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (TextureSelector("sky texture", descriptor.sky_texture_name))
        {
            string_path temp;
            strconcat(temp, descriptor.sky_texture_name.c_str(), "#small");
            descriptor.sky_texture_env_name = temp;

            descriptor.on_device_create();
        }

        ImGui::ColorEdit3("sky color", (float*)&descriptor.sky_color);
        ImGui::ColorEdit4("hemi color", (float*)&descriptor.hemi_color, ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine();
        ItemHelp("Name in configs: \n"
                 "hemisphere_color");

        float rotation = rad2deg(descriptor.sky_rotation);
        if (ImGui::DragFloat("sky rotation", &rotation, 0.5f, -360.0f, 360.f))
            descriptor.sky_rotation = deg2rad(rotation);
    }
    if (ImGui::CollapsingHeader("clouds##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (TextureSelector("clouds texture", descriptor.clouds_texture_name))
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
            for (const auto& ambient : env.Ambients)
            {
                shared_str current = descriptor.env_ambient ? descriptor.env_ambient->name() : "";
                if (ImGui::Selectable(ambient->name().c_str(), ambient->name() == current))
                    descriptor.env_ambient = ambient;
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ItemHelp("Name in configs: \n"
            "CS/COP: ambient\n"
            "   SOC: env_ambient");

        ImGui::ColorEdit3("ambient color", (float*)&descriptor.ambient);
        ImGui::SameLine();
        ItemHelp("Name in configs: \n"
                 "CS/COP: ambient_color\n"
                 "   SOC: ambient");
    }
    if (ImGui::CollapsingHeader("fog##category", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("far plane", &descriptor.far_plane);
        ImGui::ColorEdit3("fog color", (float*)&descriptor.fog_color);
        ImGui::DragFloat("fog distance", &descriptor.fog_distance, 1.0f, 0.0f, descriptor.far_plane);
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
        if (ImGui::BeginCombo("thunderbolts", descriptor.tb_id.c_str()))
        {
            if (ImGui::Selectable("##", descriptor.tb_id.empty()))
                descriptor.tb_id = "";
            for (const auto& section : env.m_thunderbolt_collections_config->sections())
            {
                if (ImGui::Selectable(section->Name.c_str(), section->Name == descriptor.tb_id))
                    descriptor.tb_id = section->Name;
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
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
        ImGui::SameLine();
        ItemHelp("CS/COP: takes hemisphere color as the base value.\n"
                 "SOC   : takes sky color as the base value.");

        ImGui::DragFloat("weight", &descriptor.weight, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("modifier power", &descriptor.modif_power);
        ImGui::DragFloat("fog near", &descriptor.fog_near);
        ImGui::DragFloat("fog far", &descriptor.fog_far);
    }
    ImGui::PopID();
}

void ide::ShowWeatherEditor()
{
    if (ImGui::Begin("Weather editor", &m_windows.weather, get_default_window_flags()))
    {
        auto& env = g_pGamePersistent->Environment();

        static bool force_change_weather = true;
        ImGui::Checkbox("##", &force_change_weather);
        const auto prev_size = ImGui::GetItemRectSize();
        ImGui::SameLine();
        ItemHelp("Force change weather", false);
        ImGui::SameLine();

        //ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - prev_size.x);
        if (ImGui::BeginCombo("current weather cycle", env.CurrentCycleName.c_str()))
        {
            for (const auto& [identifier, env_descriptors] : env.WeatherCycles)
            {
                if (ImGui::Selectable(identifier.c_str(), env.CurrentCycleName == identifier))
                    env.SetWeather(identifier, force_change_weather);
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("current weather fx cycle", env.bWFX ? env.CurrentWeatherName.c_str() : ""))
        {
            if (ImGui::Selectable("##", !env.bWFX))
                env.StopWFX();

            for (const auto& [identifier, env_descriptors] : env.WeatherFXs)
            {
                if (ImGui::Selectable(identifier.c_str(), env.CurrentWeatherName == identifier))
                {
                    if (env.bWFX)
                        env.StopWFX();
                    env.SetWeatherFX(identifier);
                }
            }
            ImGui::EndCombo();
        }

        auto& current = *env.CurrentEnv;
        auto& current0 = env.Current[0] ? *env.Current[0] : *env.CurrentEnv;
        auto& current1 = env.Current[1] ? *env.Current[1] : *env.CurrentEnv;

        if (ImGui::BeginCombo("current time frame", current0.m_identifier.c_str()))
        {
            for (const auto& descriptor : *env.CurrentWeather)
            {
                if (ImGui::Selectable(descriptor->m_identifier.c_str(), descriptor->m_identifier == current0.m_identifier))
                    ;
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Save"))
            env.save(current.old_style);

        constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner;

        if (ImGui::BeginTable("time lerp", 3, flags))
        {
            ImGui::TableSetupColumn("current");
            ImGui::TableSetupColumn("blend");
            ImGui::TableSetupColumn("target");
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            display_property(current0);
            ImGui::TableNextColumn();
            display_property(current);
            ImGui::TableNextColumn();
            display_property(current1);
            ImGui::EndTable();
        }
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
