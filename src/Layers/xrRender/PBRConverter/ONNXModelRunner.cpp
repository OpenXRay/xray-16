// xrRender/PBRConverter/ONNXModelRunner.cpp
// ONNX Runtime integration for AI-based PBR conversion

#include "stdafx.h"
#include "ONNXModelRunner.h"
#include "xrCore/FS.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

#ifdef USE_AI_PBR
// ONNX Runtime C++ API
#include <onnxruntime_cxx_api.h>

namespace xray::render::pbr {

// ══════════════════════════════════════════════════════════
//  STATIC MEMBERS
// ══════════════════════════════════════════════════════════

std::unique_ptr<Ort::Env> ONNXModelRunner::env_;
bool ONNXModelRunner::env_initialized_ = false;

// ══════════════════════════════════════════════════════════
//  TENSOR IMPLEMENTATION
// ══════════════════════════════════════════════════════════

Tensor Tensor::Create(u32 batch, u32 channels, u32 height, u32 width) {
    Tensor tensor;
    tensor.shape = {
        static_cast<int64_t>(batch),
        static_cast<int64_t>(channels),
        static_cast<int64_t>(height),
        static_cast<int64_t>(width)
    };
    const size_t total_size = batch * channels * height * width;
    tensor.data.resize(total_size, 0.0f);
    return tensor;
}

Tensor Tensor::FromImageData(const u8* pixels, u32 width, u32 height, u32 channels) {
    Msg("~ [Tensor::FromImageData] Creating tensor from image: width=%u, height=%u, channels=%u",
        width, height, channels);
    Msg("~   Will create tensor shape: [1, %u, %u, %u] (NCHW)", channels, height, width);

    Tensor tensor = Create(1, channels, height, width);

    // Convert u8 [0-255] → float [0.0-1.0] and transpose HWC → CHW
    // Assumes pixels are in row-major HWC format with 'width' pixels per row
    for (u32 c = 0; c < channels; ++c) {
        for (u32 h = 0; h < height; ++h) {
            for (u32 w = 0; w < width; ++w) {
                const u32 src_idx = (h * width + w) * channels + c;  // HWC
                const u32 dst_idx = c * height * width + h * width + w;  // CHW
                tensor.data[dst_idx] = static_cast<float>(pixels[src_idx]) / 255.0f;
            }
        }
    }

    return tensor;
}

xr_vector<u8> Tensor::ToImageData(bool apply_sigmoid) const {
    const u32 C = channels();
    const u32 H = height();
    const u32 W = width();

    xr_vector<u8> pixels(C * H * W);

    // DEBUG: Compute tensor statistics
    float min_val = FLT_MAX, max_val = -FLT_MAX, sum_val = 0.0f;
    for (const float& val : data) {
        min_val = std::min(min_val, val);
        max_val = std::max(max_val, val);
        sum_val += val;
    }
    float mean_val = sum_val / static_cast<float>(data.size());

    Msg("~ [Tensor::ToImageData] Shape: [%u,%u,%u,%u], Range: [%.4f, %.4f], Mean: %.4f, Sigmoid: %s",
        batch(), C, H, W, min_val, max_val, mean_val, apply_sigmoid ? "YES" : "NO");

    // Convert CHW → HWC and float → u8 [0-255]
    for (u32 c = 0; c < C; ++c) {
        for (u32 h = 0; h < H; ++h) {
            for (u32 w = 0; w < W; ++w) {
                const u32 src_idx = c * H * W + h * W + w;  // CHW
                const u32 dst_idx = (h * W + w) * C + c;    // HWC

                float value = data[src_idx];

                // Apply sigmoid if requested (matching Python: torch.sigmoid())
                if (apply_sigmoid) {
                    value = 1.0f / (1.0f + std::exp(-value));
                }

                // Clamp to [0, 1] and convert to [0, 255] (matching Python: .clamp(0, 1))
                value = std::clamp(value, 0.0f, 1.0f);
                pixels[dst_idx] = static_cast<u8>(value * 255.0f);
            }
        }
    }

    return pixels;
}

// ══════════════════════════════════════════════════════════
//  ONNX MODEL RUNNER IMPLEMENTATION
// ══════════════════════════════════════════════════════════

// Custom logger callback to forward ONNX Runtime/TensorRT logs to X-Ray's Msg() system
static void ORT_API_CALL ONNXLoggerCallback(void* param, OrtLoggingLevel severity, const char* category,
                                             const char* logger_id, const char* code_location, const char* message) {
    // Map ONNX Runtime severity levels to X-Ray log prefixes
    const char* prefix = "";
    switch (severity) {
        case ORT_LOGGING_LEVEL_VERBOSE:
            prefix = "~"; // Verbose/debug
            break;
        case ORT_LOGGING_LEVEL_INFO:
            prefix = "*"; // Info
            break;
        case ORT_LOGGING_LEVEL_WARNING:
            prefix = "!"; // Warning
            break;
        case ORT_LOGGING_LEVEL_ERROR:
        case ORT_LOGGING_LEVEL_FATAL:
            prefix = "!!!"; // Error
            break;
    }

    // Forward to X-Ray logging system
    // Format: [category] message
    if (category && category[0] != '\0' && severity >= ORT_LOGGING_LEVEL_ERROR) {
        Msg("%s [%s] %s", prefix, category, message);
    }
}

bool ONNXModelRunner::InitializeEnvironment() {
    if (env_initialized_) {
        return true;
    }

    try {
        // Create environment with custom logger callback
        // This captures ALL ONNX Runtime and TensorRT logs
        OrtLoggingFunction logging_function = ONNXLoggerCallback;
        env_ = std::make_unique<Ort::Env>(
            ORT_LOGGING_LEVEL_ERROR,  // Capture everything
            "OpenXRay_PBR",
            logging_function,
            nullptr  // No user data needed
        );
        env_initialized_ = true;
        Msg("[ONNXModelRunner] ONNX Runtime environment initialized with custom logger");
        return true;
    }
    catch (const Ort::Exception& e) {
        Msg("! [ONNXModelRunner] Failed to initialize ONNX Runtime: %s", e.what());
        return false;
    }
}

void ONNXModelRunner::ShutdownEnvironment() {
    if (env_initialized_) {
        env_.reset();
        env_initialized_ = false;
        Msg("[ONNXModelRunner] ONNX Runtime environment shut down");
    }
}

ONNXModelRunner::~ONNXModelRunner() {
    session_.reset();
    memory_info_.reset();
}

ONNXModelRunner::ONNXModelRunner(ONNXModelRunner&& other) noexcept
    : session_(std::move(other.session_))
    , memory_info_(std::move(other.memory_info_))
    , model_path_(std::move(other.model_path_))
    , use_gpu_(other.use_gpu_)
    , input_names_(std::move(other.input_names_))
    , output_names_(std::move(other.output_names_))
{
}

ONNXModelRunner& ONNXModelRunner::operator=(ONNXModelRunner&& other) noexcept {
    if (this != &other) {
        session_ = std::move(other.session_);
        memory_info_ = std::move(other.memory_info_);
        model_path_ = std::move(other.model_path_);
        use_gpu_ = other.use_gpu_;
        input_names_ = std::move(other.input_names_);
        output_names_ = std::move(other.output_names_);
    }
    return *this;
}

bool ONNXModelRunner::LoadModel(const char* model_path, bool use_gpu, const char* trt_cache_path) {
    if (!env_initialized_) {
        Msg("! [ONNXModelRunner] Environment not initialized. Call InitializeEnvironment() first.");
        return false;
    }

    // model_path is already a fully resolved path from FS.update_path()
    if (!FS.exist(model_path, FSType::External)) {
        Msg("! [ONNXModelRunner] Model file not found: %s", model_path);
        return false;
    }

    // Create session options
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(std::thread::hardware_concurrency());
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    session_options.SetLogSeverityLevel(1); // Info and above
    session_options.SetExecutionMode(ExecutionMode::ORT_PARALLEL);

    // Try to enable GPU acceleration
    use_gpu_ = false;
    if (use_gpu) {
        // Use TensorRT V2 API for NVIDIA GPUs (has advanced options like build logging)
        Ort::TensorRTProviderOptions trt_options{};

        // Build options map
        std::unordered_map<std::string, std::string> trt_opts;

        // Basic settings
        trt_opts["device_id"] = "0";
        trt_opts["trt_max_workspace_size"] = "2147483648"; // 2GB
        trt_opts["trt_fp16_enable"] = "1"; // Enable FP16 tensor cores

        // Engine caching
        trt_opts["trt_engine_cache_enable"] = "1";
        if (trt_cache_path && trt_cache_path[0] != '\0') {
            trt_opts["trt_engine_cache_path"] = trt_cache_path;
        }

        // Speed up compilation with reduced optimization (level 2 instead of 3)
        trt_opts["trt_builder_optimization_level"] = "2"; // 0-5, default 3 (slower build, best perf)

        // Enable detailed build logging to see progress
        trt_opts["trt_detailed_build_log"] = "1";

        // Use heuristics for faster builds
        trt_opts["trt_build_heuristics_enable"] = "1";

        // Enable timing cache to speed up future builds
        trt_opts["trt_timing_cache_enable"] = "1";
        if (trt_cache_path && trt_cache_path[0] != '\0') {
            trt_opts["trt_timing_cache_path"] = trt_cache_path;
        }

        trt_options.Update(trt_opts);
        session_options.AppendExecutionProvider_TensorRT_V2(*trt_options);
        Msg("[ONNXModelRunner] TensorRT V2 GPU acceleration enabled (FP16, optimization level 2, detailed logging)");
        use_gpu_ = true;
    }

    // Load model - convert to wide string for ONNX Runtime
    std::string str_path(model_path);
    std::wstring wide_path(str_path.begin(), str_path.end());
    session_ = std::make_unique<Ort::Session>(*env_, wide_path.c_str(), session_options);

    // Create memory info
    memory_info_ = std::make_unique<Ort::MemoryInfo>(
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)
    );

