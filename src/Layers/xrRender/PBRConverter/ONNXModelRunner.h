// xrRender/PBRConverter/ONNXModelRunner.h
// ONNX Runtime wrapper for AI-based PBR texture generation
//
// PURPOSE:
// Provides C++ interface to run ONNX models exported from the ai-pbr-models Python project.
// Handles model loading, inference, and tensor management.
//
// ARCHITECTURE:
// - ONNXModelRunner: Generic ONNX model executor (single model)
// - PBRPipeline: Orchestrates 2-stage AI pipeline (SegFormer → UNets)
//
// MODELS USED:
// Stage 1: SegFormer (material classification) + UNetAlbedo (basecolor generation)
// Stage 2: 5x UNetSingleChannel (metallic, roughness, AO, parallax, normal)

#pragma once

#include "xrCore/xrCore.h"
#include <memory>
#include <vector>

// ONNX Runtime headers (required for std::unique_ptr<Ort::...>)
#ifdef USE_AI_PBR
#include <onnxruntime_cxx_api.h>
#else
// Forward declarations when ONNX Runtime not available
namespace Ort {
    class Env;
    class Session;
    class MemoryInfo;
    struct Value;
}
#endif

namespace xray::render::pbr {

// ══════════════════════════════════════════════════════════
//  TENSOR DATA (Input/Output for ONNX models)
// ══════════════════════════════════════════════════════════

struct Tensor {
    std::vector<float> data;        // Flattened tensor data (NCHW format)
    std::vector<int64_t> shape;     // [batch, channels, height, width]

    u32 batch() const { return shape.size() > 0 ? static_cast<u32>(shape[0]) : 1; }
    u32 channels() const { return shape.size() > 1 ? static_cast<u32>(shape[1]) : 0; }
    u32 height() const { return shape.size() > 2 ? static_cast<u32>(shape[2]) : 0; }
    u32 width() const { return shape.size() > 3 ? static_cast<u32>(shape[3]) : 0; }
    u32 size() const { return static_cast<u32>(data.size()); }

    // Create tensor with given shape (allocated but uninitialized)
    static Tensor Create(u32 batch, u32 channels, u32 height, u32 width);

    // Create tensor from image data (RGB or single-channel)
    static Tensor FromImageData(const u8* pixels, u32 width, u32 height, u32 channels);

    // Convert tensor to u8 image data (CHW → HWC format, float [0-1] → u8 [0-255])
    // apply_sigmoid: Apply sigmoid activation before conversion (matches Python torch.sigmoid())
    xr_vector<u8> ToImageData(bool apply_sigmoid = false) const;
};

// ══════════════════════════════════════════════════════════
//  ONNX MODEL RUNNER (Single Model Executor)
// ══════════════════════════════════════════════════════════

class ONNXModelRunner {
public:
    ONNXModelRunner() = default;
    ~ONNXModelRunner();

    // Non-copyable (owns ONNX session resources)
    ONNXModelRunner(const ONNXModelRunner&) = delete;
    ONNXModelRunner& operator=(const ONNXModelRunner&) = delete;

    // Move-only
    ONNXModelRunner(ONNXModelRunner&& other) noexcept;
    ONNXModelRunner& operator=(ONNXModelRunner&& other) noexcept;

    // ═══════════════════════════════════════════════════════
    //  INITIALIZATION
    // ═══════════════════════════════════════════════════════

    // Initialize ONNX Runtime environment (call once per application)
    static bool InitializeEnvironment();
    static void ShutdownEnvironment();

    // Load ONNX model from file
    // model_path: Absolute path to .onnx file (e.g., "$game_data$/ai_models/segformer.onnx")
    // use_gpu: Try to use DirectML (Windows) or CUDA (Linux) if available
    // trt_cache_path: Path to TensorRT engine cache directory (optional, uses current dir if empty)
    bool LoadModel(const char* model_path, bool use_gpu = true, const char* trt_cache_path = "");

    // Check if model is loaded and ready
    bool IsLoaded() const;

    // Get model information
    xr_string GetModelPath() const { return model_path_; }
    xr_vector<xr_string> GetInputNames() const;
    xr_vector<xr_string> GetOutputNames() const;

    // ═══════════════════════════════════════════════════════
    //  INFERENCE
    // ═══════════════════════════════════════════════════════

    // Run inference with single input → single output
    Tensor Run(const Tensor& input);

    // Run inference with multiple inputs → multiple outputs
    // input_names/output_names must match model's expected names
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

    // Cached model metadata
    xr_vector<xr_string> input_names_;
    xr_vector<xr_string> output_names_;

    // Helper: Convert Tensor → Ort::Value
    Ort::Value TensorToOrtValue(const Tensor& tensor);

