#include "stdafx.h"

#include "editor_base.h"
#include "editor_helper.h"

#include "Environment.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"

namespace xray::editor
{
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
    ImGui::LabelText("config style", "%s", descriptor.old_style ? "soc" : "cs/cop");

    if (ImGui::CollapsingHeader("sun##header", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("sun color", (float*)&descriptor.sun_color);
        ImGui::DragFloat("shafts intensity", &descriptor.m_fSunShaftsIntensity, 1.0f, 0.0f, 1.0f);

        bool result = false;
        float y, x;
        descriptor.sun_dir.getHP(y, x);
        result  = ImGui::DragFloat("altitude", &y, 1.0f, -360.0f, 360.f);
        result |= ImGui::DragFloat("longitude", &x, 1.0f, -360.0f, 360.f);
        if (result)
        {
            descriptor.sun_dir.setHP(deg2rad(y), deg2rad(x));
        }

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
    }
    if (ImGui::CollapsingHeader("hemisphere", ImGuiTreeNodeFlags_DefaultOpen))
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

        float rotation = rad2deg(descriptor.sky_rotation);
        if (ImGui::DragFloat("sky rotation", &rotation, 1.0f, -360.0f, 360.f))
            descriptor.sky_rotation = deg2rad(rotation);

        rotation = rad2deg(descriptor.clouds_rotation);
        if (ImGui::DragFloat("clouds rotation", &rotation, 1.0f, -360.0f, 360.f))
            descriptor.clouds_rotation = deg2rad(rotation);
    }
    if (ImGui::CollapsingHeader("clouds", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (TextureSelector("clouds texture", descriptor.clouds_texture_name))
            descriptor.on_device_create();

        ImGui::ColorEdit4("clouds color", (float*)&descriptor.clouds_color, ImGuiColorEditFlags_AlphaBar);
    }
    if (ImGui::CollapsingHeader("ambient##header", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("ambient color", (float*)&descriptor.ambient);
        if (ImGui::BeginCombo("ambient##env_ambient", descriptor.env_ambient ? descriptor.env_ambient->name().c_str() : "##"))
        {
            for (const auto& ambient : env.Ambients)
            {
                shared_str current = descriptor.env_ambient ? descriptor.env_ambient->name() : "";
                if (ImGui::Selectable(ambient->name().c_str(), ambient->name() == current))
                    descriptor.env_ambient = ambient;
            }
            ImGui::EndCombo();
        }
    }
    if (ImGui::CollapsingHeader("fog", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("fog color", (float*)&descriptor.fog_color);
        ImGui::DragFloat("far plane", &descriptor.far_plane);
        ImGui::DragFloat("distance", &descriptor.fog_distance, 1.0f, 0.0f, descriptor.far_plane);
        ImGui::DragFloat("density", &descriptor.fog_density, 1.0f, 0.0f, 1.0f);
        ImGui::DragFloat("water intensity", &descriptor.m_fWaterIntensity, 1.0f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("rain", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("rain color", (float*)&descriptor.rain_color);
        ImGui::DragFloat("rain density", &descriptor.rain_density, 1.0f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("thunderbolts", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginCombo("thunderbolts collection", descriptor.tb_id.c_str()))
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
        ImGui::DragFloat("duration", &descriptor.bolt_duration);
        ImGui::DragFloat("period", &descriptor.bolt_period);

    }
    if (ImGui::CollapsingHeader("wind", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float direction = rad2deg(descriptor.wind_direction);
        if (ImGui::DragFloat("wind direction", &direction, 1.0f, -360.0f, 360.f))
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
    if (ImGui::CollapsingHeader("mixer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("fog near", &descriptor.fog_near);
        ImGui::DragFloat("fog far", &descriptor.fog_far);
        ImGui::DragFloat("weight", &descriptor.weight, 1.0f, 0.0f, 1.0f);
        ImGui::DragFloat("modifier power", &descriptor.modif_power);
    }
    ImGui::PopID();
}

void ide::ShowWeatherEditor()
{
    if (ImGui::Begin("Weather editor", &m_windows.weather, get_default_window_flags()))
    {
        auto& env = g_pGamePersistent->Environment();

        if (ImGui::BeginCombo("current weather cycle", env.CurrentWeatherName.c_str()))
        {
            for (const auto& [identifier, env_descriptors] : env.WeatherCycles)
            {
                if (ImGui::Selectable(identifier.c_str(), env.CurrentWeatherName == identifier))
                    ;
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
