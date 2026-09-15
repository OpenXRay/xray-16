#include "stdafx.h"
#include "ParticleLibraryPreview.h"
#include "ParticleLibraryDocument.h"
#include "../PSLibrary.h"
#include "../ParticleEffect.h"
#include "../ParticleGroup.h"
#include "../GpuParticleTranslate.h"
#include "xrEngine/IGame_Level.h"
#include <algorithm>
#include <vector>

namespace xray::render::fg {

struct ParticleLibraryPreview::Impl {
    struct Library {
        CPSLibrary definitions;
        std::vector<uint8_t> bytes;

        ~Library() {
            for (auto it = definitions.FirstPED(); it != definitions.LastPED(); ++it)
                GetGpuParticleManager().ReleaseDefinition(**it);
            definitions.OnDestroy();
        }
    };

    std::unique_ptr<Library> library;
    std::unique_ptr<Library> pendingLibrary;
    dxParticleCustom* visual = nullptr;
    ParticleLibraryDocument::Kind pendingKind = ParticleLibraryDocument::Kind::Effect;
    std::string pendingName;
    Fvector position{};
    bool playRequested = false;
    bool stopRequested = false;
    bool paused = false;
    bool stepRequested = false;
    bool positionDirty = false;
    u32 count = 0;
    float time = 0.f;

    void DestroyVisual() {
        if (visual)
            visual->Stop(FALSE);
        xr_delete(visual);
        count = 0;
        time = 0.f;
    }

    bool Validate(Library& source, const ParticleLibraryDocument::Entry& entry, std::string& error) {
        xr_vector<const PS::CPEDef*> checked;
        auto validateEffect = [&](const char* name, bool freeChild) {
            auto* definition = source.definitions.FindPED(name);
            if (!definition) {
                error = "Missing particle effect: " + std::string(name ? name : "");
                return false;
            }
            if (freeChild && !definition->m_Flags.is(PS::CPEDef::dfTimeLimit)) {
                error = "Birth/death child requires Time limit: " + std::string(name);
                return false;
            }
            if (std::find(checked.begin(), checked.end(), definition) != checked.end())
                return true;
            GpuPapiProgram program{};
            xr_vector<GpuPapiAction> actions;
            xr_string translationError;
            if (!TranslateGpuParticleDefinition(*definition, program, actions, translationError)) {
                error = translationError.c_str();
                return false;
            }
            checked.push_back(definition);
            return true;
        };
        if (entry.kind == ParticleLibraryDocument::Kind::Effect)
            return validateEffect(entry.name.c_str(), false);
        auto* group = source.definitions.FindPGD(entry.name.c_str());
        if (!group) {
            error = "Missing particle group: " + entry.name;
            return false;
        }
        for (const auto* effect : group->m_Effects) {
            if (!validateEffect(effect->m_EffectName.c_str(), false))
                return false;
            if (effect->m_Flags.is(PS::CPGDef::SEffect::flOnPlayChild) &&
                !validateEffect(effect->m_OnPlayChildName.c_str(), false))
                return false;
            if (effect->m_Flags.is(PS::CPGDef::SEffect::flOnBirthChild) &&
                !validateEffect(effect->m_OnBirthChildName.c_str(), true))
                return false;
            if (effect->m_Flags.is(PS::CPGDef::SEffect::flOnDeadChild) &&
                !validateEffect(effect->m_OnDeadChildName.c_str(), true))
                return false;
        }
        return true;
    }

