#pragma once

#include "xrCore/xrCore.h"
#include <memory>
#include <string>

namespace xray::render::fg {

class ParticleLibraryDocument;

class ParticleLibraryPreview {
public:
    ParticleLibraryPreview();
    ~ParticleLibraryPreview();
    bool Play(const ParticleLibraryDocument& document, size_t index, const Fvector& position, std::string& error);
    void Stop();
    void SetPaused(bool paused);
    bool IsPaused() const;
    bool IsPlaying() const;
    void Step();
    void Update(u32 milliseconds);
    u32 Count() const;
    float Time() const;
    void SetPosition(const Fvector& position);
    void OnLevelUnload();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
