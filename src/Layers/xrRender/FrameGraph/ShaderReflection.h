#pragma once

#include "xrCore/xrstring.h"
#include "ResourceShape.h"
#include <slang.h>

namespace slang {
    struct ShaderReflection;
    struct IComponentType;
}

namespace xray::render::framegraph {

struct VulkanBindShifts {
    u32 shaderResource = 0;
    u32 sampler = 0;
    u32 constantBuffer = 0;
    u32 unorderedAccess = 0;

    bool HasShifts() const { return shaderResource || sampler || constantBuffer || unorderedAccess; }
};

struct VertexInputSignature {
    struct InputElement {
        shared_str semanticName;
        u32 semanticIndex;
        nvrhi::Format format;
        u32 inputSlot;

        InputElement() : semanticIndex(0), format(nvrhi::Format::UNKNOWN), inputSlot(0) {}
    };
    xr_vector<InputElement> elements;
};

struct ShaderRTBindings {

    struct InputTexture {
        shared_str name;
        u32 slot;
        ResourceShape shape = ResourceShape::Texture;

        InputTexture() : slot(0) {}
        InputTexture(const char* n, u32 s) : name(n), slot(s) {}
        InputTexture(const char* n, u32 s, ResourceShape sh) : name(n), slot(s), shape(sh) {}
    };
    xr_vector<InputTexture> inputTextures;

    struct UAVBinding {
        shared_str name;
        u32 slot;
        ResourceShape shape = ResourceShape::Unknown;

        UAVBinding() : slot(0) {}
        UAVBinding(const char* n, u32 s, ResourceShape sh) : name(n), slot(s), shape(sh) {}
    };
    xr_vector<UAVBinding> uavBindings;

    struct Sampler {
        shared_str name;
        u32 slot;

        enum class FilterMode : u8 {
            Point,
            Linear,
            Anisotropic
        };

        enum class AddressMode : u8 {
            Wrap,
            Clamp
        };

        struct SamplerState {
            AddressMode addressMode;
            FilterMode minFilter;
            FilterMode magFilter;
            FilterMode mipFilter;
            u8 maxAnisotropy;

            SamplerState()
                : addressMode(AddressMode::Wrap)
                , minFilter(FilterMode::Linear)
                , magFilter(FilterMode::Linear)
                , mipFilter(FilterMode::Linear)
                , maxAnisotropy(1) {}
        };

        Sampler() : slot(0) {}
        Sampler(const char* n, u32 s) : name(n), slot(s) {}

        SamplerState GetExpectedState() const {
            SamplerState state;

            if (strstr(name.c_str(), "smp_rtlinear") != nullptr) {
                state.addressMode = AddressMode::Clamp;
                state.minFilter = FilterMode::Linear;
                state.magFilter = FilterMode::Linear;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 1;
            }
            else if (strstr(name.c_str(), "smp_linear") != nullptr) {
                state.addressMode = AddressMode::Wrap;
                state.minFilter = FilterMode::Linear;
                state.magFilter = FilterMode::Linear;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 1;
            }
            else if (strstr(name.c_str(), "smp_base") != nullptr ||
                     strstr(name.c_str(), "smp_material") != nullptr ||
                     strstr(name.c_str(), "smp_bump") != nullptr) {
                state.addressMode = AddressMode::Wrap;
                state.minFilter = FilterMode::Anisotropic;
                state.magFilter = FilterMode::Anisotropic;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 8;
            }
            else if (strstr(name.c_str(), "smp_nofilter") != nullptr ||
                     strstr(name.c_str(), "smp_smap") != nullptr ||
                     strstr(name.c_str(), "smp_jitter") != nullptr) {
                state.addressMode = AddressMode::Clamp;
                state.minFilter = FilterMode::Point;
                state.magFilter = FilterMode::Point;
                state.mipFilter = FilterMode::Point;
                state.maxAnisotropy = 1;
            }
            else if (strstr(name.c_str(), "smp_shadowcmp") != nullptr) {
                state.addressMode = AddressMode::Clamp;
                state.minFilter = FilterMode::Linear;
                state.magFilter = FilterMode::Linear;
                state.mipFilter = FilterMode::Point;
                state.maxAnisotropy = 1;
            }
            else {
                state.addressMode = AddressMode::Wrap;
                state.minFilter = FilterMode::Linear;
                state.magFilter = FilterMode::Linear;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 1;
            }

            return state;
        }