    void Move() {
        if (!visual || !positionDirty)
            return;
        Fmatrix transform;
        transform.identity();
        transform.c = position;
        Fvector velocity;
        velocity.set(0.f, 0.f, 0.f);
        visual->UpdateParent(transform, velocity, TRUE);
        positionDirty = false;
    }
};

ParticleLibraryPreview::ParticleLibraryPreview() : m_impl(std::make_unique<Impl>()) {}

ParticleLibraryPreview::~ParticleLibraryPreview() {
    OnLevelUnload();
}

bool ParticleLibraryPreview::Play(const ParticleLibraryDocument& document, size_t index,
    const Fvector& position, std::string& error) {
    error.clear();
    if (!g_pGameLevel || !Device.b_is_Ready) {
        error = "Load a level before playing a particle preview.";
        return false;
    }
    if (index >= document.Entries().size()) {
        error = "Select an effect or group first.";
        return false;
    }
    auto& state = *m_impl;
    std::unique_ptr<Impl::Library> replacement;
    auto* source = state.library.get();
    if (!source || source->bytes != document.Bytes()) {
        replacement = std::make_unique<Impl::Library>();
        replacement->bytes = document.Bytes();
        IReader reader(replacement->bytes.data(), replacement->bytes.size());
        if (!replacement->definitions.Load(reader)) {
            error = "The engine could not load this particle library.";
            return false;
        }
        source = replacement.get();
    }
    const auto& entry = document.Entries()[index];
    if (!state.Validate(*source, entry, error))
        return false;
    state.pendingLibrary = std::move(replacement);
    state.pendingName = entry.name;
    state.pendingKind = entry.kind;
    state.position = position;
    state.positionDirty = true;
    state.playRequested = true;
    state.stopRequested = false;
    state.paused = false;
    state.stepRequested = false;
    return true;
}

void ParticleLibraryPreview::Stop() {
    m_impl->playRequested = false;
    m_impl->stopRequested = true;
    m_impl->stepRequested = false;
    m_impl->paused = false;
    m_impl->pendingLibrary.reset();
}

void ParticleLibraryPreview::SetPaused(bool paused) { m_impl->paused = paused; }
bool ParticleLibraryPreview::IsPaused() const { return m_impl->paused; }
bool ParticleLibraryPreview::IsPlaying() const {
    return !m_impl->stopRequested && (m_impl->playRequested || (m_impl->visual && m_impl->visual->IsPlaying()));
}
void ParticleLibraryPreview::Step() {
    m_impl->paused = true;
    m_impl->stepRequested = true;
}

void ParticleLibraryPreview::Update(u32 milliseconds) {
    if (!g_pGameLevel || !Device.b_is_Ready) {
        OnLevelUnload();
        return;
    }
    auto& state = *m_impl;
    if (state.stopRequested || state.playRequested) {
        state.DestroyVisual();
        state.stopRequested = false;
    }
    if (state.playRequested) {
        if (state.pendingLibrary)
            state.library = std::move(state.pendingLibrary);
        auto& library = state.library->definitions;
        if (state.pendingKind == ParticleLibraryDocument::Kind::Effect) {
            auto* effect = xr_new<PS::CParticleEffect>();
            effect->Compile(library.FindPED(state.pendingName.c_str()));
            state.visual = effect;
        } else {
            auto* group = xr_new<PS::CParticleGroup>();
            group->Compile(library.FindPGD(state.pendingName.c_str()), library);
            state.visual = group;
        }
        state.visual->SetHudMode(FALSE);
        state.Move();
        state.visual->Play();
        state.playRequested = false;
    }
    state.Move();
    if (!state.visual)
        return;
    if ((!state.paused || state.stepRequested) && state.visual->IsPlaying()) {
        const u32 delta = state.stepRequested ? 33u : milliseconds;
        state.visual->OnFrame(delta);
        state.time += float(delta) / 1000.f;
    }
    state.stepRequested = false;
    state.count = state.visual->ParticlesCount();
}

u32 ParticleLibraryPreview::Count() const { return m_impl->stopRequested ? 0 : m_impl->count; }
float ParticleLibraryPreview::Time() const { return m_impl->stopRequested ? 0.f : m_impl->time; }
void ParticleLibraryPreview::SetPosition(const Fvector& position) {
    m_impl->position = position;
    m_impl->positionDirty = true;
}
void ParticleLibraryPreview::OnLevelUnload() {
    Stop();
    m_impl->DestroyVisual();
    m_impl->library.reset();
    m_impl->stopRequested = false;
}

}