    // Helper: Convert Ort::Value → Tensor
    Tensor OrtValueToTensor(Ort::Value& ort_value);
};

// ══════════════════════════════════════════════════════════
//  PREPROCESSING UTILITIES
// ══════════════════════════════════════════════════════════

class PreprocessingUtils {
public:
    // ImageNet STANDARD normalization (for UNets)
    // mean = [0.5, 0.5, 0.5], std = [0.5, 0.5, 0.5]
    // Used by: UNetAlbedo, UNetSingleChannel models
    static void NormalizeImageNetStandard(Tensor& tensor);

    // ImageNet DEFAULT normalization (for SegFormer)
    // mean = [0.485, 0.456, 0.406], std = [0.229, 0.224, 0.225]
    // Used by: SegFormer model (requires standard ImageNet stats)
    static void NormalizeImageNetDefault(Tensor& tensor);

    // Legacy ImageNet normalization (defaults to DEFAULT for backwards compatibility)
    static void NormalizeImageNet(Tensor& tensor);

    // Compute mean curvature map from normal map (3-channel → 1-channel)
    // Uses Sobel gradients on normal channels
    static Tensor ComputeMeanCurvature(const Tensor& normal_map);

    // Compute coarse height map from normal map via Poisson reconstruction
    // Uses FFT-based integration (requires normal_x, normal_y, normal_z)
    static Tensor ComputePoissonCoarse(const Tensor& normal_map);

    // Convert material logits → one-hot mask (for metallic/roughness conditioning)
    // logits: [1, num_classes, H, W] → mask: [1, num_classes, H, W] (softmax + threshold)
    static Tensor LogitsToMask(const Tensor& logits, u32 num_classes);

    // Upsample tensor to target size (bilinear interpolation)
    static Tensor Upsample(const Tensor& input, u32 target_height, u32 target_width);
};

// ══════════════════════════════════════════════════════════
//  AI PBR PIPELINE (2-Stage Inference)
// ══════════════════════════════════════════════════════════

struct PBRPipelineConfig {
    xr_string model_dir = "ai_models";  // Directory containing ONNX models (relative to $game_data$)
    bool use_gpu = true;
    bool verbose = false;  // Print inference timing
};

struct PBRPipelineOutputs {
    Tensor albedo;      // [1, 3, H, W] - RGB basecolor
    Tensor metallic;    // [1, 1, H, W] - Single-channel metallic
    Tensor roughness;   // [1, 1, H, W] - Single-channel roughness
    Tensor ao;          // [1, 1, H, W] - Single-channel ambient occlusion
    Tensor parallax;    // [1, 1, H, W] - Single-channel height/parallax
    Tensor normal;      // [1, 3, H, W] - RGB normal (XYZ)

    bool success = false;
};

class PBRPipeline {
public:
    PBRPipeline() = default;
    ~PBRPipeline() = default;

    // ═══════════════════════════════════════════════════════
    //  INITIALIZATION
    // ═══════════════════════════════════════════════════════

    // Load all models (must be called before Process)
    // Returns false if any model fails to load
    bool Initialize(const PBRPipelineConfig& config);

    // Check if pipeline is ready
    bool IsInitialized() const { return initialized_; }

    // ═══════════════════════════════════════════════════════
    //  INFERENCE
    // ═══════════════════════════════════════════════════════

    // Process legacy textures → PBR outputs
    // diffuse: RGB image [H, W, 3]
    // normal: RGB normal map [H, W, 3] (XYZ format)
    PBRPipelineOutputs Process(const u8* diffuse, const u8* normal, u32 width, u32 height);

    // Get model directory
    xr_string GetModelDir() const { return config_.model_dir; }

private:
    PBRPipelineConfig config_;
    bool initialized_ = false;

    // Model paths (models loaded on-demand to save VRAM)
    xr_string segformer_path_;
    xr_string unet_albedo_path_;
    xr_string unet_albedo_uncond_path_;  // Unconditional albedo model (first pass)
    xr_string unet_parallax_path_;
    xr_string unet_ao_path_;
    xr_string unet_metallic_path_;
    xr_string unet_roughness_path_;

    // TensorRT engine cache path
    xr_string trt_cache_path_;

    // Helper: Stage 1 - Generate albedo + material features
    struct Stage1Outputs {
        Tensor albedo;          // [1, 3, H, W]
        Tensor material_logits; // [1, 6, H/4, W/4]
        Tensor seg_features;    // [1, 512, H/16, W/16]
        bool success = false;
    };
    Stage1Outputs RunStage1(const Tensor& diffuse, const Tensor& normal);

    // Helper: Stage 2 - Generate PBR maps
    struct Stage2Outputs {
        Tensor metallic;
        Tensor roughness;
        Tensor ao;
        Tensor parallax;
        bool success = false;
    };
    Stage2Outputs RunStage2(
        const Tensor& albedo,
        const Tensor& normal,
        const Tensor& material_logits,
        const Tensor& seg_features
    );
};

// ══════════════════════════════════════════════════════════
//  HELPER: Check if AI models are available
// ══════════════════════════════════════════════════════════

bool AreAIModelsAvailable(const char* model_dir = "ai_models");

} // namespace xray::render::pbr
