#include "StdAfx.h"


#ifndef MASTER_GOLD
#include "xrEngine/editor_helper.h"

#include "object_factory.h"
#include "object_factory_spawner.h"

#include "alife_smart_terrain_registry.h"

extern CSE_Abstract* CALifeSimulator__spawn_item2(CALifeSimulator* self,
    pcstr section, const Fvector& position, u32 level_vertex_id,
    GameGraph::_GRAPH_ID game_vertex_id, ALife::_OBJECT_ID id_parent);

void CObjectFactory::init_spawn_data()
{
    using namespace xray;

    for (CInifile::Sect* section : pSettings->sections())
    {
        const auto& name = section->Name;
        if (name.empty())
            continue;

        if (!section->line_exist("class"))
            continue;

        const auto clsid = pSettings->r_clsid(name, "class");
        if (!item(clsid, true))
            continue;

        const auto kind    = pSettings->read_if_exists<pcstr>(name, "kind", nullptr);
        const auto wpclass = pSettings->read_if_exists<pcstr>(name, "weapon_class", nullptr);

        SpawnCategory category = try_detect_spawn_category(kind, wpclass, clsid);

        if (category >= SpawnCategory::Artefacts && category <= SpawnCategory::WeaponsMiscellaneous)
        {
            const auto width = pSettings->read_if_exists<u32>(name, "inv_grid_width", 0);
            const auto height = pSettings->read_if_exists<u32>(name, "inv_grid_height", 0);

            if (width == 0 && height == 0) // ban fake items
                category = SpawnCategory::Unknown;
            else if (pSettings->read_if_exists<bool>(name, "quest_item", false))
                category = SpawnCategory::ItemsQuest;
        }

        if (category >= SpawnCategory::WeaponsAmmo && category <= SpawnCategory::WeaponsExplosives)
        {
            cpcstr parent_section = pSettings->read_if_exists<pcstr>(name, "parent_section", name.c_str());
            if (parent_section != name)
                category = SpawnCategory::Unknown;
        }
        else if (category == SpawnCategory::SquadsStalkers)
        {
            category = SpawnCategory::Unknown;

            xr_string temp;
            cpcstr npc        = pSettings->read_if_exists<pcstr>(name, "npc", nullptr);
            cpcstr npc_random = pSettings->read_if_exists<pcstr>(name, "npc_random", "");
            _GetItem(npc ? npc : npc_random, 0, temp);

            if (!temp.empty())
            {
                const auto npc_clsid    = pSettings->r_clsid(temp.c_str(), "class");
                const auto npc_kind     = pSettings->read_if_exists<pcstr>(temp.c_str(), "kind", nullptr);
                const auto npc_category = try_detect_spawn_category(npc_kind, nullptr, npc_clsid);

                if (npc_category == SpawnCategory::CreaturesStalkers)
                    category = SpawnCategory::SquadsStalkers;
                else if (npc_category == SpawnCategory::CreaturesMutants)
                    category = SpawnCategory::SquadsMutants;
            }
        }
        else if (category == SpawnCategory::Vehicles || category == SpawnCategory::Physics)
        {
            if (!section->line_exist("visual"))
                category = SpawnCategory::Unknown;
        }

        if (category == SpawnCategory::Unknown)
            continue;

        m_spawner_sections[static_cast<size_t>(category)].emplace_back(section);
    }
}

static void print_ini_values(const CInifile::Sect* section, std::initializer_list<pcstr> list_of_interest = {})
{
    if (ImGui::BeginTable("section values", 3))
    {
        for (cpcstr name : list_of_interest)
        {
            if (!pSettings->line_exist(section->Name, name))
                continue;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("=");
            ImGui::TableSetColumnIndex(2);
            cpcstr value = pSettings->r_string(section->Name, name);
            ImGui::Text("%s", value ? value : "");
        }
        if (std::empty(list_of_interest))
        {
            for (const auto& [name, value] : section->Data)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("=");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", !value.empty() ? value.c_str() : "");
            }
        }
        ImGui::EndTable();
    }
}

static void print_npc(const CInifile::Sect* section)
{
    print_ini_values(section,
    {
        "$spawn",
        "character_profile",
        "spec_rank",
        "community",
        "story_id",
        "custom_data",
    });
}

static void print_sim_squad_scripted(const CInifile::Sect* section)
{
    print_ini_values(section,
    {
        "faction",
        "behaviour",
        "common",
        "relationship",
        "sympathy",
        "invulnerability",
        "spawn_point",
        "target_smart",
        "target_random_smart",
        "on_death",
        "item_on_all",
        "rush",
        "idle_time",
        "show_spot",
        "always_walk",
        "always_arrived",
        "npc_in_squad",
        "npc",
        "npc_random",
        "story_id",
    });
}