    // Cache input/output names
    Ort::AllocatorWithDefaultOptions allocator;

    const size_t num_inputs = session_->GetInputCount();
    input_names_.reserve(num_inputs);
    for (size_t i = 0; i < num_inputs; ++i) {
        auto name_ptr = session_->GetInputNameAllocated(i, allocator);
        input_names_.emplace_back(name_ptr.get());
    }

    const size_t num_outputs = session_->GetOutputCount();
    output_names_.reserve(num_outputs);
    for (size_t i = 0; i < num_outputs; ++i) {
        auto name_ptr = session_->GetOutputNameAllocated(i, allocator);
        output_names_.emplace_back(name_ptr.get());
    }

    model_path_ = model_path;

    Msg("[ONNXModelRunner] Loaded model: %s (%zu inputs, %zu outputs)",
        model_path, num_inputs, num_outputs);

    return true;
}

bool ONNXModelRunner::IsLoaded() const {
    return session_ != nullptr;
}

xr_vector<xr_string> ONNXModelRunner::GetInputNames() const {
    return input_names_;
}

xr_vector<xr_string> ONNXModelRunner::GetOutputNames() const {
    return output_names_;
}

Ort::Value ONNXModelRunner::TensorToOrtValue(const Tensor& tensor) {
    // Create Ort::Value from Tensor data
    return Ort::Value::CreateTensor<float>(
        *memory_info_,
        const_cast<float*>(tensor.data.data()),
        tensor.data.size(),
        tensor.shape.data(),
        tensor.shape.size()
    );
}

Tensor ONNXModelRunner::OrtValueToTensor(Ort::Value& ort_value) {
    Tensor tensor;

    // Get shape
    const auto type_info = ort_value.GetTensorTypeAndShapeInfo();
    tensor.shape = type_info.GetShape();

    // Copy data
    const float* data_ptr = ort_value.GetTensorData<float>();
    const size_t data_size = std::accumulate(
        tensor.shape.begin(),
        tensor.shape.end(),
        1LL,
        std::multiplies<int64_t>()
    );
    tensor.data.assign(data_ptr, data_ptr + data_size);

    return tensor;
}

Tensor ONNXModelRunner::Run(const Tensor& input) {
    if (!IsLoaded()) {
        Msg("! [ONNXModelRunner] Model not loaded");
        return Tensor{};
    }

    try {
        // Create input tensor
        auto input_tensor = TensorToOrtValue(input);

        // Prepare input/output names
        std::vector<const char*> input_names_cstr = { input_names_[0].c_str() };
        std::vector<const char*> output_names_cstr = { output_names_[0].c_str() };

        // Run inference
        auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_names_cstr.data(),
            &input_tensor,
            1,
            output_names_cstr.data(),
            1
        );

        // Convert output
        return OrtValueToTensor(output_tensors[0]);
    }
    catch (const Ort::Exception& e) {
        Msg("! [ONNXModelRunner] Inference failed: %s", e.what());
        return Tensor{};
    }
}

