#pragma once

#include "AnimationECS_Components.h"
#include "entt/entt.hpp"
#include "../../../Externals/imgui/imgui.h"
#include "ozz/animation/runtime/skeleton_utils.h"
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace AnimationECS {

enum class MovementDirection : u8
{
    None = 0,
    Forward,
    Backward,
    StrafeLeft,
    StrafeRight
};

enum class BodyPosture : u8
{
    Stand = 0,
    Crouch
};

enum class MovementSpeed : u8
{
    Walk = 0,
    Run,
    Sprint
};

struct MovementInput
{
    MovementDirection direction{MovementDirection::None};
    BodyPosture posture{BodyPosture::Stand};
    MovementSpeed speed{MovementSpeed::Walk};
    bool enabled{true};
};

struct MovementAnimationMap
{
    struct AnimEntry
    {
        const ozz::animation::Animation* animation{nullptr};
        float speed{1.0f};
        int sourceIndex{-1};
    };

    AnimEntry standIdle;
    AnimEntry standWalkFwd, standWalkBack, standWalkLs, standWalkRs;
    AnimEntry standRunFwd, standRunBack, standRunLs, standRunRs;
    AnimEntry sprintFwd, sprintLs, sprintRs;
    AnimEntry crouchIdle;
    AnimEntry crouchWalkFwd, crouchWalkBack, crouchWalkLs, crouchWalkRs;
    AnimEntry crouchRunFwd, crouchRunBack, crouchRunLs, crouchRunRs;
    AnimEntry jumpBegin, jumpIdle;
    AnimEntry landingLight, landingHeavy;
    AnimEntry turn;

    bool populated{false};
    int matchedCount{0};
    int totalSlots{0};

    const AnimEntry* Resolve(MovementDirection dir, BodyPosture posture, MovementSpeed speed) const
    {
        if (dir == MovementDirection::None)
            return (posture == BodyPosture::Crouch) ? &crouchIdle : &standIdle;

        if (speed == MovementSpeed::Sprint && posture == BodyPosture::Stand) {
            switch (dir) {
            case MovementDirection::Forward:    return &sprintFwd;
            case MovementDirection::StrafeLeft: return &sprintLs;
            case MovementDirection::StrafeRight:return &sprintRs;
            default:                           return &sprintFwd;
            }
        }

        if (posture == BodyPosture::Crouch) {
            if (speed == MovementSpeed::Run) {
                switch (dir) {
                case MovementDirection::Forward:    return &crouchRunFwd;
                case MovementDirection::Backward:   return &crouchRunBack;
                case MovementDirection::StrafeLeft: return &crouchRunLs;
                case MovementDirection::StrafeRight:return &crouchRunRs;
                default: return &crouchIdle;
                }
            }
            switch (dir) {
            case MovementDirection::Forward:    return &crouchWalkFwd;
            case MovementDirection::Backward:   return &crouchWalkBack;
            case MovementDirection::StrafeLeft: return &crouchWalkLs;
            case MovementDirection::StrafeRight:return &crouchWalkRs;
            default: return &crouchIdle;
            }
        }

        if (speed == MovementSpeed::Run) {
            switch (dir) {
            case MovementDirection::Forward:    return &standRunFwd;
            case MovementDirection::Backward:   return &standRunBack;
            case MovementDirection::StrafeLeft: return &standRunLs;
            case MovementDirection::StrafeRight:return &standRunRs;
            default: return &standIdle;
            }
        }

        switch (dir) {
        case MovementDirection::Forward:    return &standWalkFwd;
        case MovementDirection::Backward:   return &standWalkBack;
        case MovementDirection::StrafeLeft: return &standWalkLs;
        case MovementDirection::StrafeRight:return &standWalkRs;
        default: return &standIdle;
        }
    }

    template<typename MetadataT>
    static void BuildFromAnimations(
        MovementAnimationMap& map,
        const std::vector<ozz::animation::Animation>& animations,
        const std::vector<MetadataT>& metadata)
    {
        std::unordered_map<std::string, AnimEntry*> nameTable;
        nameTable["norm_idle_0"]       = &map.standIdle;
        nameTable["norm_walk_fwd_0"]   = &map.standWalkFwd;
        nameTable["norm_walk_back_0"]  = &map.standWalkBack;
        nameTable["norm_walk_ls_0"]    = &map.standWalkLs;
        nameTable["norm_walk_rs_0"]    = &map.standWalkRs;
        nameTable["norm_run_fwd_0"]    = &map.standRunFwd;
        nameTable["norm_run_back_0"]   = &map.standRunBack;
        nameTable["norm_run_ls_0"]     = &map.standRunLs;
        nameTable["norm_run_rs_0"]     = &map.standRunRs;
        nameTable["norm_escape_00"]    = &map.sprintFwd;
        nameTable["norm_escape_ls_00"] = &map.sprintLs;
        nameTable["norm_escape_rs_00"] = &map.sprintRs;
        nameTable["cr_idle_1"]         = &map.crouchIdle;
        nameTable["cr_walk_fwd_0"]     = &map.crouchWalkFwd;
        nameTable["cr_walk_back_0"]    = &map.crouchWalkBack;
        nameTable["cr_walk_ls_0"]      = &map.crouchWalkLs;
        nameTable["cr_walk_rs_0"]      = &map.crouchWalkRs;
        nameTable["cr_run_fwd_0"]      = &map.crouchRunFwd;
        nameTable["cr_run_back_0"]     = &map.crouchRunBack;
        nameTable["cr_run_ls_0"]       = &map.crouchRunLs;
        nameTable["cr_run_rs_0"]       = &map.crouchRunRs;
        nameTable["norm_jump_begin"]   = &map.jumpBegin;
        nameTable["norm_jump_idle"]    = &map.jumpIdle;
        nameTable["norm_jump_end"]     = &map.landingLight;
        nameTable["norm_jump_end_1"]   = &map.landingHeavy;
        nameTable["norm_turn"]         = &map.turn;

        map.totalSlots = static_cast<int>(nameTable.size());
        map.matchedCount = 0;

        for (size_t i = 0; i < animations.size(); ++i) {
            std::string name;
            if (i < metadata.size() && !metadata[i].name.empty())
                name = metadata[i].name;
            else if (animations[i].name() && animations[i].name()[0] != '\0')
                name = animations[i].name();
            else
                continue;

            auto it = nameTable.find(name);
            if (it != nameTable.end() && !it->second->animation) {
                it->second->animation = &animations[i];
                it->second->sourceIndex = static_cast<int>(i);
                float metaSpeed = (i < metadata.size()) ? metadata[i].speed : 1.0f;
                it->second->speed = (metaSpeed > 0.0f) ? metaSpeed : 1.0f;
                ++map.matchedCount;
            }
        }

        map.populated = true;
        Msg("[MovementSM] Animation map: matched %d/%d slots", map.matchedCount, map.totalSlots);

        for (auto& [slotName, entry] : nameTable) {
            if (!entry->animation)
                Msg("[MovementSM]   MISSING: '%s'", slotName.c_str());
        }
    }
};

struct MovementStateMachineState
{
    MovementDirection lastDirection{MovementDirection::None};
    BodyPosture lastPosture{BodyPosture::Stand};
    MovementSpeed lastSpeed{MovementSpeed::Walk};
    const char* lastAnimationName{nullptr};
    int lastAnimationIndex{-1};
    bool animationChanged{false};
};

class MovementInputSystem
{
public:
    static void Update(entt::registry& registry)
    {
        auto& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard)
            return;

        MovementDirection dir = MovementDirection::None;
        if (ImGui::IsKeyDown(ImGuiKey_W))
            dir = MovementDirection::Forward;
        else if (ImGui::IsKeyDown(ImGuiKey_S))
            dir = MovementDirection::Backward;
        else if (ImGui::IsKeyDown(ImGuiKey_A))
            dir = MovementDirection::StrafeLeft;
        else if (ImGui::IsKeyDown(ImGuiKey_D))
            dir = MovementDirection::StrafeRight;

        BodyPosture posture = BodyPosture::Stand;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
            posture = BodyPosture::Crouch;

        MovementSpeed speed = MovementSpeed::Walk;
        if (dir != MovementDirection::None &&
            (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)))
            speed = MovementSpeed::Run;

        auto view = registry.view<MovementInput>();
        for (auto entity : view) {
            auto& input = view.get<MovementInput>(entity);
            if (!input.enabled)
                continue;
            input.direction = dir;
            input.posture = posture;
            input.speed = speed;
        }
    }
};

