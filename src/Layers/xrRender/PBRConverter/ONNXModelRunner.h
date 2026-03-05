#pragma once

#include "xrCore/xrCore.h"
#include <memory>
#include <vector>

#ifdef USE_AI_PBR
#include <onnxruntime_cxx_api.h>
#else
namespace Ort {
    class Env;
    class Session;
    class MemoryInfo;
    struct Value;
}
#endif

namespace xray::render::pbr {

struct Tensor {
    std::vector<float> data;
    std::vector<int64_t> shape;

    u32 batch() const { return shape.size() > 0 ? static_cast<u32>(shape[0]) : 1; }
    u32 channels() const { return shape.size() > 1 ? static_cast<u32>(shape[1]) : 0; }
    u32 height() const { return shape.size() > 2 ? static_cast<u32>(shape[2]) : 0; }
    u32 width() const { return shape.size() > 3 ? static_cast<u32>(shape[3]) : 0; }
    u32 size() const { return static_cast<u32>(data.size()); }

    static Tensor Create(u32 batch, u32 channels, u32 height, u32 width);

    static Tensor FromImageData(const u8* pixels, u32 width, u32 height, u32 channels);

    xr_vector<u8> ToImageData(bool apply_sigmoid = false) const;
};

class ONNXModelRunner {
public:
    ONNXModelRunner() = default;
    ~ONNXModelRunner();

    ONNXModelRunner(const ONNXModelRunner&) = delete;
    ONNXModelRunner& operator=(const ONNXModelRunner&) = delete;

    ONNXModelRunner(ONNXModelRunner&& other) noexcept;
    ONNXModelRunner& operator=(ONNXModelRunner&& other) noexcept;

    static bool InitializeEnvironment();
    static void ShutdownEnvironment();

    bool LoadModel(const char* model_path, bool use_gpu = true, const char* trt_cache_path = "",
                   const char* trt_profile_min = nullptr, const char* trt_profile_max = nullptr, const char* trt_profile_opt = nullptr);

    bool IsLoaded() const;

    xr_string GetModelPath() const { return model_path_; }
    xr_vector<xr_string> GetInputNames() const;
    xr_vector<xr_string> GetOutputNames() const;

    Tensor Run(const Tensor& input);

    xr_vector<Tensor> Run(
        const xr_vector<Tensor>& inputs,
        const xr_vector<const char*>& input_names,
        const xr_vector<const char*>& output_names
    );

private:
    static std::unique_ptr<Ort::Env> env_;
    static bool env_initialized_;

    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::MemoryInfo> memory_info_;

    xr_string model_path_;
    bool use_gpu_ = false;

    xr_vector<xr_string> input_names_;
    xr_vector<xr_string> output_names_;

    Ort::Value TensorToOrtValue(const Tensor& tensor);

    Tensor OrtValueToTensor(Ort::Value& ort_value);
};

class PreprocessingUtils {
public:
    static void NormalizeImageNetStandard(Tensor& tensor);

    static void NormalizeImageNetDefault(Tensor& tensor);

    static void NormalizeImageNet(Tensor& tensor);

    static Tensor ComputeMeanCurvature(const Tensor& normal_map);

    static Tensor ComputePoissonCoarse(const Tensor& normal_map);

    static Tensor LogitsToMask(const Tensor& logits, u32 num_classes);

    static Tensor Upsample(const Tensor& input, u32 target_height, u32 target_width);
};

struct PBRPipelineConfig {
    xr_string model_dir = "ai_models";
    bool use_gpu = true;
    bool verbose = false;
};

struct PBRPipelineOutputs {
    Tensor albedo;
    Tensor metallic;
    Tensor roughness;
    Tensor ao;
    Tensor parallax;
    Tensor normal;

    bool success = false;
};

class PBRPipeline {
public:
    PBRPipeline() = default;
    ~PBRPipeline() = default;

    bool Initialize(const PBRPipelineConfig& config);

    bool IsInitialized() const { return initialized_; }

    PBRPipelineOutputs Process(const u8* diffuse, const u8* normal, u32 width, u32 height);

    struct Stage1Result {
        Tensor albedo;
        Tensor normal;
        Tensor material_logits;
        Tensor seg_features;
        Tensor diffuse_rgba_original;
        bool success = false;
    };

    Stage1Result ProcessStage1(const u8* diffuse, const u8* normal, u32 width, u32 height);
    PBRPipelineOutputs ProcessStage2(Stage1Result& stage1);
    bool NeedsSplitProcessing(u32 width, u32 height) const;
    void WarmupTRTEngines(const xr_vector<std::pair<u32, u32>>& dimensions);

    xr_string GetModelDir() const { return config_.model_dir; }

private:
    PBRPipelineConfig config_;
    bool initialized_ = false;

    xr_string segformer_path_;
    xr_string unet_albedo_path_;
    xr_string unet_albedo_uncond_path_;
    xr_string unet_parallax_path_;
    xr_string unet_ao_path_;
    xr_string unet_metallic_path_;
    xr_string unet_roughness_path_;
    xr_string trt_cache_path_;

    ONNXModelRunner cached_unet_albedo_uncond_;
    ONNXModelRunner cached_segformer_;
    ONNXModelRunner cached_unet_albedo_;
    ONNXModelRunner cached_unet_parallax_;
    ONNXModelRunner cached_unet_ao_;
    ONNXModelRunner cached_unet_metallic_;
    ONNXModelRunner cached_unet_roughness_;
    u32 cached_H_ = 0;
    u32 cached_W_ = 0;

    enum class LoadedStage { None, Stage1, Stage2, All };
    LoadedStage loaded_stage_ = LoadedStage::None;

    static constexpr u32 MIN_AI_RESOLUTION = 16;
    static constexpr u32 VRAM_SPLIT_THRESHOLD = 512 * 1024;
    static constexpr u32 VRAM_SINGLE_MODEL_THRESHOLD = 2048 * 1024;

    struct Stage1Outputs {
        Tensor albedo;
        Tensor material_logits;
        Tensor seg_features;
        bool success = false;
    };

    struct Stage2Outputs {
        Tensor metallic;
        Tensor roughness;
        Tensor ao;
        Tensor parallax;
        bool success = false;
    };

    void ResetAllCachedModels();
    bool EnsureStage1ModelsLoaded(u32 H, u32 W);
    bool EnsureStage2ModelsLoaded(u32 H, u32 W);
    bool EnsureAllModelsLoaded(u32 H, u32 W);

    Stage1Outputs RunStage1(const Tensor& diffuse, const Tensor& normal);
    Stage2Outputs RunStage2(
        const Tensor& albedo,
        const Tensor& normal,
        const Tensor& material_logits,
        const Tensor& seg_features
    );
    Stage2Outputs RunStage2SingleModel(
        const Tensor& albedo,
        const Tensor& normal,
        const Tensor& material_logits,
        const Tensor& seg_features
    );
    bool NeedsSingleModelProcessing(u32 width, u32 height) const;
};

bool AreAIModelsAvailable(const char* model_dir = "ai_models");

}