xr_vector<Tensor> ONNXModelRunner::Run(
    const xr_vector<Tensor>& inputs,
    const xr_vector<const char*>& input_names,
    const xr_vector<const char*>& output_names)
{
    if (!IsLoaded()) {
        Msg("! [ONNXModelRunner] Model not loaded");
        return {};
    }

    try {
        // Create input tensors
        xr_vector<Ort::Value> input_tensors;
        input_tensors.reserve(inputs.size());
        for (const auto& tensor : inputs) {
            input_tensors.emplace_back(TensorToOrtValue(tensor));
        }

        // Run inference
        auto output_tensors = session_->Run(
            Ort::RunOptions{nullptr},
            input_names.data(),
            input_tensors.data(),
            inputs.size(),
            output_names.data(),
            output_names.size()
        );

        // Convert outputs
        xr_vector<Tensor> results;
        results.reserve(output_tensors.size());
        for (auto& ort_tensor : output_tensors) {
            results.emplace_back(OrtValueToTensor(ort_tensor));
        }

        return results;
    }
    catch (const Ort::Exception& e) {
        Msg("! [ONNXModelRunner] Inference failed: %s", e.what());
        return {};
    }
}

// ══════════════════════════════════════════════════════════
//  PREPROCESSING UTILITIES
// ══════════════════════════════════════════════════════════

// ImageNet STANDARD normalization (for UNets)
// mean = [0.5, 0.5, 0.5], std = [0.5, 0.5, 0.5]
// Used by: UNetAlbedo, UNetSingleChannel models
void PreprocessingUtils::NormalizeImageNetStandard(Tensor& tensor) {
    constexpr float MEAN[] = {0.5f, 0.5f, 0.5f};
    constexpr float STD[] = {0.5f, 0.5f, 0.5f};

    const u32 H = tensor.height();
    const u32 W = tensor.width();

    // Only normalize first 3 channels (RGB)
    const u32 channels_to_normalize = std::min(tensor.channels(), 3u);

    for (u32 c = 0; c < channels_to_normalize; ++c) {
        for (u32 h = 0; h < H; ++h) {
            for (u32 w = 0; w < W; ++w) {
                const u32 idx = c * H * W + h * W + w;
                tensor.data[idx] = (tensor.data[idx] - MEAN[c]) / STD[c];
            }
        }
    }
}

// ImageNet DEFAULT normalization (for SegFormer)
// mean = [0.485, 0.456, 0.406], std = [0.229, 0.224, 0.225]
// Used by: SegFormer model (requires standard ImageNet stats)
void PreprocessingUtils::NormalizeImageNetDefault(Tensor& tensor) {
    constexpr float MEAN[] = {0.485f, 0.456f, 0.406f};
    constexpr float STD[] = {0.229f, 0.224f, 0.225f};

    const u32 H = tensor.height();
    const u32 W = tensor.width();

    // Only normalize first 3 channels (RGB)
    const u32 channels_to_normalize = std::min(tensor.channels(), 3u);

    for (u32 c = 0; c < channels_to_normalize; ++c) {
        for (u32 h = 0; h < H; ++h) {
            for (u32 w = 0; w < W; ++w) {
                const u32 idx = c * H * W + h * W + w;
                tensor.data[idx] = (tensor.data[idx] - MEAN[c]) / STD[c];
            }
        }
    }
}

// Legacy function - defaults to DEFAULT normalization for backwards compatibility
void PreprocessingUtils::NormalizeImageNet(Tensor& tensor) {
    NormalizeImageNetDefault(tensor);
}

Tensor PreprocessingUtils::ComputeMeanCurvature(const Tensor& normal_map) {
    const u32 H = normal_map.height();
    const u32 W = normal_map.width();

    // Step 1: Apply 3x3 median blur to normals (matching Python: K.filters.median_blur)
    Tensor blurred = Tensor::Create(1, 3, H, W);
    for (u32 c = 0; c < 3; ++c) {
        for (u32 h = 1; h < H - 1; ++h) {
            for (u32 w = 1; w < W - 1; ++w) {
                // Collect 3x3 neighborhood
                float neighbors[9];
                int idx = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        neighbors[idx++] = normal_map.data[c * H * W + (h + dy) * W + (w + dx)];
                    }
                }
                // Median of 9 values
                std::nth_element(neighbors, neighbors + 4, neighbors + 9);
                blurred.data[c * H * W + h * W + w] = neighbors[4];
            }
        }
    }

    // Step 2: Compute Sobel gradients on blurred normals
    Tensor curvature = Tensor::Create(1, 1, H, W);

    constexpr float sobel_x[9] = {
        -1.0f, 0.0f, 1.0f,
        -2.0f, 0.0f, 2.0f,
        -1.0f, 0.0f, 1.0f
    };
    constexpr float sobel_y[9] = {
        -1.0f, -2.0f, -1.0f,
         0.0f,  0.0f,  0.0f,
         1.0f,  2.0f,  1.0f
    };

    for (u32 h = 1; h < H - 1; ++h) {
        for (u32 w = 1; w < W - 1; ++w) {
            // Compute dnx_dx (x-derivative of normal's x component)
            float dnx_dx = 0.0f;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const u32 idx = 0 * H * W + (h + dy) * W + (w + dx);  // Channel 0 (nx)
                    dnx_dx += blurred.data[idx] * sobel_x[(dy + 1) * 3 + (dx + 1)];
                }
            }

            // Compute dny_dy (y-derivative of normal's y component)
            float dny_dy = 0.0f;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const u32 idx = 1 * H * W + (h + dy) * W + (w + dx);  // Channel 1 (ny)
                    dny_dy += blurred.data[idx] * sobel_y[(dy + 1) * 3 + (dx + 1)];
                }
            }

            // Step 3: Mean curvature = 0.5 * (dnx_dx + dny_dy), then absolute value
            // (Matching Python: curv = 0.5 * (dnx_dx + dny_dy); curv = curv.abs())
            curvature.data[h * W + w] = std::abs(0.5f * (dnx_dx + dny_dy));
        }
    }

    // Step 4: Percentile normalize using 99th percentile (matching Python)
    std::vector<float> sorted_values = curvature.data;
    std::sort(sorted_values.begin(), sorted_values.end());
    const size_t p99_idx = static_cast<size_t>(sorted_values.size() * 0.99f);
    const float p99 = std::max(sorted_values[p99_idx], 1e-6f);

    for (auto& val : curvature.data) {
        val = std::clamp(val / p99, 0.0f, 1.0f);
    }

    // Step 5: Remap to [-1, 1] (matching Python: curv = (curv - 0.5) * 2.0)
    for (auto& val : curvature.data) {
        val = (val - 0.5f) * 2.0f;
    }

    return curvature;
}