class MovementAnimationSelectionSystem
{
public:
    static void Update(entt::registry& registry)
    {
        auto view = registry.view<MovementInput, MovementAnimationMap,
                                  AnimationController, AnimationState,
                                  MovementStateMachineState>();

        for (auto entity : view) {
            auto& input = view.get<MovementInput>(entity);
            auto& animMap = view.get<MovementAnimationMap>(entity);
            auto& controller = view.get<AnimationController>(entity);
            auto& animState = view.get<AnimationState>(entity);
            auto& smState = view.get<MovementStateMachineState>(entity);

            if (!input.enabled || !animMap.populated)
                continue;

            const auto* entry = animMap.Resolve(input.direction, input.posture, input.speed);

            if (!entry || !entry->animation) {
                const auto* fallback = animMap.Resolve(MovementDirection::None, BodyPosture::Stand, MovementSpeed::Walk);
                if (!fallback || !fallback->animation)
                    continue;
                entry = fallback;
            }

            smState.lastDirection = input.direction;
            smState.lastPosture = input.posture;
            smState.lastSpeed = input.speed;
            smState.lastAnimationName = entry->animation->name();
            smState.lastAnimationIndex = entry->sourceIndex;

            if (controller.animation == entry->animation) {
                smState.animationChanged = false;
                continue;
            }

            controller.animation = entry->animation;
            controller.playback_speed = entry->speed;
            animState.current_time = 0.0f;
            animState.time_ratio = 0.0f;
            animState.is_playing = true;
            animState.is_looping = true;
            smState.animationChanged = true;
        }
    }
};

