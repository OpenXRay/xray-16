#include "pch.hpp"

#include "editor_environment_effects_manager.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "property_collection.hpp"
#include "editor_environment_effects_effect.hpp"
#include "editor_environment_detail.hpp"

using effects_manager = editor::environment::effects::manager;

template <>
void property_collection<effects_manager::effect_container_type, effects_manager>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    xr_strcpy(buffer, buffer_size, m_container[item_index]->id());
}

template <>
XRay::Editor::property_holder_base* property_collection<
    effects_manager::effect_container_type, effects_manager>::create()
{
    using editor::environment::effects::effect;

    auto* object = xr_new<effect>(m_holder, generate_unique_id("effect_unique_id_").c_str());
    object->fill(this);
    return (object->object());
}

namespace editor::environment::effects
{
manager::manager(::editor::environment::manager* environment)
    : m_environment(*environment), m_collection(0), m_changed(true)
{
    m_collection = xr_new<collection_type>(&m_effects, this, &m_changed);
}

manager::~manager()
{
    xr_delete(m_collection);
    delete_data(m_effects_ids);
}

void manager::load()
{
    string_path file_name;
    CInifile* config = xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "effects.ltx"), true, true, false);

    VERIFY(m_effects.empty());

    typedef CInifile::Root sections_type;
    sections_type& sections = config->sections();
    m_effects.reserve(sections.size());

    for (const auto &i : sections)
    {
        auto* object = xr_new<effect>(*this, i->Name);
        object->load(*config);
        object->fill(m_collection);
        m_effects.push_back(object);
    }

    xr_delete(config);
}

void manager::save()
{
    string_path file_name;
    CInifile* config = xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "effects.ltx"), false, false, true);

    for (const auto &i : m_effects)
        i->save(*config);

    xr_delete(config);
}

void manager::fill(XRay::Editor::property_holder_base* holder)
{
    VERIFY(holder);
    holder->add_property("effects", "ambients", "this option is responsible for effects", m_collection);
}

manager::effects_ids_type const& manager::effects_ids() const
{
    if (!m_changed)
        return (m_effects_ids);

    m_changed = false;

    delete_data(m_effects_ids);

    m_effects_ids.resize(m_effects.size());

    auto j = m_effects_ids.begin();
    for (const auto &i : m_effects)
        *j++ = xr_strdup(i->id());

    std::sort(m_effects_ids.begin(), m_effects_ids.end(), detail::logical_string_predicate());

    return (m_effects_ids);
}

shared_str manager::unique_id(shared_str const& id) const
{
    if (m_collection->unique_id(id.c_str()))
        return (id);

    return (m_collection->generate_unique_id(id.c_str()));
}
} // namespace editor::environment::effects