Tensor PreprocessingUtils::ComputePoissonCoarse(const Tensor& normal_map) {
    const u32 H = normal_map.height();
    const u32 W = normal_map.width();

    // Step 1: Apply 3x3 median blur to normals (matching Python: K.filters.median_blur)
    Tensor blurred = Tensor::Create(1, 3, H, W);
    for (u32 c = 0; c < 3; ++c) {
        for (u32 h = 1; h < H - 1; ++h) {
            for (u32 w = 1; w < W - 1; ++w) {
                // Collect 3x3 neighborhood
                float neighbors[9];
                int idx = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        neighbors[idx++] = normal_map.data[c * H * W + (h + dy) * W + (w + dx)];
                    }
                }
                // Median of 9 values
                std::nth_element(neighbors, neighbors + 4, neighbors + 9);
                blurred.data[c * H * W + h * W + w] = neighbors[4];
            }
        }
    }

    // Step 2: Compute slopes from normals (matching Python)
    Tensor height = Tensor::Create(1, 1, H, W);

    for (u32 h = 0; h < H; ++h) {
        for (u32 w = 0; w < W; ++w) {
            const u32 idx = h * W + w;

            // Get normal components (already normalized in [0,1], convert to [-1,1])
            const float nx = blurred.data[0 * H * W + idx] * 2.0f - 1.0f;
            const float ny = blurred.data[1 * H * W + idx] * 2.0f - 1.0f;
            const float nz = std::max(blurred.data[2 * H * W + idx] * 2.0f - 1.0f, 1e-3f);

            // Compute slopes (matching Python: gx = nx / nz, gy = ny / nz)
            const float grad_x = std::clamp(nx / nz, -10.0f, 10.0f);
            const float grad_y = std::clamp(ny / nz, -10.0f, 10.0f);

            // Simple integration (approximation of FFT Poisson solve)
            float accumulated = 0.0f;
            if (w > 0) accumulated += height.data[h * W + (w - 1)] + grad_x;
            if (h > 0 && w == 0) accumulated += height.data[(h - 1) * W + w] + grad_y;
            height.data[idx] = accumulated * 0.5f;
        }
    }

    // Step 3: Normalize to [0, 1]
    float min_h = *std::min_element(height.data.begin(), height.data.end());
    float max_h = *std::max_element(height.data.begin(), height.data.end());
    float range_h = max_h - min_h;

    if (range_h > 1e-6f) {
        for (auto& val : height.data) {
            val = (val - min_h) / range_h;
        }
    }

    // Step 4: Percentile clipping (matching Python: p1, p99)
    std::vector<float> sorted_values = height.data;
    std::sort(sorted_values.begin(), sorted_values.end());
    const size_t p1_idx = static_cast<size_t>(sorted_values.size() * 0.01f);
    const size_t p99_idx = static_cast<size_t>(sorted_values.size() * 0.99f);
    const float p1 = sorted_values[p1_idx];
    const float p99 = sorted_values[p99_idx];

    for (auto& val : height.data) {
        val = std::clamp(val, p1, p99);
        val = (val - p1) / (p99 - p1 + 1e-6f);
    }

    // Step 5: Gaussian blur (15x15, sigma=5) to smooth the result
    // (matching Python: K.filters.gaussian_blur2d with kernel_size=15, sigma=5)
    Tensor blurred_height = Tensor::Create(1, 1, H, W);
    const int kernel_radius = 7;  // 15x15 kernel
    const float sigma = 5.0f;

    // Gaussian weights
    float kernel[15][15];
    float kernel_sum = 0.0f;
    for (int ky = -kernel_radius; ky <= kernel_radius; ++ky) {
        for (int kx = -kernel_radius; kx <= kernel_radius; ++kx) {
            const float dist_sq = static_cast<float>(kx * kx + ky * ky);
            kernel[ky + kernel_radius][kx + kernel_radius] = std::exp(-dist_sq / (2.0f * sigma * sigma));
            kernel_sum += kernel[ky + kernel_radius][kx + kernel_radius];
        }
    }
    // Normalize kernel
    for (int ky = 0; ky < 15; ++ky) {
        for (int kx = 0; kx < 15; ++kx) {
            kernel[ky][kx] /= kernel_sum;
        }
    }

    // Apply Gaussian blur
    for (u32 h = kernel_radius; h < H - kernel_radius; ++h) {
        for (u32 w = kernel_radius; w < W - kernel_radius; ++w) {
            float sum = 0.0f;
            for (int ky = -kernel_radius; ky <= kernel_radius; ++ky) {
                for (int kx = -kernel_radius; kx <= kernel_radius; ++kx) {
                    sum += height.data[(h + ky) * W + (w + kx)] * kernel[ky + kernel_radius][kx + kernel_radius];
                }
            }
            blurred_height.data[h * W + w] = sum;
        }
    }

    // Step 6: Remap to [-1, 1] (matching Python: (blur_torch - 0.5) * 2.0)
    for (auto& val : blurred_height.data) {
        val = (val - 0.5f) * 2.0f;
    }

    return blurred_height;
}

Tensor PreprocessingUtils::LogitsToMask(const Tensor& logits, u32 num_classes) {
    const u32 H = logits.height();
    const u32 W = logits.width();

    Tensor mask = Tensor::Create(1, num_classes, H, W);

    // Softmax over classes, then threshold
    for (u32 h = 0; h < H; ++h) {
        for (u32 w = 0; w < W; ++w) {
            // Find max logit for softmax stability
            float max_logit = -std::numeric_limits<float>::infinity();
            for (u32 c = 0; c < num_classes; ++c) {
                const u32 idx = c * H * W + h * W + w;
                max_logit = std::max(max_logit, logits.data[idx]);
            }

            // Compute softmax
            float sum_exp = 0.0f;
            for (u32 c = 0; c < num_classes; ++c) {
                const u32 idx = c * H * W + h * W + w;
                sum_exp += std::exp(logits.data[idx] - max_logit);
            }

            // Normalize
            for (u32 c = 0; c < num_classes; ++c) {
                const u32 idx = c * H * W + h * W + w;
                mask.data[idx] = std::exp(logits.data[idx] - max_logit) / sum_exp;
            }
        }
    }

    return mask;
}