inline const char* DirectionName(MovementDirection d)
{
    switch (d) {
    case MovementDirection::None:        return "Idle";
    case MovementDirection::Forward:     return "Forward";
    case MovementDirection::Backward:    return "Backward";
    case MovementDirection::StrafeLeft:  return "Strafe Left";
    case MovementDirection::StrafeRight: return "Strafe Right";
    }
    return "?";
}

inline const char* PostureName(BodyPosture p)
{
    return (p == BodyPosture::Crouch) ? "Crouch" : "Stand";
}

inline const char* SpeedName(MovementSpeed s)
{
    switch (s) {
    case MovementSpeed::Walk:   return "Walk";
    case MovementSpeed::Run:    return "Run";
    case MovementSpeed::Sprint: return "Sprint";
    }
    return "?";
}

enum class BodyChannel : u8 { Legs = 0, Torso = 1, Head = 2, Count = 3 };

struct ChannelState
{
    const ozz::animation::Animation* animation{nullptr};
    ozz::animation::SamplingJob::Context context;
    ozz::vector<ozz::math::SoaTransform> locals;
    float currentTime{0.0f};
    float timeRatio{0.0f};
    float playbackSpeed{1.0f};
    bool isPlaying{false};
    bool isLooping{true};

    void Initialize(int numJoints, int numSoaJoints)
    {
        locals.resize(numSoaJoints);
        context.Resize(numJoints);
    }

    bool IsInitialized() const { return !locals.empty(); }

    void SetAnimation(const ozz::animation::Animation* anim, float speed = 1.0f, bool loop = true)
    {
        if (animation == anim)
            return;
        animation = anim;
        playbackSpeed = speed;
        currentTime = 0.0f;
        timeRatio = 0.0f;
        isPlaying = (anim != nullptr);
        isLooping = loop;
    }

    void AdvanceTime(float dt)
    {
        if (!isPlaying || !animation)
            return;
        currentTime += dt * playbackSpeed;
        const float duration = animation->duration();
        if (duration <= 0.0f)
            return;
        timeRatio = currentTime / duration;
        if (timeRatio > 1.0f) {
            if (isLooping) {
                currentTime = fmodf(currentTime, duration);
                timeRatio = fmodf(timeRatio, 1.0f);
            } else {
                currentTime = duration;
                timeRatio = 1.0f;
                isPlaying = false;
            }
        }
    }

    bool Sample()
    {
        if (!isPlaying || !animation || locals.empty())
            return false;
        ozz::animation::SamplingJob job;
        job.animation = animation;
        job.context = &context;
        job.ratio = timeRatio;
        job.output = ozz::make_span(locals);
        return job.Run();
    }

    bool Finished() const { return !isPlaying && timeRatio >= 1.0f; }
};

