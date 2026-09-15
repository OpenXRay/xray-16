#include "stdafx.h"
#include "ParticleEditor.h"
#include "Layers/xrRender/ParticleEffectDef.h"
#include "Layers/xrRender/ParticleGroup.h"
#include "Layers/xrRender/GpuParticleManager.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace xray::render::fg
{
namespace
{
struct NamedFlag
{
    const char* name;
    uint32_t mask;
};

constexpr NamedFlag effectFlags[] = {
    {"Sprite (dfSprite)", PS::CPEDef::dfSprite},
    {"Framed (dfFramed)", PS::CPEDef::dfFramed},
    {"Animated (dfAnimated)", PS::CPEDef::dfAnimated},
    {"Random frame (dfRandomFrame)", PS::CPEDef::dfRandomFrame},
    {"Random playback (dfRandomPlayback)", PS::CPEDef::dfRandomPlayback},
    {"Time limit (dfTimeLimit)", PS::CPEDef::dfTimeLimit},
    {"Align to path (dfAlignToPath)", PS::CPEDef::dfAlignToPath},
    {"Collision (dfCollision)", PS::CPEDef::dfCollision},
    {"Delete on collision (dfCollisionDel)", PS::CPEDef::dfCollisionDel},
    {"Velocity scale (dfVelocityScale)", PS::CPEDef::dfVelocityScale},
    {"Dynamic collision (dfCollisionDyn)", PS::CPEDef::dfCollisionDyn},
    {"World align (dfWorldAlign)", PS::CPEDef::dfWorldAlign},
    {"Face align (dfFaceAlign)", PS::CPEDef::dfFaceAlign},
    {"Culling (dfCulling)", PS::CPEDef::dfCulling},
    {"Cull counterclockwise (dfCullCCW)", PS::CPEDef::dfCullCCW}
};

constexpr NamedFlag groupEffectFlags[] = {
    {"Deferred stop (flDefferedStop)", PS::CPGDef::SEffect::flDefferedStop},
    {"On-play child (flOnPlayChild)", PS::CPGDef::SEffect::flOnPlayChild},
    {"Enabled (flEnabled)", PS::CPGDef::SEffect::flEnabled},
    {"Rewind on-play child (flOnPlayChildRewind)", PS::CPGDef::SEffect::flOnPlayChildRewind},
    {"On-birth child (flOnBirthChild)", PS::CPGDef::SEffect::flOnBirthChild},
    {"On-death child (flOnDeadChild)", PS::CPGDef::SEffect::flOnDeadChild}
};

bool HasLevel()
{
    return g_pGameLevel && g_pGameLevel->bReady;
}

bool ContainsInsensitive(const std::string& text, const char* filter)
{
    if (!*filter)
        return true;
    const char* end = filter + std::char_traits<char>::length(filter);
    return std::search(text.begin(), text.end(), filter, end, [](unsigned char left, unsigned char right)
    {
        return std::tolower(left) == std::tolower(right);
    }) != text.end();
}
}

ParticleEditor::ParticleEditor()
{
    std::snprintf(m_openPath.data(), m_openPath.size(), "%s", "../scop_unpacked/particles.xr");
    std::snprintf(m_savePath.data(), m_savePath.size(), "%s", "../scop_unpacked/particles_edited.xr");
    m_position.set(0.f, 0.f, 0.f);
}

ParticleEditor::~ParticleEditor()
{
    m_preview.Stop();
}

void ParticleEditor::ObserveClose()
{
    if (m_wasOpen && !is_open())
        RequestClose();
    m_wasOpen = is_open();
}

void ParticleEditor::Update(u32 milliseconds)
{
    ObserveClose();
    m_preview.Update(milliseconds);
}

void ParticleEditor::OnLevelUnload()
{
    m_preview.OnLevelUnload();
    m_hasPosition = false;
}

void ParticleEditor::RequestClose()
{
    m_preview.Stop();
    if (m_document.Dirty())
    {
        set_opened(true);
        m_pendingAction = PendingAction::Close;
        m_showDiscard = true;
    }
    else
        CloseDocument();
}

void ParticleEditor::CloseDocument()
{
    m_preview.Stop();
    m_document = ParticleLibraryDocument{};
    m_filtered.clear();
    m_selected = size_t(-1);
    m_pendingAction = PendingAction::None;
    m_error.clear();
    m_status.clear();
    set_opened(false);
    m_wasOpen = false;
}

void ParticleEditor::OpenDocument(const std::string& path)
{
    m_error.clear();
    m_status.clear();
    m_preview.Stop();
    if (!m_document.Open(path, m_error))
        return;
    m_selected = m_document.Entries().empty() ? size_t(-1) : 0;
    RebuildFilter();
    const auto separator = path.find_last_of("/\\");
    const std::string destination = (separator == std::string::npos ? std::string{} : path.substr(0, separator + 1))
        + "particles_edited.xr";
    std::snprintf(m_savePath.data(), m_savePath.size(), "%s", destination.c_str());
    m_status = "Library opened. The source file is never modified.";
}

void ParticleEditor::RequestSave()
{
    m_error.clear();
    m_status.clear();
    m_pendingSave = m_savePath.data();
    if (m_pendingSave.empty())
    {
        m_error = "Enter a Save As destination.";
        return;
    }
    try
    {
        std::error_code error;
        const auto status = std::filesystem::symlink_status(m_pendingSave, error);
        if (error && error != std::errc::no_such_file_or_directory)
        {
            m_error = "Cannot inspect destination: " + error.message();
            return;
        }
        if (std::filesystem::exists(status))
            m_showOverwrite = true;
        else
            SaveDocument(m_pendingSave);
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        m_error = error.what();
    }
}

void ParticleEditor::SaveDocument(const std::string& path)
{
    m_error.clear();
    m_status.clear();
    if (m_document.SaveAs(path, m_error))
        m_status = "Saved: " + path;
}

void ParticleEditor::RebuildFilter()
{
    m_filtered.clear();
    const auto& entries = m_document.Entries();
    m_filtered.reserve(entries.size());
    for (size_t index = 0; index < entries.size(); ++index)
    {
        const auto& entry = entries[index];
        if (m_kindFilter == 1 && entry.kind != ParticleLibraryDocument::Kind::Effect)
            continue;
        if (m_kindFilter == 2 && entry.kind != ParticleLibraryDocument::Kind::Group)
            continue;
        if (ContainsInsensitive(entry.name, m_filter.data()))
            m_filtered.push_back(index);
    }
}

void ParticleEditor::PlaceInFront()
{
    if (!HasLevel())
        return;
    Fvector direction = Device.vCameraDirection;
    direction.normalize_safe();
    m_position.mad(Device.vCameraPosition, direction, m_cameraDistance);
    m_hasPosition = true;
    m_preview.SetPosition(m_position);
}

void ParticleEditor::DrawFiles()
{
    ImGui::InputText("Source path", m_openPath.data(), m_openPath.size());
    ImGui::SameLine();
    if (ImGui::Button("Open"))
    {
        m_pendingOpen = m_openPath.data();
        if (m_document.Dirty())
        {
            m_pendingAction = PendingAction::Open;
            m_showDiscard = true;
        }
        else
            OpenDocument(m_pendingOpen);
    }
    ImGui::InputText("Destination", m_savePath.data(), m_savePath.size());
    ImGui::SameLine();
    ImGui::BeginDisabled(m_document.Path().empty());
    if (ImGui::Button("Save As"))
        RequestSave();
    ImGui::EndDisabled();
    if (!m_document.Path().empty())
        ImGui::TextWrapped("%s%s", m_document.Path().c_str(), m_document.Dirty() ? "  [unsaved changes]" : "");
    ImGui::TextDisabled("Source protected. Existing destinations require confirmation. Raw flags: press Enter to apply.");
}

void ParticleEditor::DrawBrowser()
{
    bool changed = ImGui::InputText("Search", m_filter.data(), m_filter.size());
    changed |= ImGui::Combo("Show", &m_kindFilter, "Effects and groups\0Effects\0Groups\0");
    if (changed)
        RebuildFilter();
    ImGui::Text("%zu / %zu entries", m_filtered.size(), m_document.Entries().size());
    if (ImGui::BeginChild("Entry list", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders))
    {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_filtered.size()));
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
            {
                const size_t index = m_filtered[row];
                const auto& entry = m_document.Entries()[index];
                ImGui::PushID(static_cast<int>(index));
                const ImVec2 position = ImGui::GetCursorScreenPos();
                if (ImGui::Selectable("##entry", m_selected == index))
                {
                    if (m_selected != index)
                        m_preview.Stop();
                    m_selected = index;
                }
                ImGui::GetWindowDrawList()->AddText(position, ImGui::GetColorU32(ImGuiCol_Text),
                    entry.kind == ParticleLibraryDocument::Kind::Effect ? "E" : "G");
                ImGui::GetWindowDrawList()->AddText(ImVec2(position.x + ImGui::GetFontSize() * 1.5f, position.y),
                    ImGui::GetColorU32(ImGuiCol_Text), entry.name.c_str());
                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();
}

bool ParticleEditor::DrawFlags(uint32_t& flags, bool effectDefinition, bool groupEffect)
{
    bool changed = ImGui::InputScalar("Flags (hex)", ImGuiDataType_U32, &flags, nullptr, nullptr, "%08X",
        ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);
    changed |= ImGui::InputScalar("Flags (decimal)", ImGuiDataType_U32, &flags, nullptr, nullptr, "%u",
        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue);
    const NamedFlag* names = effectDefinition ? effectFlags : groupEffectFlags;
    const size_t count = effectDefinition ? std::size(effectFlags) : groupEffect ? std::size(groupEffectFlags) : 0;
    uint32_t known = 0;
    for (size_t index = 0; index < count; ++index)
    {
        known |= names[index].mask;
        bool enabled = (flags & names[index].mask) != 0;
        if (ImGui::Checkbox(names[index].name, &enabled))
        {
            flags = enabled ? flags | names[index].mask : flags & ~names[index].mask;
            changed = true;
        }
    }
    ImGui::Text("Other bits: 0x%08X", flags & ~known);
    return changed;
}

void ParticleEditor::FlagsChanged(bool success)
{
    m_status.clear();
    if (success)
    {
        m_error.clear();
        m_preview.Stop();
        m_status = "Flags updated. Preview stopped; Play / Restart uses the edited library.";
    }
}

void ParticleEditor::DrawSelection()
{
    const auto& entries = m_document.Entries();
    if (m_selected >= entries.size())
    {
        ImGui::TextWrapped("Open a particles.xr library and select an effect or group. Browsing, flags and Save As work without a level.");
        return;
    }
    const auto& entry = entries[m_selected];
    const bool effect = entry.kind == ParticleLibraryDocument::Kind::Effect;
    ImGui::TextWrapped("%s: %s", effect ? "Effect" : "Group", entry.name.c_str());
    const auto drawMaterial = [](const ParticleLibraryDocument::Entry& definition)
    {
        ImGui::TextWrapped("Shader (particles.xr): %s",
            definition.shader.empty() ? "(not defined)" : definition.shader.c_str());
        ImGui::TextWrapped("Texture: %s",
            definition.texture.empty() ? "(not defined)" : definition.texture.c_str());
    };
    if (effect)
    {
        drawMaterial(entry);
        ImGui::Text("Capacity: %u particles", entry.maxParticles);
    }
    else
        ImGui::TextWrapped("Shaders are defined by the effects referenced below, not by the group itself.");
    ImGui::Text("Time limit: %.3f s", entry.timeLimit);
    if (effect && !(entry.flags & PS::CPEDef::dfTimeLimit))
        ImGui::TextDisabled("Time limit flag is disabled.");
    if (effect && ImGui::CollapsingHeader("Particle timing (live GPU)")) {
        auto& manager = GetGpuParticleManager();
        const PS::CPEDef* liveDef = nullptr;
        for (const auto* candidate : manager.GetDefinitions())
            if (candidate && 0 == xr_strcmp(candidate->Name(), entry.name.c_str())) { liveDef = candidate; break; }
        if (!liveDef) {
            ImGui::TextDisabled("Effect not spawned in the world yet.");
        } else {
            float timeLimit = manager.GetTimeLimit(*liveDef);
            if (ImGui::SliderFloat("Time limit##live", &timeLimit, 0.f, 30.f, "%.2f s"))
                manager.SetTimeLimit(*liveDef, timeLimit);
            auto params = manager.GetActionParams(*liveDef);
            for (u32 i = 0; i < params.size(); ++i) {
                ImGui::PushID(int(i));
                if (params[i].type == 10) {
                    if (ImGui::SliderFloat("KillOld age", &params[i].value, 0.01f, 60.f, "%.2f s"))
                        manager.SetActionParam(*liveDef, i, params[i].value);
                } else if (params[i].type == 21) {
                    if (ImGui::SliderFloat("Source rate", &params[i].value, 0.f, 1000.f, "%.1f /s"))
                        manager.SetActionParam(*liveDef, i, params[i].value);
                }
                ImGui::PopID();
            }
        }
    }
    if (ImGui::CollapsingHeader("Definition flags", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("definition");
        uint32_t flags = entry.flags;
        if (DrawFlags(flags, effect, false))
            FlagsChanged(m_document.SetFlags(m_selected, flags, m_error));
        ImGui::PopID();
    }
    if (!effect)
    {
        ImGui::Separator();
        ImGui::Text("Group effects: %zu", entry.effects.size());
        const auto drawReference = [&](const char* label, const std::string& name)
        {
            ImGui::TextWrapped("%s: %s", label, name.empty() ? "(none)" : name.c_str());
            if (name.empty())
                return;
            const auto definition = std::find_if(entries.begin(), entries.end(), [&](const auto& candidate)
            {
                return candidate.kind == ParticleLibraryDocument::Kind::Effect && candidate.name == name;
            });
            ImGui::Indent();
            if (definition != entries.end())
                drawMaterial(*definition);
            else
                ImGui::TextWrapped("Effect not found in this particles.xr library; shader is unknown.");
            ImGui::Unindent();
        };
        for (size_t index = 0; index < entry.effects.size(); ++index)
        {
            const auto& item = entry.effects[index];
            ImGui::PushID(static_cast<int>(index));
            if (ImGui::TreeNode("group effect", "%zu: %s", index + 1, item.effectName.c_str()))
            {
                drawReference("Effect", item.effectName);
                ImGui::Text("Start: %.3f s   End: %.3f s", item.timeStart, item.timeEnd);
                drawReference("On play", item.onPlayChild);
                drawReference("On birth", item.onBirthChild);
                drawReference("On death", item.onDeadChild);
                uint32_t flags = item.flags;
                if (DrawFlags(flags, false, true))
                    FlagsChanged(m_document.SetGroupEffectFlags(m_selected, index, flags, m_error));
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }
}

void ParticleEditor::DrawPlayback()
{
    ImGui::Separator();
    const bool level = HasLevel();
    if (!level)
        ImGui::TextDisabled("Load a level to preview particles. File editing remains available.");
    ImGui::BeginDisabled(!level);
    ImGui::SetNextItemWidth(160.f);
    ImGui::DragFloat("Camera distance (m)", &m_cameraDistance, 0.1f, 0.1f, 1000.f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    if (ImGui::Button("Place in front"))
        PlaceInFront();
    float position[] = {m_position.x, m_position.y, m_position.z};
    if (ImGui::InputFloat3("World XYZ", position, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (std::isfinite(position[0]) && std::isfinite(position[1]) && std::isfinite(position[2]))
        {
            m_position.set(position[0], position[1], position[2]);
            m_hasPosition = true;
            m_preview.SetPosition(m_position);
        }
        else
            m_error = "Preview position must contain finite coordinates.";
    }
    ImGui::BeginDisabled(m_selected >= m_document.Entries().size());
    if (ImGui::Button("Play / Restart"))
    {
        if (!m_hasPosition)
            PlaceInFront();
        m_error.clear();
        m_status.clear();
        if (m_preview.Play(m_document, m_selected, m_position, m_error))
            m_status = "Preview playing in the world. Hide the editor to inspect it; closing the tool stops playback.";
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!m_preview.IsPlaying());
    if (ImGui::Button(m_preview.IsPaused() ? "Resume" : "Pause"))
        m_preview.SetPaused(!m_preview.IsPaused());
    ImGui::SameLine();
    if (ImGui::Button("Step 33 ms"))
    {
        m_preview.SetPaused(true);
        m_preview.Step();
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
        m_preview.Stop();
    ImGui::SameLine();
    ImGui::Text("%s | %.3f s | %u particles", m_preview.IsPlaying() ? (m_preview.IsPaused() ? "Paused" : "Playing") : "Stopped",
        m_preview.Time(), m_preview.Count());
}

void ParticleEditor::DrawConfirmations()
{
    if (m_showDiscard)
    {
        ImGui::OpenPopup("Unsaved particle edits");
        m_showDiscard = false;
    }
    if (ImGui::BeginPopupModal("Unsaved particle edits", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("This library has unsaved flag changes.");
        ImGui::TextUnformatted(m_pendingAction == PendingAction::Open ? "Discard them and open the requested library?" : "Discard them and close the tool?");
        ImGui::TextUnformatted("Cancel to return and use Save As first.");
        if (ImGui::Button("Discard changes"))
        {
            const auto action = m_pendingAction;
            m_pendingAction = PendingAction::None;
            ImGui::CloseCurrentPopup();
            if (action == PendingAction::Open)
                OpenDocument(m_pendingOpen);
            else if (action == PendingAction::Close)
                CloseDocument();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_pendingAction = PendingAction::None;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (m_showOverwrite)
    {
        ImGui::OpenPopup("Overwrite particle destination");
        m_showOverwrite = false;
    }
    if (ImGui::BeginPopupModal("Overwrite particle destination", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("This destination already exists:");
        ImGui::TextWrapped("%s", m_pendingSave.c_str());
        ImGui::TextUnformatted("Replace it with this edited library? The original source remains protected.");
        if (ImGui::Button("Overwrite"))
        {
            ImGui::CloseCurrentPopup();
            SaveDocument(m_pendingSave);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void ParticleEditor::on_tool_frame()
{
    ObserveClose();
    if (!is_open())
        return;
    m_wasOpen = true;
    ImGui::SetNextWindowSize(ImVec2(1050.f, 800.f), ImGuiCond_FirstUseEver);
    bool opened = true;
    const bool visible = ImGui::Begin(tool_name(), &opened, get_default_window_flags());
    if (!opened)
        RequestClose();
    if (visible && is_open())
    {
        DrawFiles();
        DrawPlayback();
        if (!m_error.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.35f, 1.f));
            ImGui::TextWrapped("%s", m_error.c_str());
            ImGui::PopStyleColor();
        }
        if (!m_status.empty())
            ImGui::TextWrapped("%s", m_status.c_str());
        ImGui::Separator();
        const float width = std::max(200.f, ImGui::GetContentRegionAvail().x * 0.34f);
        if (ImGui::BeginChild("Browser", ImVec2(width, 0.f), ImGuiChildFlags_Borders))
            DrawBrowser();
        ImGui::EndChild();
        ImGui::SameLine();
        if (ImGui::BeginChild("Selection", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders))
            DrawSelection();
        ImGui::EndChild();
    }
    DrawConfirmations();
    ImGui::End();
    m_wasOpen = is_open();
}
}