Tensor PreprocessingUtils::Upsample(const Tensor& input, u32 target_height, u32 target_width) {
    const u32 C = input.channels();
    const u32 src_H = input.height();
    const u32 src_W = input.width();

    Tensor output = Tensor::Create(1, C, target_height, target_width);

    // Bilinear interpolation
    const float scale_h = static_cast<float>(src_H) / static_cast<float>(target_height);
    const float scale_w = static_cast<float>(src_W) / static_cast<float>(target_width);

    for (u32 c = 0; c < C; ++c) {
        for (u32 h = 0; h < target_height; ++h) {
            for (u32 w = 0; w < target_width; ++w) {
                const float src_h = h * scale_h;
                const float src_w = w * scale_w;

                const u32 h0 = static_cast<u32>(src_h);
                const u32 w0 = static_cast<u32>(src_w);
                const u32 h1 = std::min(h0 + 1, src_H - 1);
                const u32 w1 = std::min(w0 + 1, src_W - 1);

                const float dh = src_h - h0;
                const float dw = src_w - w0;

                // Get source values
                const u32 idx00 = c * src_H * src_W + h0 * src_W + w0;
                const u32 idx01 = c * src_H * src_W + h0 * src_W + w1;
                const u32 idx10 = c * src_H * src_W + h1 * src_W + w0;
                const u32 idx11 = c * src_H * src_W + h1 * src_W + w1;

                const float v00 = input.data[idx00];
                const float v01 = input.data[idx01];
                const float v10 = input.data[idx10];
                const float v11 = input.data[idx11];

                // Bilinear interpolation
                const float v0 = v00 * (1.0f - dw) + v01 * dw;
                const float v1 = v10 * (1.0f - dw) + v11 * dw;
                const float v = v0 * (1.0f - dh) + v1 * dh;

                const u32 out_idx = c * target_height * target_width + h * target_width + w;
                output.data[out_idx] = v;
            }
        }
    }

    return output;
}

// ══════════════════════════════════════════════════════════
//  PBR PIPELINE IMPLEMENTATION
// ══════════════════════════════════════════════════════════

bool PBRPipeline::Initialize(const PBRPipelineConfig& config) {
    config_ = config;

    Msg("[PBRPipeline] Initializing AI models from: $game_data$/%s", config.model_dir.c_str());

    // Initialize ONNX Runtime environment
    if (!ONNXModelRunner::InitializeEnvironment()) {
        return false;
    }

    // Build model paths using VFS and validate they exist
    string_path seg_path, albedo_path, albedo_uncond_path, parallax_path, ao_path, metallic_path, roughness_path;
    FS.update_path(seg_path, "$game_data$", (config.model_dir + "\\segformer.onnx").c_str());
    FS.update_path(albedo_path, "$game_data$", (config.model_dir + "\\unet_albedo.onnx").c_str());
    FS.update_path(albedo_uncond_path, "$game_data$", (config.model_dir + "\\unet_albedo_uncond.onnx").c_str());
    FS.update_path(parallax_path, "$game_data$", (config.model_dir + "\\unet_parallax.onnx").c_str());
    FS.update_path(ao_path, "$game_data$", (config.model_dir + "\\unet_ao.onnx").c_str());
    FS.update_path(metallic_path, "$game_data$", (config.model_dir + "\\unet_metallic.onnx").c_str());
    FS.update_path(roughness_path, "$game_data$", (config.model_dir + "\\unet_roughness.onnx").c_str());

    // Validate all model files exist
    bool all_exist = true;
    all_exist &= FS.exist(seg_path, FSType::External);
    all_exist &= FS.exist(albedo_path, FSType::External);
    all_exist &= FS.exist(albedo_uncond_path, FSType::External);
    all_exist &= FS.exist(parallax_path, FSType::External);
    all_exist &= FS.exist(ao_path, FSType::External);
    all_exist &= FS.exist(metallic_path, FSType::External);
    all_exist &= FS.exist(roughness_path, FSType::External);

    if (!all_exist) {
        Msg("! [PBRPipeline] One or more model files not found");
        return false;
    }

    // Store paths for on-demand loading (saves VRAM - only load when needed)
    segformer_path_ = seg_path;
    unet_albedo_path_ = albedo_path;
    unet_albedo_uncond_path_ = albedo_uncond_path;
    unet_parallax_path_ = parallax_path;
    unet_ao_path_ = ao_path;
    unet_metallic_path_ = metallic_path;
    unet_roughness_path_ = roughness_path;

    // Resolve TensorRT engine cache path using VFS
    string_path cache_path;
    FS.update_path(cache_path, "$game_data$", (config.model_dir + "\\trt_cache").c_str());
    trt_cache_path_ = cache_path;
    Msg("[PBRPipeline] TensorRT cache path: %s", trt_cache_path_.c_str());

    initialized_ = true;
    Msg("[PBRPipeline] All model paths validated (models loaded on-demand)");
    return true;
}