struct MultiChannelAnimator
{
    static constexpr int kChannelCount = static_cast<int>(BodyChannel::Count);

    ChannelState channels[kChannelCount];
    ozz::vector<ozz::math::SimdFloat4> jointWeights[kChannelCount];

    int torsoRootJoint{-1};
    int headRootJoint{-1};
    bool initialized{false};

    void Initialize(const ozz::animation::Skeleton* skeleton)
    {
        const int nj = skeleton->num_joints();
        const int nsj = skeleton->num_soa_joints();

        for (int ch = 0; ch < kChannelCount; ++ch) {
            channels[ch].Initialize(nj, nsj);
            jointWeights[ch].resize(nsj);
        }

        const auto restPose = skeleton->joint_rest_poses();
        for (int ch = 0; ch < kChannelCount; ++ch)
            for (int i = 0; i < nsj; ++i)
                channels[ch].locals[i] = restPose[i];

        static const char* spineNames[] = {"bip01_spine", "bip01_spine1", "spine", "Spine1", nullptr};
        static const char* headNames[] = {"bip01_head", "bip01_neck", "head", "Head", nullptr};

        for (const char** n = spineNames; *n && torsoRootJoint < 0; ++n)
            torsoRootJoint = ozz::animation::FindJoint(*skeleton, *n);
        for (const char** n = headNames; *n && headRootJoint < 0; ++n)
            headRootJoint = ozz::animation::FindJoint(*skeleton, *n);

        BuildPartitionMasks(*skeleton);
        initialized = true;

        Msg("[MultiChannel] Init: joints=%d torsoRoot=%d headRoot=%d", nj, torsoRootJoint, headRootJoint);
    }

    void BuildPartitionMasks(const ozz::animation::Skeleton& skeleton)
    {
        const int nsj = skeleton.num_soa_joints();

        for (int i = 0; i < nsj; ++i)
            jointWeights[static_cast<int>(BodyChannel::Legs)][i] = ozz::math::simd_float4::one();
        for (int i = 0; i < nsj; ++i)
            jointWeights[static_cast<int>(BodyChannel::Torso)][i] = ozz::math::simd_float4::zero();
        for (int i = 0; i < nsj; ++i)
            jointWeights[static_cast<int>(BodyChannel::Head)][i] = ozz::math::simd_float4::zero();

        auto setWeight = [](ozz::vector<ozz::math::SimdFloat4>* w, float val) {
            return [w, val](int joint, int) {
                auto& soa = w->at(joint / 4);
                soa = ozz::math::SetI(soa, ozz::math::simd_float4::Load1(val), joint % 4);
            };
        };

        if (torsoRootJoint >= 0) {
            ozz::animation::IterateJointsDF(skeleton, setWeight(&jointWeights[static_cast<int>(BodyChannel::Legs)], 0.0f), torsoRootJoint);
            ozz::animation::IterateJointsDF(skeleton, setWeight(&jointWeights[static_cast<int>(BodyChannel::Torso)], 1.0f), torsoRootJoint);
        }

        if (headRootJoint >= 0) {
            ozz::animation::IterateJointsDF(skeleton, setWeight(&jointWeights[static_cast<int>(BodyChannel::Torso)], 0.0f), headRootJoint);
            ozz::animation::IterateJointsDF(skeleton, setWeight(&jointWeights[static_cast<int>(BodyChannel::Head)], 1.0f), headRootJoint);
        }
    }
};

struct TorsoAnimationMap
{
    static constexpr int kMaxSlots = 13;

    struct SlotEntry
    {
        const ozz::animation::Animation* aimIdle{nullptr};
        const ozz::animation::Animation* aimWalk{nullptr};
        const ozz::animation::Animation* aimRun{nullptr};
        float speedIdle{1.0f}, speedWalk{1.0f}, speedRun{1.0f};
    };

    SlotEntry slots[kMaxSlots];
    const ozz::animation::Animation* headIdle{nullptr};
    float headIdleSpeed{1.0f};
    int activeSlot{0};
    bool populated{false};
    int matchedCount{0};
    int totalSlots{0};

