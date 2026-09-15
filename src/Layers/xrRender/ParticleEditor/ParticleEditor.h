#pragma once

#include "xrEngine/editor_base.h"
#include "ParticleLibraryDocument.h"
#include "ParticleLibraryPreview.h"

#include <array>
#include <string>
#include <vector>

namespace xray::render::fg
{
class ParticleEditor final : public xray::editor::ide_tool
{
public:
    ParticleEditor();
    ~ParticleEditor() override;

    void Update(u32 milliseconds);
    void OnLevelUnload();
    void on_tool_frame() override;
    pcstr tool_name() const override { return "Particle Editor"; }

private:
    enum class PendingAction { None, Open, Close };

    void ObserveClose();
    void RequestClose();
    void CloseDocument();
    void OpenDocument(const std::string& path);
    void RequestSave();
    void SaveDocument(const std::string& path);
    void RebuildFilter();
    void PlaceInFront();
    void DrawFiles();
    void DrawBrowser();
    void DrawSelection();
    void DrawPlayback();
    void DrawConfirmations();
    bool DrawFlags(uint32_t& flags, bool effectDefinition, bool groupEffect);
    void FlagsChanged(bool success);

    ParticleLibraryDocument m_document;
    ParticleLibraryPreview m_preview;
    std::array<char, 4096> m_openPath{};
    std::array<char, 4096> m_savePath{};
    std::array<char, 256> m_filter{};
    std::vector<size_t> m_filtered;
    size_t m_selected = size_t(-1);
    int m_kindFilter = 0;
    Fvector m_position{};
    float m_cameraDistance = 4.f;
    bool m_hasPosition = false;
    bool m_wasOpen = false;
    bool m_showDiscard = false;
    bool m_showOverwrite = false;
    PendingAction m_pendingAction = PendingAction::None;
    std::string m_pendingOpen;
    std::string m_pendingSave;
    std::string m_error;
    std::string m_status;
};
}