PBRPipeline::Stage1Outputs PBRPipeline::RunStage1(const Tensor& diffuse, const Tensor& normal) {
    Stage1Outputs result;

    const u32 H = diffuse.height();
    const u32 W = diffuse.width();

    if (config_.verbose) {
        Msg("[PBRPipeline] Stage1: Processing %ux%u texture (Two-Pass Albedo)", W, H);
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 1: Normalize diffuse and normal with STANDARD normalization
    // ═══════════════════════════════════════════════════════
    // CRITICAL: UNets expect STANDARD normalization (0.5, 0.5, 0.5)
    // SegFormer expects DEFAULT normalization (0.485, 0.456, 0.406)
    Tensor diffuse_std = Tensor::Create(1, 3, H, W);
    Tensor normal_std = Tensor::Create(1, 3, H, W);

    // Copy diffuse and normal (to avoid modifying originals)
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            diffuse_std.data[c * H * W + i] = diffuse.data[c * H * W + i];
            normal_std.data[c * H * W + i] = normal.data[c * H * W + i];
        }
    }

    // Normalize with STANDARD stats for UNets
    PreprocessingUtils::NormalizeImageNetStandard(diffuse_std);
    PreprocessingUtils::NormalizeImageNetStandard(normal_std);

    // ═══════════════════════════════════════════════════════
    //  STEP 2: Concatenate STANDARD-normalized tensors → 6ch input for UNets
    // ═══════════════════════════════════════════════════════
    Tensor input_6ch_std = Tensor::Create(1, 6, H, W);

    // Copy normalized diffuse → channels 0-2
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            input_6ch_std.data[c * H * W + i] = diffuse_std.data[c * H * W + i];
        }
    }

    // Copy normalized normal → channels 3-5
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            input_6ch_std.data[(c + 3) * H * W + i] = normal_std.data[c * H * W + i];
        }
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 3: FIRST PASS - Run UNet Albedo UNCOND (no features) → "dirty" albedo
    // ═══════════════════════════════════════════════════════
    Tensor dirty_albedo;
    {
        ONNXModelRunner unet_albedo_uncond;
        if (!unet_albedo_uncond.LoadModel(unet_albedo_uncond_path_.c_str(), config_.use_gpu, trt_cache_path_.c_str())) {
            Msg("! [PBRPipeline] Stage1: Failed to load UNet Albedo Uncond");
            return result;
        }

        xr_vector<Tensor> uncond_inputs = { input_6ch_std };
        xr_vector<const char*> uncond_input_names = { "image" };
        xr_vector<const char*> uncond_output_names = { "albedo" };

        xr_vector<Tensor> uncond_outputs = unet_albedo_uncond.Run(uncond_inputs, uncond_input_names, uncond_output_names);

        if (uncond_outputs.empty()) {
            Msg("! [PBRPipeline] Stage1: UNet Albedo Uncond failed");
            return result;
        }

        dirty_albedo = std::move(uncond_outputs[0]);
    } // UNet Albedo Uncond unloaded here

    if (config_.verbose) {
        Msg("[PBRPipeline] First pass (uncond): Dirty albedo [%u,%u,%u,%u]",
            dirty_albedo.batch(), dirty_albedo.channels(),
            dirty_albedo.height(), dirty_albedo.width());
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 4: Re-normalize dirty albedo to DEFAULT for SegFormer
    // ═══════════════════════════════════════════════════════

    // Normalize with DEFAULT stats for SegFormer
    PreprocessingUtils::NormalizeImageNetDefault(dirty_albedo);

    // ═══════════════════════════════════════════════════════
    //  STEP 5: Create 6ch input for SegFormer (DEFAULT-normalized albedo + STANDARD-normalized normal)
    // ═══════════════════════════════════════════════════════
    Tensor segformer_input_6ch = Tensor::Create(1, 6, H, W);

    // Copy DEFAULT-normalized dirty albedo → channels 0-2
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            segformer_input_6ch.data[c * H * W + i] = dirty_albedo.data[c * H * W + i];
        }
    }

    // Copy STANDARD-normalized normal → channels 3-5
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            segformer_input_6ch.data[(c + 3) * H * W + i] = normal_std.data[c * H * W + i];
        }
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 6: Run SegFormer → logits + features (load on-demand)
    // ═══════════════════════════════════════════════════════
    xr_vector<Tensor> seg_outputs;
    {
        ONNXModelRunner segformer;
        if (!segformer.LoadModel(segformer_path_.c_str(), config_.use_gpu, trt_cache_path_.c_str())) {
            Msg("! [PBRPipeline] Stage1: Failed to load SegFormer");
            return result;
        }

        xr_vector<Tensor> seg_inputs = { segformer_input_6ch };
        xr_vector<const char*> seg_input_names = { "input" };
        xr_vector<const char*> seg_output_names = { "logits", "features" };

        seg_outputs = segformer.Run(seg_inputs, seg_input_names, seg_output_names);
    } // SegFormer unloaded here

    if (seg_outputs.size() != 2) {
        Msg("! [PBRPipeline] Stage1: SegFormer returned %zu outputs, expected 2", seg_outputs.size());
        return result;
    }

    result.material_logits = std::move(seg_outputs[0]);
    result.seg_features = std::move(seg_outputs[1]);

    if (config_.verbose) {
        Msg("[PBRPipeline] SegFormer output: logits [%u,%u,%u,%u], features [%u,%u,%u,%u]",
            result.material_logits.batch(), result.material_logits.channels(),
            result.material_logits.height(), result.material_logits.width(),
            result.seg_features.batch(), result.seg_features.channels(),
            result.seg_features.height(), result.seg_features.width());
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 7: SECOND PASS - Run UNet Albedo with features → final albedo (load on-demand)
    // ═══════════════════════════════════════════════════════
    xr_vector<Tensor> albedo_outputs;
    {
        ONNXModelRunner unet_albedo;
        if (!unet_albedo.LoadModel(unet_albedo_path_.c_str(), config_.use_gpu, trt_cache_path_.c_str())) {
            Msg("! [PBRPipeline] Stage1: Failed to load UNet Albedo");
            return result;
        }

        // Use original STANDARD-normalized 6ch input + seg_features
        xr_vector<Tensor> albedo_inputs = { input_6ch_std, result.seg_features };
        xr_vector<const char*> albedo_input_names = { "image", "seg_features" };
        xr_vector<const char*> albedo_output_names = { "albedo" };

        albedo_outputs = unet_albedo.Run(albedo_inputs, albedo_input_names, albedo_output_names);
    } // UNet Albedo unloaded here

    if (albedo_outputs.empty()) {
        Msg("! [PBRPipeline] Stage1: UNet Albedo failed");
        return result;
    }

    result.albedo = std::move(albedo_outputs[0]);

    if (config_.verbose) {
        Msg("[PBRPipeline] Second pass (cond): Final albedo [%u,%u,%u,%u]",
            result.albedo.batch(), result.albedo.channels(),
            result.albedo.height(), result.albedo.width());
    }

    result.success = true;
    return result;
}