    const ozz::animation::Animation* Resolve(int slot, MovementSpeed speed, MovementDirection dir, float* outSpeed = nullptr) const
    {
        if (slot < 0 || slot >= kMaxSlots) {
            if (outSpeed) *outSpeed = 1.0f;
            return nullptr;
        }
        const auto& s = slots[slot];
        if (dir == MovementDirection::None || speed == MovementSpeed::Sprint) {
            if (outSpeed) *outSpeed = s.speedIdle;
            return s.aimIdle;
        }
        if (speed == MovementSpeed::Run) {
            if (outSpeed) *outSpeed = s.speedRun;
            return s.aimRun ? s.aimRun : s.aimIdle;
        }
        if (outSpeed) *outSpeed = s.speedWalk;
        return s.aimWalk ? s.aimWalk : s.aimIdle;
    }

    template<typename MetadataT>
    static void BuildFromAnimations(
        TorsoAnimationMap& map,
        const std::vector<ozz::animation::Animation>& animations,
        const std::vector<MetadataT>& metadata)
    {
        std::unordered_map<std::string, std::pair<const ozz::animation::Animation**, float*>> nameTable;

        for (int slot = 0; slot < map.kMaxSlots; ++slot) {
            std::string slotStr = std::to_string(slot) + "_";
            nameTable["norm_torso_" + slotStr + "aim_1"] = {&map.slots[slot].aimIdle, &map.slots[slot].speedIdle};
            nameTable["norm_torso_" + slotStr + "aim_2"] = {&map.slots[slot].aimWalk, &map.slots[slot].speedWalk};
            nameTable["norm_torso_" + slotStr + "aim_3"] = {&map.slots[slot].aimRun, &map.slots[slot].speedRun};
        }
        nameTable["head_idle_0"] = {&map.headIdle, &map.headIdleSpeed};

        map.totalSlots = static_cast<int>(nameTable.size());
        map.matchedCount = 0;

        for (size_t i = 0; i < animations.size(); ++i) {
            std::string name;
            if (i < metadata.size() && !metadata[i].name.empty())
                name = metadata[i].name;
            else if (animations[i].name() && animations[i].name()[0] != '\0')
                name = animations[i].name();
            else
                continue;

            auto it = nameTable.find(name);
            if (it != nameTable.end() && !(*it->second.first)) {
                *it->second.first = &animations[i];
                float metaSpeed = (i < metadata.size()) ? metadata[i].speed : 1.0f;
                *it->second.second = (metaSpeed > 0.0f) ? metaSpeed : 1.0f;
                ++map.matchedCount;
            }
        }

        map.populated = true;
        Msg("[MultiChannel] Torso map: matched %d/%d (head_idle: %s)",
            map.matchedCount, map.totalSlots, map.headIdle ? "found" : "MISSING");
    }
};

enum class JumpPhase : u8 { None = 0, Begin, Airborne, Landing };

struct JumpState
{
    JumpPhase phase{JumpPhase::None};
    float phaseTime{0.0f};
    float airborneMinTime{0.5f};
    bool jumpRequested{false};
    bool landRequested{false};
};

struct CrossfadeState
{
    static constexpr int kChannelCount = static_cast<int>(BodyChannel::Count);
    static constexpr float kDefaultDuration = 0.15f;

    ozz::vector<ozz::math::SoaTransform> frozenLocals[kChannelCount];
    ozz::vector<ozz::math::SoaTransform> blendTemp;
    float fadeTime[kChannelCount]{};
    float fadeDuration{kDefaultDuration};
    bool active[kChannelCount]{};

    void Initialize(int numSoaJoints)
    {
        for (int ch = 0; ch < kChannelCount; ++ch)
            frozenLocals[ch].resize(numSoaJoints);
        blendTemp.resize(numSoaJoints);
    }

    void Snapshot(int channel, const ozz::vector<ozz::math::SoaTransform>& src)
    {
        const size_t count = std::min(frozenLocals[channel].size(), src.size());
        for (size_t i = 0; i < count; ++i)
            frozenLocals[channel][i] = src[i];
        fadeTime[channel] = 0.0f;
        active[channel] = true;
    }

    bool IsFading(int ch) const { return active[ch] && fadeTime[ch] < fadeDuration; }
    float NewWeight(int ch) const { return active[ch] ? std::min(fadeTime[ch] / fadeDuration, 1.0f) : 1.0f; }
};