void CObjectFactory::on_tool_frame()
{
    using namespace xray;

    if (!get_open_state())
        return;

    if (!ImGui::Begin(tool_name(), &get_open_state(), get_default_window_flags() & ~ImGuiWindowFlags_MenuBar))
    {
        ImGui::End();
        return;
    }

    enum class Spawn : u8
    {
        NearActor,
        ToInventory,
        OnSmart,
    };

    pcstr spawn_section{};

    static auto  spawn_category{ SpawnCategory::Artefacts };
    static int   spawn_amount{ 1 };
    static Spawn where_to_spawn{ Spawn::NearActor };

    static const GameGraph::SLevel* selected_level{};
    static const CSE_ALifeSmartZone* selected_smart{};

    auto window_size = ImGui::GetContentRegionAvail();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
    ImGuiChildFlags child_flags = ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_NavFlattened;

    // 1. Display available items categories
    if (ImGui::BeginChild("Spawn categories", { 0, window_size.y }, child_flags | ImGuiChildFlags_AutoResizeX, window_flags))
    {
        if (ImGui::BeginMenuBar())
        {
            ImGui::Text("Spawn categories");
            ImGui::EndMenuBar();
        }

        for (u8 i = 0; i < CATEGORIES_COUNT; ++i)
        {
            if (m_spawner_sections[i].empty())
                continue;

            const auto category = static_cast<SpawnCategory>(i);

            cpcstr name = spawn_category_to_text(category);

            if (ImGui::Selectable(name, category == spawn_category))
                spawn_category = category;
        }
    }
    window_size = ImGui::GetWindowSize(); // order dependent, don't move this line anywhere
    ImGui::EndChild();
    ImGui::SameLine();

    window_size.x = std::max(window_size.x, ImGui::GetContentRegionAvail().x / 3.0f);

    const auto& sections = m_spawner_sections[u8(spawn_category)];

    // 2. Display the list of items available for spawning
    if (ImGui::BeginChild("Sections list", window_size, child_flags, window_flags))
    {
        enum DisplayMode : int
        {
            DisplayGameNames,
            DisplayLTXSections,
        };
        static int display_mode = DisplayLTXSections;

        if (ImGui::BeginMenuBar())
        {
            ImGui::Text("Available: %zu", sections.size());

            constexpr pcstr styles[] =
            {
                "Game",
                "LTX",
            };

            ImGui::SetNextItemWidth(ImGui::CalcTextSize(" Game ").x); // shrink to minimal
            imgui::Selector("", display_mode, styles, std::size(styles),
                            "Left-click on the item in this list to spawn it.\n"
                            "You can also select between displaying game names or ltx sections by using a switch.");
            ImGui::EndMenuBar();
        }

        for (const CInifile::Sect* section : sections)
        {
            xr_string name;
            switch (static_cast<DisplayMode>(display_mode))
            {
            case DisplayGameNames:
            {
                const std::locale locale("");
                if (cpcstr inv_name = pSettings->read_if_exists<pcstr>(section->Name.c_str(), "inv_name", nullptr))
                {
                    const auto translated = StringTable().translate(inv_name);
                    name = StringToUTF8(translated.c_str(), locale);
                    break;
                }
                if (cpcstr character_profile = pSettings->read_if_exists<pcstr>(section->Name.c_str(), "character_profile", nullptr))
                {
                    if (CSpecificCharacter::GetById(character_profile, true))
                    {
                        CSpecificCharacter character;
                        character.Load(character_profile);
                        cpcstr character_name = character.Name();
                        if (character_name[0] && !strstr(character_name, "GENERATE_NAME"))
                        {
                            const auto translated = StringTable().translate(character_name);
                            name = StringToUTF8(translated.c_str(), locale);
                            break;
                        }
                    }
                }
                [[fallthrough]];
            }
            default:
            case DisplayLTXSections:
                name = section->Name.c_str();
                break;
            } // switch (static_cast<DisplayMode>(display_mode))

            ImGui::PushID(section->Name.c_str());
            if (ImGui::Button(name.c_str()))
                spawn_section = section->Name.c_str();
            ImGui::PopID();

            ImGui::SetNextWindowSizeConstraints({}, { 600.f, 738.f });
            if (ImGui::BeginPopupContextItem())
            {
                ImGui::Text("[%s]", section->Name.c_str());
                print_ini_values(section);
                ImGui::EndPopup();
            }

            ImGui::SetNextWindowSizeConstraints({}, { 600.f, 738.f });
            if (ImGui::BeginItemTooltip())
            {
                ImGui::Text("[%s]", section->Name.c_str());

                switch (spawn_category)
                {
                case SpawnCategory::CreaturesStalkers:
                case SpawnCategory::CreaturesMutants:
                    print_npc(section);
                    break;
                case SpawnCategory::SquadsStalkers:
                case SpawnCategory::SquadsMutants:
                    print_sim_squad_scripted(section);
                    break;
                } // switch (spawn_category)

                ImGui::BeginDisabled();
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Text("Right-click to see all config values.");
                ImGui::EndDisabled();
                ImGui::EndTooltip();
            }
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    window_size = ImGui::GetContentRegionAvail();

    // 3. Display spawn options
    if (ImGui::BeginChild("Spawn", window_size, child_flags, window_flags))
    {
        // 3.1. Draw the header options
        if (ImGui::BeginMenuBar())
        {
            ImGui::Text("Spawn");

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);

            ImGui::SetNextItemWidth(ImGui::CalcTextSize("100").x); // shrink to minimal
            ImGui::DragInt("", &spawn_amount, 0.5f, 1, 100);

            ImGui::PopStyleVar();

            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar("Where to spawn", tab_bar_flags))
            {
                if (ImGui::BeginTabItem("near player"))
                {
                    where_to_spawn = Spawn::NearActor;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("to inventory"))
                {
                    where_to_spawn = Spawn::ToInventory;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("on smart"))
                {
                    where_to_spawn = Spawn::OnSmart;
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            imgui::ItemHelp("Select the amount of items to spawn and where you want to spawn them.");
            ImGui::EndMenuBar();
        }

        switch (where_to_spawn)
        {
        case Spawn::NearActor:
        case Spawn::ToInventory:
        {
            ImGui::PushStyleColor(ImGuiCol_Button, 0);

            const auto tex = InventoryUtilities::GetEquipmentIconsShader()->GetImGuiTextureId();

            for (const CInifile::Sect* section : sections)
            {
                const float w = pSettings->read_if_exists<float>(section->Name, "inv_grid_width", 0) * INV_GRID_WIDTH;
                const float h = pSettings->read_if_exists<float>(section->Name, "inv_grid_height", 0) * INV_GRID_HEIGHT;

                if (fis_zero(w) || fis_zero(h))
                    continue;

                const float x = pSettings->r_float(section->Name, "inv_grid_x") * INV_GRID_WIDTH;
                const float y = pSettings->r_float(section->Name, "inv_grid_y") * INV_GRID_HEIGHT;

                const bool spawn = ImGui::ImageButton(section->Name.c_str(), tex.texture, { w , h },
                    { x / tex.size.x, y / tex.size.y }, { (x + w) / tex.size.x, (y + h) / tex.size.y });
                if (spawn)
                    spawn_section = section->Name.c_str();
            }
            ImGui::PopStyleColor();
            break;
        }
        case Spawn::OnSmart:
        {
            auto* alife = ai().get_alife();
            const auto* game_graph = ai().get_game_graph();

            if (!alife || !game_graph)
            {
                ImGui::TextWrapped("A-Life should be loaded for this spawn method to work.");
                break;
            }

            // Get current level using the level graph, if nothing selected
            if (!selected_level && ai().get_level_graph())
            {
                selected_level = &game_graph->header().level(ai().level_graph().level_id());
            }

            // Display levels list
            if (ImGui::BeginCombo("Level", selected_level ? selected_level->name().c_str() : ""))
            {
                for (const auto& [_, level] : game_graph->header().levels())
                {
                    if (ImGui::Selectable(level.name().c_str(), &level == selected_level))
                        selected_level = &level;
                }
                ImGui::EndCombo();
            }

            // Select nearest to camera smart, if nothing selected
            if (!selected_smart)
            {
                const auto& smarts = alife->smart_terrains().objects();
                const auto it = std::min_element(smarts.begin(), smarts.end(), [](const auto& left, const auto& right)
                {
                    if (!left.second->m_bOnline)
                        return false;

                    const float d1 = left.second->Position().distance_to_sqr(Device.vCameraPosition);
                    const float d2 = right.second->Position().distance_to_sqr(Device.vCameraPosition);
                    return d1 < d2;
                });
                if (it != smarts.end() && it->second->m_bOnline)
                    selected_smart = it->second;
            }
            // Re-select smart terrain if we just changed the selected level
            else if (game_graph->vertex(selected_smart->m_tGraphID)->level_id() != selected_level->id())
            {
                for (const auto& [_, smart] : alife->smart_terrains().objects())
                {
                    const auto* game_vertex = game_graph->vertex(smart->m_tGraphID);
                    if (game_vertex->level_id() == selected_level->id())
                    {
                        selected_smart = smart;
                        break;
                    }
                }
            }

            // Display smart list
            if (ImGui::BeginCombo("Smart", selected_smart ? selected_smart->name_replace() : ""))
            {
                for (const auto& [_, smart] : alife->smart_terrains().objects())
                {
                    const auto* game_vertex = game_graph->vertex(smart->m_tGraphID);
                    if (game_vertex->level_id() != selected_level->id())
                        continue;
                    if (ImGui::Selectable(smart->name_replace(), smart == selected_smart))
                        selected_smart = smart;
                }
                ImGui::EndCombo();
            }
            break;
        }
        } // switch (where_to_spawn)

        if (spawn_category == SpawnCategory::SquadsStalkers || spawn_category == SpawnCategory::SquadsMutants)
        {
            ImGui::TextWrapped("Squads are not supported for spawning right now.");
        }
    }
    ImGui::EndChild();

    // Finita la comedia.
    if (spawn_section && g_pGameLevel)
    {
        auto entity = Level().CurrentViewEntity();
        if (!entity)
            entity = Level().CurrentControlEntity();

        Fvector pos;
        u32  level_vertex_id = u32(-1);
        auto game_vertex_id  = GameGraph::_GRAPH_ID(-1);
        auto parent_id       = ALife::_OBJECT_ID(-1);

        switch (where_to_spawn)
        {
        case Spawn::ToInventory:
            if (entity && spawn_category <= SpawnCategory::WeaponsMiscellaneous)
                parent_id = entity->ID();
            [[fallthrough]];

        case Spawn::NearActor:
            if (entity)
            {
                pos = entity->Position();
                level_vertex_id = entity->ai_location().level_vertex_id();
                game_vertex_id = entity->ai_location().game_vertex_id();
            }
            else if (ai().get_level_graph() && ai().get_cross_table())
            {
                pos = Device.vCameraPosition;
                level_vertex_id = ai().level_graph().vertex_id(pos);
                game_vertex_id = ai().cross_table().vertex(level_vertex_id).game_vertex_id();
            }
            break;

        case Spawn::OnSmart:
            if (selected_smart)
            {
                pos = selected_smart->Position();
                level_vertex_id = selected_smart->m_tNodeID;
                game_vertex_id = selected_smart->m_tGraphID;
            }
            break;
        }

        auto* alife = const_cast<CALifeSimulator*>(ai().get_alife());

        for (int i = 0; i < spawn_amount; i++)
        {
            if (spawn_category == SpawnCategory::SquadsStalkers || spawn_category == SpawnCategory::SquadsMutants)
            {

            }
            else if (!alife || spawn_category == SpawnCategory::CreaturesPhantoms)
            {
                auto* abstract = Level().spawn_item(spawn_section, pos, level_vertex_id, parent_id, true);

                if (auto* anomaly = abstract ? abstract->cast_anomalous_zone() : nullptr)
                {
                    CShapeData::shape_def shape
                    {
                        .type = CShapeData::cfSphere,
                        .data = { Fsphere{ .P = {}, .R = 3.0f } },
                    };
                    anomaly->assign_shapes(&shape, 1);
                }

                NET_Packet P;
                abstract->Spawn_Write(P, TRUE); // Not sure if it should be true or false (i.e. local or not)
                Level().Send(P, net_flags(TRUE));
                F_entity_Destroy(abstract);
            }
            else if (level_vertex_id != u32(-1) && game_vertex_id != GameGraph::_GRAPH_ID(-1))
            {
                auto* abstract = CALifeSimulator__spawn_item2(alife, spawn_section, pos,
                                                             level_vertex_id, game_vertex_id, parent_id);

                if (auto* anomaly = abstract ? abstract->cast_anomalous_zone() : nullptr)
                {
                    CShapeData::shape_def shape
                    {
                        .type = CShapeData::cfSphere,
                        .data = { Fsphere{ .P = {}, .R = 3.0f } },
                    };
                    anomaly->assign_shapes(&shape, 1);
                }
            }
        }
    }

    ImGui::End();
}
#endif
