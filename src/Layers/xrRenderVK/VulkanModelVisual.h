#pragma once

#include "GpuModel.h"
#include "GpuLevel.h"
#include "VulkanKinematics.h"
#include "Include/xrRender/RenderVisual.h"
#include "xrEngine/vis_common.h"

#include <memory>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace xray::render::vulkan
{
// A renderer-owned OGF instance. Embedded children own their instance state;
// linked children belong to the loaded level and are never deleted here.
class VulkanModelVisual final : public IRenderVisual
{
public:
    VulkanModelVisual(const VisualRecord& record, std::string cache_name, std::shared_ptr<GpuModel> gpu,
        GpuLevel* level = nullptr)
        : gpu_(std::move(gpu)), cache_name_(std::move(cache_name)), type_(record.type),
          linked_(record.linked_children), level_(level), level_revision_(level ? level->revision() : 0)
    {
        visibility_.clear();
        const auto& b = record.bounds;
        visibility_.box.set(b[0], b[1], b[2], b[3], b[4], b[5]);
        visibility_.sphere.P.set(b[6], b[7], b[8]);
        visibility_.sphere.R = b[9];
        initial_box_ = visibility_.box;
        initial_sphere_ = visibility_.sphere;
    }

    // Every copy has independent bounds and child identities, while GPU
    // buffers and materials remain alive until the final copy is retired.
    explicit VulkanModelVisual(const VulkanModelVisual& other)
        : gpu_(other.gpu_), lod_gpu_(other.lod_gpu_), lod_normals_(other.lod_normals_),
          cache_name_(other.cache_name_), type_(other.type_), linked_(other.linked_),
          level_(other.level_), level_revision_(other.level_revision_)
    {
        visibility_.clear();
        visibility_.box = other.visibility_.box;
        visibility_.sphere = other.visibility_.sphere;
        initial_box_ = other.initial_box_;
        initial_sphere_ = other.initial_sphere_;
        if (other.skeleton_)
            skeleton_ = std::make_unique<VulkanKinematics>(*other.skeleton_, this);
        for (const auto& child : other.children_)
            children_.push_back(std::make_unique<VulkanModelVisual>(*child));
    }

    vis_data& getVisData() override
    {
        return visibility_;
    }
    const vis_data& visibility() const { return visibility_; }

    u32 getType() const override
    {
        return type_;
    }

    IRenderVisual* getSubModel(u8 index) override
    {
        if (index < children_.size()) return children_[index].get();
        const size_t linked_index = size_t(index) - children_.size();
        return linked_valid() && linked_index < linked_.size() ?
            level_->get_visual(linked_[linked_index]) : nullptr;
    }

    void add_child(std::unique_ptr<VulkanModelVisual> child) { children_.push_back(std::move(child));
    }

    void reset_instance_state()
    {
        visibility_.clear();
        visibility_.reset_object_data();
        visibility_.box = initial_box_;
        visibility_.sphere = initial_sphere_;
        if (skeleton_)
            skeleton_->reset_instance_state();
        for (auto& child : children_)
            child->reset_instance_state();
    }

    void release_pose_buffers(IKinematics* owner = nullptr)
    {
        if (skeleton_)
            owner = skeleton_.get();
        if (owner && gpu_)
            gpu_->release_instance(owner);
        for (auto& child : children_)
            child->release_pose_buffers(owner);
    }
    const auto& children() const { return children_; }
    const auto& linked() const { return linked_; }
    bool linked_valid() const { return level_ && level_->revision() == level_revision_; }
    bool has_gpu() const { return !!gpu_; }
    void set_lod(std::array<std::shared_ptr<GpuModel>, 8> gpu,
        std::array<std::array<float, 3>, 8> normals)
    {
        lod_gpu_ = std::move(gpu);
        lod_normals_ = normals;
    }
    bool has_lod() const { return !!lod_gpu_[0]; }
    GpuModel& lod_gpu(uint8_t facet) const { return *lod_gpu_[facet]; }
    const auto& lod_normals() const { return lod_normals_; }
    void set_skeleton(std::unique_ptr<VulkanKinematics> skeleton) { skeleton_ = std::move(skeleton); }
    IKinematics* dcast_PKinematics() override { return skeleton_.get(); }
    VulkanKinematics* skeleton() const { return skeleton_.get(); }
    const VulkanModelVisual* find(const IRenderVisual* visual) const
    {
        if (this == visual) return this;
        for (const auto& child : children_)
            if (const auto* found = child->find(visual)) return found;
        return nullptr;
    }

    GpuModel& gpu()
    {
        return *gpu_;
    }
    GpuModel& gpu() const { return *gpu_; }

    const std::string& cache_name() const
    {
        return cache_name_;
    }

    void set_cache_name(std::string name)
    {
        cache_name_ = std::move(name);
    }
#ifdef DEBUG
    shared_str getDebugName() override
    {
        return "vulkan_static_ogf";
    }
#endif

private:
    vis_data visibility_;
    Fbox initial_box_;
    Fsphere initial_sphere_;
    std::shared_ptr<GpuModel> gpu_;
    std::array<std::shared_ptr<GpuModel>, 8> lod_gpu_{};
    std::array<std::array<float, 3>, 8> lod_normals_{};
    std::unique_ptr<VulkanKinematics> skeleton_;
    std::string cache_name_;
    u32 type_{};
    std::vector<std::unique_ptr<VulkanModelVisual>> children_;
    std::vector<uint32_t> linked_;
    GpuLevel* level_{};
    uint64_t level_revision_{};
};
} // namespace xray::render::vulkan
