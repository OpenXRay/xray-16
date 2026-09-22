#include "stdafx.h"

#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/BindlessTypes.h"
#include "Layers/xrRender/Materials/MaterialSystem.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render
{
u8 MaterialCache::QuantizeDeveloperChannel(float value)
{
    return u8(clampr(iFloor(value * 255.0f), 0, 255));
}

void MaterialCache::BuildDeveloperTextureDesc(fg::RenderDevice::TextureDesc& desc, nvrhi::Format format, pcstr debugName)
{
    desc.width = DEVELOPER_TEXTURE_SIZE;
    desc.height = DEVELOPER_TEXTURE_SIZE;
    desc.mipLevels = 1;
    desc.format = format;
    desc.generateMips = false;
    desc.debugName = debugName;
}

void MaterialCache::FillDeveloperColorPixels(u8* pixels, const Fvector& color, float opacity)
{
    const u8 r = QuantizeDeveloperChannel(color.x);
    const u8 g = QuantizeDeveloperChannel(color.y);
    const u8 b = QuantizeDeveloperChannel(color.z);
    const u8 a = QuantizeDeveloperChannel(opacity);

    for (u32 texel = 0; texel < DEVELOPER_TEXTURE_SIZE * DEVELOPER_TEXTURE_SIZE; ++texel)
    {
        pixels[texel * 4 + 0] = r;
        pixels[texel * 4 + 1] = g;
        pixels[texel * 4 + 2] = b;
        pixels[texel * 4 + 3] = a;
    }
}

void MaterialCache::FillDeveloperPbrPixels(u8* pixels, float metallic, float roughness)
{
    const u8 m = QuantizeDeveloperChannel(metallic);
    const u8 r = QuantizeDeveloperChannel(roughness);

    for (u32 texel = 0; texel < DEVELOPER_TEXTURE_SIZE * DEVELOPER_TEXTURE_SIZE; ++texel)
    {
        pixels[texel * 4 + 0] = m;
        pixels[texel * 4 + 1] = r;
        pixels[texel * 4 + 2] = 255;
        pixels[texel * 4 + 3] = 0;
    }
}

u32 MaterialCache::RegisterDeveloperMaterial(const char* key, const char* shaderName, const char* textureName,
    const Fvector& color, float metallic, float roughness, float opacity)
{
    if (!key || !key[0] || !shaderName || !shaderName[0] || !textureName || !textureName[0])
    {
        Msg("! [dev_level] material registration rejected - incomplete identity (key='%s')", key ? key : "");
        return UINT32_MAX;
    }

    if (!m_device || !m_device->IsInitialized())
    {
        Msg("! [dev_level] material '%s' rejected - render device unavailable", key);
        return UINT32_MAX;
    }

    auto& materialBuffer = fg::bindless::MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
    {
        Msg("! [dev_level] material '%s' rejected - bindless material buffer unavailable", key);
        return UINT32_MAX;
    }

    const shared_str shaderNameStr(shaderName);
    const shared_str textureNameStr(textureName);
    const auto nameKey = std::make_pair(shaderNameStr, textureNameStr);

    u32 materialID = UINT32_MAX;

    const auto existing = m_developerMaterials.find(key);
    if (existing != m_developerMaterials.end())
    {
        DeveloperMaterial& record = existing->second;

        if (m_device->IsTextureValid(record.diffuse) && m_device->IsTextureValid(record.pbr))
        {
            m_materialIDByNames[nameKey] = record.materialID;
            return record.materialID;
        }

        const u32 previousMaterialID = record.materialID;
        ReleaseDeveloperMaterial(record, false);
        record = DeveloperMaterial{};
        if (previousMaterialID < materialBuffer.GetMaterialCount())
            materialID = previousMaterialID;
        record.materialID = materialID;

        Msg("! [dev_level] material '%s' textures are stale - rebuilding the procedural definition (matID=%u)",
            key, materialID);
    }

    char debugName[128];
    fg::RenderDevice::TextureDesc diffuseDesc;
    xr_sprintf(debugName, "dev_albedo_linear_%s", key);
    BuildDeveloperTextureDesc(diffuseDesc, nvrhi::Format::RGBA8_UNORM, debugName);

    u8 diffusePixels[DEVELOPER_TEXTURE_SIZE * DEVELOPER_TEXTURE_SIZE * 4];
    FillDeveloperColorPixels(diffusePixels, color, opacity);
    const fg::TextureHandle diffuse = m_device->CreateTexture(diffuseDesc, diffusePixels);

    fg::RenderDevice::TextureDesc pbrDesc;
    xr_sprintf(debugName, "dev_pbr_packed_%s", key);
    BuildDeveloperTextureDesc(pbrDesc, nvrhi::Format::RGBA8_UNORM, debugName);

    u8 pbrPixels[DEVELOPER_TEXTURE_SIZE * DEVELOPER_TEXTURE_SIZE * 4];
    FillDeveloperPbrPixels(pbrPixels, metallic, roughness);
    const fg::TextureHandle pbr = m_device->CreateTexture(pbrDesc, pbrPixels);

    if (!m_device->IsTextureValid(diffuse) || !m_device->IsTextureValid(pbr))
    {
        Msg("! [dev_level] material '%s' rejected - procedural texture creation failed", key);
        DeveloperMaterial partial;
        partial.diffuse = diffuse;
        partial.pbr = pbr;
        partial.shaderName = shaderNameStr;
        partial.textureName = textureNameStr;
        ReleaseDeveloperMaterial(partial, true);
        return UINT32_MAX;
    }

    nvrhi::ITexture* diffuseTexture = m_device->GetNativeTexture(diffuse);
    nvrhi::ITexture* pbrTexture = m_device->GetNativeTexture(pbr);

    const u32 diffuseIndex = RegisterMaterialTexture(diffuseTexture);
    const u32 pbrIndex = RegisterMaterialTexture(pbrTexture);
    if (diffuseIndex == fg::bindless::INVALID_TEXTURE_INDEX || pbrIndex == fg::bindless::INVALID_TEXTURE_INDEX)
    {
        Msg("! [dev_level] material '%s' rejected - bindless texture registration failed", key);
        DeveloperMaterial partial;
        partial.diffuse = diffuse;
        partial.pbr = pbr;
        partial.diffuseIndex = diffuseIndex;
        partial.pbrIndex = pbrIndex;
        partial.shaderName = shaderNameStr;
        partial.textureName = textureNameStr;
        ReleaseDeveloperMaterial(partial, true);
        return UINT32_MAX;
    }

    const auto& materialInfo = MaterialSystem::Instance().GetMaterialInfo(shaderName, textureName);
    fg::bindless::MaterialData materialData = {};
    materialData.diffuseIndex = diffuseIndex;
    materialData.normalIndex = fg::bindless::INVALID_TEXTURE_INDEX;
    materialData.detailIndex = fg::bindless::INVALID_TEXTURE_INDEX;
    materialData.pbrIndex = pbrIndex;
    materialData.detailScale = 1.0f;
    materialData.alphaRef = materialInfo.GetAlphaReference();
    materialData.flags = materialInfo.GetMaterialFlags() | fg::bindless::MAT_FLAG_HAS_PBR;
    materialData.shaderVariant = materialInfo.shaderVariant;

    if (materialID != UINT32_MAX)
        materialBuffer.UpdateMaterial(materialID, materialData);
    else
        materialID = materialBuffer.RegisterMaterial(materialData);

    if (materialID == UINT32_MAX)
    {
        Msg("! [dev_level] material '%s' rejected - material buffer is full", key);
        DeveloperMaterial partial;
        partial.diffuse = diffuse;
        partial.pbr = pbr;
        partial.diffuseIndex = diffuseIndex;
        partial.pbrIndex = pbrIndex;
        partial.shaderName = shaderNameStr;
        partial.textureName = textureNameStr;
        ReleaseDeveloperMaterial(partial, true);
        return UINT32_MAX;
    }

    DeveloperMaterial material = {};
    material.materialID = materialID;
    material.diffuse = diffuse;
    material.pbr = pbr;
    material.diffuseIndex = diffuseIndex;
    material.pbrIndex = pbrIndex;
    material.shaderName = shaderNameStr;
    material.textureName = textureNameStr;

    m_developerBindlessIndices.push_back(diffuseIndex);
    m_developerBindlessIndices.push_back(pbrIndex);
    m_materialIDByNames[nameKey] = materialID;

    if (existing != m_developerMaterials.end())
        existing->second = material;
    else
        m_developerMaterials.emplace(key, material);

    Msg("* [dev_level] event=material_registered key='%s' shader='%s' mat_id=%u diffuse=%u pbr=%u metallic=%.3f roughness=%.3f opacity=%.3f emissive=%.3f transparent=%u variant=%u flags=0x%X",
        key, shaderName, materialID, diffuseIndex, pbrIndex, metallic, roughness, opacity, materialInfo.emissive,
        u32(materialInfo.transparent), materialData.shaderVariant, materialData.flags);

    return materialID;
}

u32 MaterialCache::GetVisualMaterialEpoch() const
{
    return m_visualMaterialEpoch;
}

void MaterialCache::ReleaseDeveloperMaterial(const DeveloperMaterial& material, bool releaseBackend)
{
    u32 indices[2];
    u32 indexCount = 0;

    for (u32 index : { material.diffuseIndex, material.pbrIndex })
    {
        const auto tracked = std::find(m_developerBindlessIndices.begin(), m_developerBindlessIndices.end(), index);
        if (tracked != m_developerBindlessIndices.end())
            m_developerBindlessIndices.erase(tracked);

        const auto mapped = std::find(m_bindlessTextureIndices.begin(), m_bindlessTextureIndices.end(), index);
        if (mapped != m_bindlessTextureIndices.end())
            m_bindlessTextureIndices.erase(mapped);

        if (index != fg::bindless::INVALID_TEXTURE_INDEX)
        {
            for (auto texture = m_bindlessTextures.begin(); texture != m_bindlessTextures.end();)
            {
                if (texture->second == index)
                    texture = m_bindlessTextures.erase(texture);
                else
                    ++texture;
            }
        }

        if (index != fg::bindless::INVALID_TEXTURE_INDEX)
            indices[indexCount++] = index;
    }

    if (releaseBackend && m_textureBackend && indexCount)
        m_textureBackend->ReleaseBindlessTextures(indices, indexCount);

    if (m_device)
    {
        if (m_device->IsTextureValid(material.diffuse))
            m_device->DestroyTexture(material.diffuse);
        if (m_device->IsTextureValid(material.pbr))
            m_device->DestroyTexture(material.pbr);
    }

    m_materialIDByNames.erase(std::make_pair(material.shaderName, material.textureName));
}

void MaterialCache::ReleaseDeveloperMaterials()
{
    if (m_developerMaterials.empty() && m_developerBindlessIndices.empty())
        return;

    u32 released = 0;
    for (const auto& entry : m_developerMaterials)
    {
        if (entry.second.materialID != UINT32_MAX)
            ++released;
        ReleaseDeveloperMaterial(entry.second, true);
    }

    m_developerMaterials.clear();
    m_developerBindlessIndices.clear();

    Msg("* [dev_level] released %u procedural material(s)", released);
}
}
