#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/xr_resource.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"

namespace xray::render::resources {
    class FGResourceManager;
}

namespace xray::render::framegraph {
    class ShaderLoader;
}

namespace xray::render
{

/**
 * MaterialSystem - Central authority for material information in D3D12/FrameGraph renderer
 *
 * Replaces the legacy blender-based shader system with a modern approach:
 * 1. Material metadata files (.material) for explicit property definitions
 * 2. Shader reflection for automatic property detection (alpha test via discard)
 * 3. Sensible defaults when neither source is available
 *
 * Thread-safe for read operations after initialization.
 */
class MaterialSystem
{
public:
    /**
     * Material properties extracted from metadata or shader reflection
     */
    struct MaterialInfo
    {
        // Rendering flags
        bool alphaTest = false;      // Uses clip()/discard - needs alpha test in depth prepass
        bool transparent = false;    // Requires back-to-front sorting (bStrictB2F)
        u8 priority = 1;             // Render priority (0-3) for batching

        // Future PBR properties (stored here for unified material system)
        float metallic = 0.0f;
        float roughness = 0.5f;
        float aoScale = 1.0f;

        // Shader variant hints
        bool useParallax = false;
        bool useDetail = false;
    };

    /**
     * Texture set for a material - handles PBR texture conventions
     */
    struct TextureSet
    {
        ng::TextureHandle albedo;    // Base color / diffuse
        ng::TextureHandle normal;    // Normal map
        ng::TextureHandle pbr;       // Packed PBR (R=AO, G=Roughness, B=Metallic)
        ng::TextureHandle detail;    // Detail texture (optional)

        bool IsValid() const;
    };

    // Singleton access
    static MaterialSystem& Instance();

    // Initialization (call once during renderer init)
    void Initialize(resources::FGResourceManager* fgResourceManager, framegraph::ShaderLoader* shaderLoader);
    void Shutdown();

    /**
     * Get material info for a shader name
     * Checks in order: .material file, shader reflection, defaults
     * Results are cached after first lookup
     */
    const MaterialInfo& GetMaterialInfo(const char* shaderName);

    /**
     * Preload textures for a texture name during level load
     * Blocking call - textures will be fully loaded when this returns
     * Designed for future async streaming extension
     */
    void PreloadMaterialTextures(const char* textureName);

    /**
     * Get preloaded texture set for a texture name
     * Returns empty set if not preloaded
     */
    const TextureSet& GetTextures(const char* textureName) const;

    /**
     * Check if textures are loaded for a material
     */
    bool HasTextures(const char* textureName) const;

    /**
     * Clear all caches (call on level unload)
     */
    void ClearCaches();

    /**
     * Get statistics for debugging
     */
    struct Stats
    {
        u32 materialsCached = 0;
        u32 textureSetsCached = 0;
        u32 materialsFromFiles = 0;
        u32 materialsFromReflection = 0;
        u32 materialsFromDefaults = 0;
    };
    Stats GetStats() const;

private:
    MaterialSystem() = default;
    ~MaterialSystem() = default;
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator=(const MaterialSystem&) = delete;

    // Allow static instance creation
    friend MaterialSystem& GetMaterialSystemInstance();

    // Material info loading
    MaterialInfo LoadFromMetadataFile(const char* shaderName);
    MaterialInfo InferFromShaderReflection(const char* shaderName);
    MaterialInfo GetDefaultMaterialInfo() const;

    // Texture path resolution (handles PBR conventions)
    xr_string GetAlbedoPath(const char* textureName) const;
    xr_string GetNormalPath(const char* textureName) const;
    xr_string GetPBRPath(const char* textureName) const;
    xr_string GetDetailPath(const char* textureName) const;

    // Parse .material file
    bool ParseMaterialFile(const char* path, MaterialInfo& outInfo);

    // Caches (thread-safe reads after init)
    xr_map<shared_str, MaterialInfo> m_materialCache;
    xr_map<shared_str, TextureSet> m_textureCache;

    // Default empty texture set for missing textures
    TextureSet m_emptyTextureSet;

    // Default material info for unknown shaders
    MaterialInfo m_defaultMaterialInfo;

    // External dependencies (not owned)
    resources::FGResourceManager* m_fgResourceManager = nullptr;
    framegraph::ShaderLoader* m_shaderLoader = nullptr;

    // Statistics
    mutable Stats m_stats;

    // Initialization flag
    bool m_initialized = false;
};

} // namespace xray::render