class MultiChannelSamplingSystem
{
public:
    static void ProcessEntity(
        MultiChannelAnimator& mc,
        CrossfadeState* crossfade,
        const ozz::animation::Skeleton* skeleton,
        AnimationBuffers& buffers,
        float dt)
    {
        static int sDebugFrame = 0;
        ++sDebugFrame;
        const bool debugLog = (sDebugFrame % 60 == 0);

        for (int ch = 0; ch < mc.kChannelCount; ++ch) {
            mc.channels[ch].AdvanceTime(dt);
            bool ok = mc.channels[ch].Sample();
            if (debugLog) {
                Msg("[MC] ch=%d anim=%p playing=%d ratio=%.3f sample=%s locals=%zu",
                    ch, mc.channels[ch].animation, mc.channels[ch].isPlaying,
                    mc.channels[ch].timeRatio, ok ? "OK" : "SKIP", mc.channels[ch].locals.size());
            }
        }

        if (crossfade) {
            for (int ch = 0; ch < mc.kChannelCount; ++ch) {
                if (!crossfade->IsFading(ch))
                    continue;

                crossfade->fadeTime[ch] += dt;
                const float w = crossfade->NewWeight(ch);

                if (w >= 1.0f) {
                    crossfade->active[ch] = false;
                    continue;
                }

                ozz::animation::BlendingJob::Layer layers[2];
                layers[0].transform = ozz::make_span(crossfade->frozenLocals[ch]);
                layers[0].weight = 1.0f - w;
                layers[1].transform = ozz::make_span(mc.channels[ch].locals);
                layers[1].weight = w;

                ozz::animation::BlendingJob fadeJob;
                fadeJob.layers = ozz::span<ozz::animation::BlendingJob::Layer>(layers, 2);
                fadeJob.rest_pose = skeleton->joint_rest_poses();
                fadeJob.output = ozz::make_span(crossfade->blendTemp);
                fadeJob.Run();

                std::copy(crossfade->blendTemp.begin(), crossfade->blendTemp.end(),
                          mc.channels[ch].locals.begin());
            }
        }

        ozz::animation::BlendingJob::Layer blendLayers[MultiChannelAnimator::kChannelCount];
        int layerCount = 0;

        for (int ch = 0; ch < mc.kChannelCount; ++ch) {
            if (!mc.channels[ch].IsInitialized())
                continue;
            blendLayers[layerCount].transform = ozz::make_span(mc.channels[ch].locals);
            blendLayers[layerCount].weight = 1.0f;
            blendLayers[layerCount].joint_weights = ozz::make_span(mc.jointWeights[ch]);
            ++layerCount;
        }

        ozz::animation::BlendingJob blendJob;
        blendJob.layers = ozz::span<ozz::animation::BlendingJob::Layer>(blendLayers, layerCount);
        blendJob.rest_pose = skeleton->joint_rest_poses();
        blendJob.output = ozz::make_span(buffers.locals);
        bool blendOk = blendJob.Run();

        if (debugLog) {
            Msg("[MC] blend: layers=%d output=%zu ok=%s", layerCount, buffers.locals.size(), blendOk ? "YES" : "NO");
        }
    }
};

class JumpInputSystem
{
public:
    static void Update(entt::registry& registry)
    {
        auto& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard)
            return;

        bool spaceDown = ImGui::IsKeyDown(ImGuiKey_Space);

        auto view = registry.view<JumpState, MovementInput>();
        for (auto entity : view) {
            auto& jump = view.get<JumpState>(entity);
            auto& input = view.get<MovementInput>(entity);
            if (!input.enabled)
                continue;

            if (spaceDown && jump.phase == JumpPhase::None)
                jump.jumpRequested = true;
            if (!spaceDown && jump.phase == JumpPhase::Airborne)
                jump.landRequested = true;
        }
    }
};