PBRPipeline::Stage2Outputs PBRPipeline::RunStage2(
    const Tensor& albedo,
    const Tensor& normal,
    const Tensor& material_logits,
    const Tensor& seg_features)
{
    Stage2Outputs result;

    const u32 H = normal.height();
    const u32 W = normal.width();

    if (config_.verbose) {
        Msg("[PBRPipeline] Stage2: Processing %ux%u texture", W, H);
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 1: Dual normalization (STANDARD + DEFAULT)
    // ═══════════════════════════════════════════════════════
    // CRITICAL: Python creates TWO albedo copies with different normalizations!
    // - albedo_std (STANDARD 0.5): for UNets (not used in 5ch, but kept for consistency)
    // - albedo_default (DEFAULT 0.485): for 12ch input (metallic/roughness)
    // - normal_std (STANDARD 0.5): for all inputs

    Tensor albedo_std = Tensor::Create(1, 3, H, W);
    Tensor albedo_default = Tensor::Create(1, 3, H, W);
    Tensor normal_std = Tensor::Create(1, 3, H, W);

    // Copy albedo (twice) and normal
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            albedo_std.data[c * H * W + i] = albedo.data[c * H * W + i];
            albedo_default.data[c * H * W + i] = albedo.data[c * H * W + i];
            normal_std.data[c * H * W + i] = normal.data[c * H * W + i];
        }
    }

    // Normalize albedo with BOTH stats
    PreprocessingUtils::NormalizeImageNetStandard(albedo_std);
    PreprocessingUtils::NormalizeImageNetDefault(albedo_default);

    // Normalize normal with STANDARD stats
    PreprocessingUtils::NormalizeImageNetStandard(normal_std);

    // ═══════════════════════════════════════════════════════
    //  STEP 2: Compute preprocessing from NORMALIZED normal
    // ═══════════════════════════════════════════════════════

    // Compute mean curvature from STANDARD-normalized normal map
    Tensor curvature = PreprocessingUtils::ComputeMeanCurvature(normal_std);

    // Compute Poisson coarse height from STANDARD-normalized normal map
    Tensor poisson = PreprocessingUtils::ComputePoissonCoarse(normal_std);

    // Convert material logits to one-hot mask (6 classes)
    constexpr u32 NUM_CLASSES = 6;  // fabric, ground, leather, metal, stone, wood
    Tensor material_mask = PreprocessingUtils::LogitsToMask(material_logits, NUM_CLASSES);

    // DEBUG: Check material mask values
    float mask_min = FLT_MAX, mask_max = -FLT_MAX;
    for (const float& val : material_mask.data) {
        mask_min = std::min(mask_min, val);
        mask_max = std::max(mask_max, val);
    }
    Msg("~ [PBRPipeline] Material mask before upsample: Range [%.4f, %.4f], Shape [%u,%u,%u,%u]",
        mask_min, mask_max,
        material_mask.batch(), material_mask.channels(), material_mask.height(), material_mask.width());

    // Upsample material mask to match texture resolution (from H/4 to H)
    material_mask = PreprocessingUtils::Upsample(material_mask, H, W);

    // DEBUG: Check material mask after upsample
    mask_min = FLT_MAX; mask_max = -FLT_MAX;
    for (const float& val : material_mask.data) {
        mask_min = std::min(mask_min, val);
        mask_max = std::max(mask_max, val);
    }
    Msg("~ [PBRPipeline] Material mask after upsample: Range [%.4f, %.4f]", mask_min, mask_max);

    if (config_.verbose) {
        Msg("[PBRPipeline] Preprocessing: curvature [%u,%u,%u,%u], poisson [%u,%u,%u,%u], mask [%u,%u,%u,%u]",
            curvature.batch(), curvature.channels(), curvature.height(), curvature.width(),
            poisson.batch(), poisson.channels(), poisson.height(), poisson.width(),
            material_mask.batch(), material_mask.channels(), material_mask.height(), material_mask.width());
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 3: Prepare inputs for 4 UNets
    // ═══════════════════════════════════════════════════════

    // Input for Parallax/AO: 5 channels (RGB STANDARD-normalized normal + curvature + poisson)
    Tensor input_5ch = Tensor::Create(1, 5, H, W);
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            input_5ch.data[c * H * W + i] = normal_std.data[c * H * W + i];
        }
    }
    for (u32 i = 0; i < H * W; ++i) {
        input_5ch.data[3 * H * W + i] = curvature.data[i];
        input_5ch.data[4 * H * W + i] = poisson.data[i];
    }

    // DEBUG: Check albedo and normal ranges
    float albedo_min = FLT_MAX, albedo_max = -FLT_MAX;
    for (const float& val : albedo.data) {
        albedo_min = std::min(albedo_min, val);
        albedo_max = std::max(albedo_max, val);
    }
    float normal_min = FLT_MAX, normal_max = -FLT_MAX;
    for (const float& val : normal.data) {
        normal_min = std::min(normal_min, val);
        normal_max = std::max(normal_max, val);
    }
    Msg("~ [PBRPipeline] Input ranges: albedo [%.4f, %.4f], normal [%.4f, %.4f]",
        albedo_min, albedo_max, normal_min, normal_max);

    // Input for Metallic/Roughness: 12 channels (DEFAULT albedo + STANDARD normal + 6-class mask)
    // CRITICAL: Python uses DEFAULT-normalized albedo here, not STANDARD!
    Msg("~ [PBRPipeline] Creating 12ch input from: albedo_default [%u,%u,%u,%u], normal_std [%u,%u,%u,%u], mask [%u,%u,%u,%u]",
        albedo_default.batch(), albedo_default.channels(), albedo_default.height(), albedo_default.width(),
        normal_std.batch(), normal_std.channels(), normal_std.height(), normal_std.width(),
        material_mask.batch(), material_mask.channels(), material_mask.height(), material_mask.width());

    Tensor input_12ch = Tensor::Create(1, 12, H, W);
    // Copy DEFAULT-normalized albedo (3ch) - matches Python's albedo_segformer
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            input_12ch.data[c * H * W + i] = albedo_default.data[c * H * W + i];
        }
    }
    // Copy STANDARD-normalized normal (3ch)
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            input_12ch.data[(c + 3) * H * W + i] = normal_std.data[c * H * W + i];
        }
    }
    // Copy material mask (6ch)
    for (u32 c = 0; c < NUM_CLASSES; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            input_12ch.data[(c + 6) * H * W + i] = material_mask.data[c * H * W + i];
        }
    }

    // ═══════════════════════════════════════════════════════
    //  STEP 3: Run UNet Parallax (5ch input) - load on-demand
    // ═══════════════════════════════════════════════════════
    xr_vector<Tensor> parallax_outputs;
    {
        ONNXModelRunner unet_parallax;
        if (!unet_parallax.LoadModel(unet_parallax_path_.c_str(), config_.use_gpu, trt_cache_path_.c_str())) {
            Msg("! [PBRPipeline] Stage2: Failed to load UNet Parallax");
            return result;
        }

        xr_vector<Tensor> parallax_inputs = { input_5ch, seg_features };
        xr_vector<const char*> parallax_input_names = { "image", "seg_features" };
        xr_vector<const char*> parallax_output_names = { "output" };

        parallax_outputs = unet_parallax.Run(parallax_inputs, parallax_input_names, parallax_output_names);
    } // UNet Parallax unloaded here

    if (parallax_outputs.empty()) {
        Msg("! [PBRPipeline] Stage2: UNet Parallax failed");
        return result;
    }
    result.parallax = std::move(parallax_outputs[0]);

    // ═══════════════════════════════════════════════════════
    //  STEP 4: Run UNet AO (5ch input) - load on-demand
    // ═══════════════════════════════════════════════════════
    xr_vector<Tensor> ao_outputs;
    {
        ONNXModelRunner unet_ao;
        if (!unet_ao.LoadModel(unet_ao_path_.c_str(), config_.use_gpu, trt_cache_path_.c_str())) {
            Msg("! [PBRPipeline] Stage2: Failed to load UNet AO");
            return result;
        }

        xr_vector<Tensor> ao_inputs = { input_5ch, seg_features };
        xr_vector<const char*> ao_input_names = { "image", "seg_features" };
        xr_vector<const char*> ao_output_names = { "output" };

        ao_outputs = unet_ao.Run(ao_inputs, ao_input_names, ao_output_names);
    } // UNet AO unloaded here

    if (ao_outputs.empty()) {
        Msg("! [PBRPipeline] Stage2: UNet AO failed");
        return result;
    }
    result.ao = std::move(ao_outputs[0]);

    // ═══════════════════════════════════════════════════════
    //  STEP 5: Run UNet Metallic (12ch input) - load on-demand
    // ═══════════════════════════════════════════════════════
    Msg("~ [PBRPipeline] Metallic inputs: image [%u,%u,%u,%u], seg_features [%u,%u,%u,%u]",
        input_12ch.batch(), input_12ch.channels(), input_12ch.height(), input_12ch.width(),
        seg_features.batch(), seg_features.channels(), seg_features.height(), seg_features.width());

    xr_vector<Tensor> metallic_outputs;
    {
        ONNXModelRunner unet_metallic;
        if (!unet_metallic.LoadModel(unet_metallic_path_.c_str(), config_.use_gpu, trt_cache_path_.c_str())) {
            Msg("! [PBRPipeline] Stage2: Failed to load UNet Metallic");
            return result;
        }

        xr_vector<Tensor> metallic_inputs = { input_12ch, seg_features };
        xr_vector<const char*> metallic_input_names = { "image", "seg_features" };
        xr_vector<const char*> metallic_output_names = { "output" };

        metallic_outputs = unet_metallic.Run(metallic_inputs, metallic_input_names, metallic_output_names);
    } // UNet Metallic unloaded here

    if (metallic_outputs.empty()) {
        Msg("! [PBRPipeline] Stage2: UNet Metallic failed");
        return result;
    }
    result.metallic = std::move(metallic_outputs[0]);

    // ═══════════════════════════════════════════════════════
    //  STEP 6: Run UNet Roughness (12ch input) - load on-demand
    // ═══════════════════════════════════════════════════════
    xr_vector<Tensor> roughness_outputs;
    {
        ONNXModelRunner unet_roughness;
        if (!unet_roughness.LoadModel(unet_roughness_path_.c_str(), config_.use_gpu, trt_cache_path_.c_str())) {
            Msg("! [PBRPipeline] Stage2: Failed to load UNet Roughness");
            return result;
        }

        xr_vector<Tensor> roughness_inputs = { input_12ch, seg_features };
        xr_vector<const char*> roughness_input_names = { "image", "seg_features" };
        xr_vector<const char*> roughness_output_names = { "output" };

        roughness_outputs = unet_roughness.Run(roughness_inputs, roughness_input_names, roughness_output_names);
    } // UNet Roughness unloaded here

    if (roughness_outputs.empty()) {
        Msg("! [PBRPipeline] Stage2: UNet Roughness failed");
        return result;
    }
    result.roughness = std::move(roughness_outputs[0]);

    if (config_.verbose) {
        Msg("[PBRPipeline] Stage2 complete: parallax [%u,%u,%u,%u], AO [%u,%u,%u,%u], metallic [%u,%u,%u,%u], roughness [%u,%u,%u,%u]",
            result.parallax.batch(), result.parallax.channels(), result.parallax.height(), result.parallax.width(),
            result.ao.batch(), result.ao.channels(), result.ao.height(), result.ao.width(),
            result.metallic.batch(), result.metallic.channels(), result.metallic.height(), result.metallic.width(),
            result.roughness.batch(), result.roughness.channels(), result.roughness.height(), result.roughness.width());
    }

    result.success = true;
    return result;
}