        nvrhi::SamplerHandle CreateNVRHISampler(nvrhi::IDevice* device) const {
            auto samplerState = GetExpectedState();

            nvrhi::SamplerDesc nvrhiDesc;

            auto isLinearOrAniso = [](FilterMode mode) {
                return mode == FilterMode::Linear || mode == FilterMode::Anisotropic;
            };

            nvrhiDesc.setMinFilter(isLinearOrAniso(samplerState.minFilter));
            nvrhiDesc.setMagFilter(isLinearOrAniso(samplerState.magFilter));
            nvrhiDesc.setMipFilter(isLinearOrAniso(samplerState.mipFilter));

            nvrhi::SamplerAddressMode nvrhiAddressMode =
                (samplerState.addressMode == AddressMode::Clamp)
                    ? nvrhi::SamplerAddressMode::Clamp
                    : nvrhi::SamplerAddressMode::Wrap;
            nvrhiDesc.setAllAddressModes(nvrhiAddressMode);

            nvrhiDesc.setMaxAnisotropy(static_cast<float>(samplerState.maxAnisotropy));

            return device->createSampler(nvrhiDesc);
        }
    };
    xr_vector<Sampler> samplers;

    struct OutputRT {
        u32 slot;
        shared_str formatDesc;

        OutputRT() : slot(0) {}
        OutputRT(u32 s) : slot(s) {}
    };
    xr_vector<OutputRT> outputRTs;

    bool hasDepthOutput = false;

    shared_str shaderName;
};

struct ConstantBufferInfo {
    shared_str name;
    u32 slot;
    u32 size;

    ConstantBufferInfo() : slot(0), size(0) {}
    ConstantBufferInfo(const char* n, u32 s, u32 sz) : name(n), slot(s), size(sz) {}
};

enum class UpdateFrequency : u8 {
    Engine = 0,
    Pass = 1,
    Material = 2,
    Instance = 3
};

enum class ConstantPersistence : u8 {
    Volatile = 0,
    Static = 1,
    SemiStatic = 2
};

struct ShaderConstant {
    shared_str name;
    u32 offset;
    u32 size;
    UpdateFrequency frequency;
    ConstantPersistence persistence;
    u16 cbIndex;

    enum class Type : u8 {
        Scalar,
        Vector,
        Matrix3x4,
        Matrix4x4,
        Struct,
        Array
    };

    Type type = Type::Scalar;
    u16 arrayCount = 1;
    u16 elementSize = 0;
    u16 matrixStride = 0;

    bool IsArray() const { return arrayCount > 1; }
    bool IsMatrix() const { return type == Type::Matrix3x4 || type == Type::Matrix4x4; }
    bool IsMatrix3x4() const { return type == Type::Matrix3x4; }
    bool IsMatrix4x4() const { return type == Type::Matrix4x4; }

    ShaderConstant()
        : offset(0), size(0), frequency(UpdateFrequency::Instance)
        , persistence(ConstantPersistence::Volatile), cbIndex(0) {}
};

struct ShaderConstantBuffers {
    xr_vector<ConstantBufferInfo> buffers;

    const ConstantBufferInfo* GetBySlot(u32 slot) const {
        for (const auto& cb : buffers) {
            if (cb.slot == slot) return &cb;
        }
        return nullptr;
    }

    const ConstantBufferInfo* GetByName(const char* name) const {
        for (const auto& cb : buffers) {
            if (xr_strcmp(cb.name.c_str(), name) == 0) return &cb;
        }
        return nullptr;
    }
};

struct ShaderConstantLayout {
    ShaderConstantBuffers constantBuffers;
    xr_vector<ShaderConstant> constants;

    const ShaderConstant* FindConstant(const char* name) const {
        for (const auto& c : constants) {
            if (xr_strcmp(c.name.c_str(), name) == 0) return &c;
        }
        return nullptr;
    }

    xr_vector<const ShaderConstant*> GetConstantsByFrequency(UpdateFrequency freq) const {
        xr_vector<const ShaderConstant*> result;
        for (const auto& c : constants) {
            if (c.frequency == freq) result.push_back(&c);
        }
        return result;
    }
};

class ShaderReflector {
public:

    static ExtractedReflection ExtractReflection(
        slang::ShaderReflection* slangReflection,
        slang::IComponentType* linkedProgram,
        VulkanBindShifts vkShifts = {});

    static const VertexInputSignature& GetVertexInputSignature(
        const ExtractedReflection* reflection);

    static const ShaderRTBindings& GetRTBindings(
        const ExtractedReflection* reflection);

    static const ShaderConstantBuffers& GetConstantBuffers(
        const ExtractedReflection* reflection);




private:
    static VertexInputSignature AnalyzeVertexShader(slang::ShaderReflection* reflection);
    static ShaderRTBindings AnalyzeResourceBindings(slang::ShaderReflection* reflection, SlangStage stage);
    static ShaderConstantBuffers AnalyzeConstantBuffers(slang::ShaderReflection* reflection);
    static ShaderConstantLayout AnalyzeConstantLayout(slang::ShaderReflection* reflection);

    static UpdateFrequency InferConstantFrequency(const char* constantName, const char* cbName);

    static ConstantPersistence InferConstantPersistence(const char* cbName, UpdateFrequency frequency);

};

}