class JumpStateTransitionSystem
{
public:
    static void Update(entt::registry& registry, float dt)
    {
        auto view = registry.view<JumpState>();
        for (auto entity : view) {
            auto& jump = view.get<JumpState>(entity);

            switch (jump.phase) {
            case JumpPhase::None:
                if (jump.jumpRequested) {
                    jump.phase = JumpPhase::Begin;
                    jump.phaseTime = 0.0f;
                    jump.jumpRequested = false;
                }
                break;
            case JumpPhase::Begin:
                jump.phaseTime += dt;
                {
                    auto* mc = registry.try_get<MultiChannelAnimator>(entity);
                    if (mc && mc->channels[static_cast<int>(BodyChannel::Legs)].Finished()) {
                        jump.phase = JumpPhase::Airborne;
                        jump.phaseTime = 0.0f;
                    }
                }
                break;
            case JumpPhase::Airborne:
                jump.phaseTime += dt;
                if (jump.landRequested || jump.phaseTime > jump.airborneMinTime + 2.0f) {
                    jump.phase = JumpPhase::Landing;
                    jump.phaseTime = 0.0f;
                    jump.landRequested = false;
                }
                break;
            case JumpPhase::Landing:
                jump.phaseTime += dt;
                {
                    auto* mc = registry.try_get<MultiChannelAnimator>(entity);
                    if (mc && mc->channels[static_cast<int>(BodyChannel::Legs)].Finished()) {
                        jump.phase = JumpPhase::None;
                        jump.phaseTime = 0.0f;
                    }
                }
                break;
            }
        }
    }
};

class MultiChannelAnimationSelectionSystem
{
public:
    static void Update(entt::registry& registry)
    {
        auto view = registry.view<MovementInput, MovementAnimationMap, MultiChannelAnimator,
                                  MovementStateMachineState>();

        for (auto entity : view) {
            auto& input = view.get<MovementInput>(entity);
            auto& legsMap = view.get<MovementAnimationMap>(entity);
            auto& mc = view.get<MultiChannelAnimator>(entity);
            auto& smState = view.get<MovementStateMachineState>(entity);

            if (!input.enabled || !legsMap.populated || !mc.initialized)
                continue;

            auto& legsChannel = mc.channels[static_cast<int>(BodyChannel::Legs)];
            auto& torsoChannel = mc.channels[static_cast<int>(BodyChannel::Torso)];

            auto* crossfade = registry.try_get<CrossfadeState>(entity);
            auto* jumpState = registry.try_get<JumpState>(entity);
            auto* torsoMap = registry.try_get<TorsoAnimationMap>(entity);

            const ozz::animation::Animation* legsAnim = nullptr;
            float legsSpeed = 1.0f;
            bool legsLoop = true;

            if (jumpState && jumpState->phase != JumpPhase::None) {
                switch (jumpState->phase) {
                case JumpPhase::Begin:
                    if (legsMap.jumpBegin.animation) {
                        legsAnim = legsMap.jumpBegin.animation;
                        legsSpeed = legsMap.jumpBegin.speed;
                        legsLoop = false;
                    }
                    break;
                case JumpPhase::Airborne:
                    if (legsMap.jumpIdle.animation) {
                        legsAnim = legsMap.jumpIdle.animation;
                        legsSpeed = legsMap.jumpIdle.speed;
                        legsLoop = true;
                    }
                    break;
                case JumpPhase::Landing:
                    if (legsMap.landingLight.animation) {
                        legsAnim = legsMap.landingLight.animation;
                        legsSpeed = legsMap.landingLight.speed;
                        legsLoop = false;
                    }
                    break;
                default: break;
                }
            }

            if (!legsAnim) {
                const auto* entry = legsMap.Resolve(input.direction, input.posture, input.speed);
                if (entry && entry->animation) {
                    legsAnim = entry->animation;
                    legsSpeed = entry->speed;
                }
            }

            if (legsAnim && legsChannel.animation != legsAnim) {
                if (crossfade && legsChannel.IsInitialized())
                    crossfade->Snapshot(static_cast<int>(BodyChannel::Legs), legsChannel.locals);
                legsChannel.SetAnimation(legsAnim, legsSpeed, legsLoop);
                smState.animationChanged = true;
                Msg("[MC-Select] Legs -> %s (speed=%.2f loop=%d)",
                    legsAnim->name() ? legsAnim->name() : "?", legsSpeed, legsLoop);
            } else {
                smState.animationChanged = false;
            }

            smState.lastDirection = input.direction;
            smState.lastPosture = input.posture;
            smState.lastSpeed = input.speed;
            if (legsChannel.animation) {
                smState.lastAnimationName = legsChannel.animation->name();
            }

            if (torsoMap && torsoMap->populated) {
                float torsoSpeed = 1.0f;
                const auto* torsoAnim = torsoMap->Resolve(
                    torsoMap->activeSlot, input.speed, input.direction, &torsoSpeed);
                if (torsoAnim && torsoChannel.animation != torsoAnim) {
                    if (crossfade && torsoChannel.IsInitialized())
                        crossfade->Snapshot(static_cast<int>(BodyChannel::Torso), torsoChannel.locals);
                    torsoChannel.SetAnimation(torsoAnim, torsoSpeed);
                }
            }
        }
    }
};