PBRPipelineOutputs PBRPipeline::Process(const u8* diffuse, const u8* normal, u32 width, u32 height) {
    PBRPipelineOutputs outputs;

    if (!initialized_) {
        Msg("! [PBRPipeline] Pipeline not initialized");
        return outputs;
    }

    // Convert input images to tensors
    // Note: DecompressDDS outputs RGBA8 (4 channels), so we read 4 then extract RGB
    Tensor diffuse_rgba = Tensor::FromImageData(diffuse, width, height, 4);
    Tensor normal_rgba = Tensor::FromImageData(normal, width, height, 4);

    // Extract RGB channels
    Tensor diffuse_tensor = Tensor::Create(1, 3, diffuse_rgba.height(), diffuse_rgba.width());
    Tensor normal_tensor = Tensor::Create(1, 3, normal_rgba.height(), normal_rgba.width());

    const u32 H = diffuse_rgba.height();
    const u32 W = diffuse_rgba.width();

    // Extract diffuse RGB (discard alpha - gloss channel)
    for (u32 c = 0; c < 3; ++c) {
        for (u32 i = 0; i < H * W; ++i) {
            diffuse_tensor.data[c * H * W + i] = diffuse_rgba.data[c * H * W + i];
        }
    }

    // Swizzle X-Ray normal format to AI model format
    // X-Ray format (RGBA): R=gloss, G=normalZ, B=normalY, A=normalX
    // AI expects (RGB):    R=normalX, G=normalY, B=normalZ
    for (u32 i = 0; i < H * W; ++i) {
        normal_tensor.data[0 * H * W + i] = normal_rgba.data[3 * H * W + i];  // R ← A (normal X)
        normal_tensor.data[1 * H * W + i] = normal_rgba.data[2 * H * W + i];  // G ← B (normal Y)
        normal_tensor.data[2 * H * W + i] = normal_rgba.data[1 * H * W + i];  // B ← G (normal Z)
    }

    // Stage 1: Albedo + Material Classification
    auto stage1 = RunStage1(diffuse_tensor, normal_tensor);
    if (!stage1.success) {
        return outputs;
    }

    // Stage 2: PBR Maps
    auto stage2 = RunStage2(stage1.albedo, normal_tensor, stage1.material_logits, stage1.seg_features);
    if (!stage2.success) {
        return outputs;
    }

    // Pack results
    outputs.albedo = std::move(stage1.albedo);
    outputs.metallic = std::move(stage2.metallic);
    outputs.roughness = std::move(stage2.roughness);
    outputs.ao = std::move(stage2.ao);
    outputs.parallax = std::move(stage2.parallax);
    outputs.normal = normal_tensor;  // Passthrough
    outputs.success = true;

    return outputs;
}

// ══════════════════════════════════════════════════════════
//  HELPER FUNCTIONS
// ══════════════════════════════════════════════════════════

bool AreAIModelsAvailable(const char* model_dir) {
    // Check if all required models exist
    const char* required_models[] = {
        "segformer.onnx",
        "unet_albedo.onnx",
        "unet_albedo_uncond.onnx",
        "unet_parallax.onnx",
        "unet_ao.onnx",
        "unet_metallic.onnx",
        "unet_roughness.onnx"
    };

    for (const auto& model : required_models) {
        // Build relative path
        xr_string relative_path = model_dir;
        relative_path.append("/");
        relative_path.append(model);

        // Resolve using VFS
        string_path full_path;
        FS.update_path(full_path, "$game_data$", relative_path.c_str());

        if (!FS.exist(full_path)) {
            return false;
        }
    }

    return true;
}
}

#else // !USE_AI_PBR

// Stub implementations when ONNX Runtime is not available
namespace xray::render::pbr {

bool AreAIModelsAvailable(const char* model_dir) {
    return false;  // AI models not available when ONNX Runtime is disabled
}

} // namespace xray::render::pbr

#endif // USE_AI_PBR
