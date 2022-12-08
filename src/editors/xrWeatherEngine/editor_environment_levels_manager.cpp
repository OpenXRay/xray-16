////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_levels_manager.cpp
// Created : 28.12.2007
// Modified : 28.12.2007
// Author : Dmitriy Iassenev
// Description : editor environment levels manager class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

#include "editor_environment_levels_manager.hpp"
#include "editor_environment_weathers_manager.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "Include/editor/ide.hpp"
#include "ide.hpp"

namespace editor::environment::levels
{
static pcstr s_default_weather_id = "[default]";
static pcstr s_level_section_id = "levels";

manager::manager(::editor::environment::weathers::manager* weathers) : m_weathers(*weathers), m_property_holder(0) {}
manager::~manager()
{
    VERIFY(m_config_single);
    CInifile::Destroy(m_config_single);
    m_config_single = 0;

    VERIFY(m_config_mp);
    CInifile::Destroy(m_config_mp);
    m_config_mp = 0;

    if (!Device.editor())
        return;

    ::ide().destroy(m_property_holder);
}

void manager::fill_levels(CInifile& config, pcstr section, pcstr category)
{
    for (const auto &i : config.r_section(section).Data)
    {
        if (!i.first.size())
            continue;

        VERIFY(config.section_exist(i.first));
        if (!config.line_exist(i.first, "weathers"))
        {
            m_levels.emplace(i.first.c_str(), std::make_pair(category, s_default_weather_id));
            continue;
        }

        pcstr weather_id = config.r_string(i.first, "weathers");
        m_levels.emplace(i.first.c_str(), std::make_pair(category, weather_id));
    }
}

void manager::load()
{
    string_path file_name;

    // Обратите внимание: данные файлы будут перезаписаны при закрытии.
    // Также: комментарии будут удалены и все секции отсортированы.
    m_config_single = CInifile::Create(FS.update_path(file_name, "$game_config$", "game_maps_single.ltx"), false);
    m_config_mp     = CInifile::Create(FS.update_path(file_name, "$game_config$", "game_maps_mp.ltx"    ), false);

    VERIFY(m_levels.empty());
    fill_levels(*m_config_single, "level_maps_single", "single");
    fill_levels(*m_config_mp    , "level_maps_mp"    , "multiplayer");
}

pcstr const* manager::collection() { return (&*m_weathers.weather_ids().begin()); }
u32 manager::collection_size() { return (m_weathers.weather_ids().size()); }
void manager::fill()
{
    VERIFY(!m_property_holder);
    m_property_holder = ::ide().create_property_holder("levels");

    typedef XRay::Editor::property_holder_base::string_collection_getter_type collection_getter_type;
    collection_getter_type collection_getter;
    collection_getter.bind(this, &manager::collection);

    typedef XRay::Editor::property_holder_base::string_collection_size_getter_type collection_size_getter_type;
    collection_size_getter_type collection_size_getter;
    collection_size_getter.bind(this, &manager::collection_size);

    for (auto &i : m_levels)
    {
        string_path description;
        xr_strcpy(description, "weather for level ");
        xr_strcat(description, i.first.c_str());
        m_property_holder->add_property(i.first.c_str(), i.second.first, description, i.second.second.c_str(),
            i.second.second, collection_getter, collection_size_getter,
            XRay::Editor::property_holder_base::value_editor_combo_box, XRay::Editor::property_holder_base::cannot_enter_text);
    }

    ::ide().environment_levels(m_property_holder);
}
} // namespace editor::environment::levels