inline const char* JumpPhaseName(JumpPhase p)
{
    switch (p) {
    case JumpPhase::None:     return "None";
    case JumpPhase::Begin:    return "Begin";
    case JumpPhase::Airborne: return "Airborne";
    case JumpPhase::Landing:  return "Landing";
    }
    return "?";
}

inline const char* ChannelName(int ch)
{
    switch (static_cast<BodyChannel>(ch)) {
    case BodyChannel::Legs:  return "Legs";
    case BodyChannel::Torso: return "Torso";
    case BodyChannel::Head:  return "Head";
    default: return "?";
    }
}

inline void DrawMovementStateMachinePanel(entt::registry& registry, bool& showPanel)
{
    if (!showPanel)
        return;
    if (!ImGui::Begin("Movement State Machine", &showPanel)) {
        ImGui::End();
        return;
    }

    auto view = registry.view<MovementInput, MovementAnimationMap, MovementStateMachineState>();
    if (view.begin() == view.end()) {
        ImGui::TextColored(ImVec4(1.f, 1.f, 0.f, 1.f), "No entities with movement components");
        ImGui::End();
        return;
    }

    auto entity = *view.begin();
    auto& input = view.get<MovementInput>(entity);
    auto& animMap = view.get<MovementAnimationMap>(entity);

    ImGui::SeparatorText("Input");
    ImGui::Text("Direction: %s", DirectionName(input.direction));
    ImGui::Text("Posture:   %s", PostureName(input.posture));
    ImGui::Text("Speed:     %s", SpeedName(input.speed));

    auto* jumpState = registry.try_get<JumpState>(entity);
    if (jumpState) {
        ImGui::Text("Jump:      %s (%.1fs)", JumpPhaseName(jumpState->phase), jumpState->phaseTime);
    }

    ImGui::SeparatorText("Legs Animation Map");
    ImGui::Text("Matched: %d / %d", animMap.matchedCount, animMap.totalSlots);

    auto* mc = registry.try_get<MultiChannelAnimator>(entity);
    if (mc && mc->initialized) {
        ImGui::SeparatorText("Channels");
        for (int ch = 0; ch < mc->kChannelCount; ++ch) {
            auto& chan = mc->channels[ch];
            const char* animName = (chan.animation && chan.animation->name() && chan.animation->name()[0])
                ? chan.animation->name() : "<none>";
            ImGui::Text("%s: %s (t=%.2f)", ChannelName(ch), animName, chan.timeRatio);
        }

        ImGui::Text("Partition: torso_root=%d head_root=%d", mc->torsoRootJoint, mc->headRootJoint);
    }

    auto* torsoMap = registry.try_get<TorsoAnimationMap>(entity);
    if (torsoMap) {
        ImGui::SeparatorText("Torso");
        ImGui::SliderInt("Weapon Slot", &torsoMap->activeSlot, 0, TorsoAnimationMap::kMaxSlots - 1);
        ImGui::Text("Torso map: %d / %d matched", torsoMap->matchedCount, torsoMap->totalSlots);
    }

    auto* crossfade = registry.try_get<CrossfadeState>(entity);
    if (crossfade) {
        ImGui::SeparatorText("Crossfade");
        ImGui::SliderFloat("Fade Duration", &crossfade->fadeDuration, 0.01f, 1.0f, "%.2fs");
        for (int ch = 0; ch < CrossfadeState::kChannelCount; ++ch) {
            if (crossfade->active[ch]) {
                float progress = crossfade->fadeTime[ch] / crossfade->fadeDuration;
                ImGui::ProgressBar(progress, ImVec2(-1, 0), ChannelName(ch));
            }
        }
    }

    ImGui::Separator();
    ImGui::Checkbox("Enabled", &input.enabled);
    ImGui::TextDisabled("W/S/A/D = Move | Shift = Run | Ctrl = Crouch | Space = Jump");

    ImGui::End();
}

} // namespace AnimationECS
