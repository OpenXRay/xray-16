#include "xrEngine/stdafx.h"
#include "VulkanLensFlareRender.h"
#include "VulkanGameDevice.h"
#include "xrEngine/xr_efflensflare.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"

#include <algorithm>

namespace xray::render::vulkan
{
void VulkanFlareRender::Copy(IFlareRender& source)
{
    if (&source == this) return;
    const auto& other = static_cast<VulkanFlareRender&>(source);
    const std::string name = other.texture_name_;
    DestroyShader();
    CreateShader(nullptr, name.c_str());
}

void VulkanFlareRender::CreateShader(LPCSTR, LPCSTR texture)
{
    DestroyShader();
    if (!texture || !*texture) return;
    texture_name_ = texture;
    std::string error;
    if (!device_.textures().ui(texture_name_, device_.ui_pass(), texture_set_, error))
        Msg("! [renderer-vulkan] flare texture '%s': %s", texture, error.c_str());
}

void VulkanFlareRender::DestroyShader()
{
    if (texture_set_)
        device_.textures().release_ui(texture_set_, device_.ui_pass());
    texture_set_ = VK_NULL_HANDLE;
    texture_name_.clear();
}

void VulkanLensFlareRender::draw(const Fvector& center, const Fvector& axis_x,
    const Fvector& axis_y, float radius, float opacity, const Fcolor& tint,
    VkDescriptorSet texture)
{
    if (!texture || radius <= 0.f || opacity <= EPS_L) return;
    Fcolor color = tint;
    color.a *= std::clamp(opacity, 0.f, 1.f);
    const u32 packed = color.get();
    Fvector x, y;
    x.mul(axis_x, radius);
    y.mul(axis_y, radius);
    Fvector corners[4];
    corners[0].add(center, x).sub(y);
    corners[1].add(center, x).add(y);
    corners[2].sub(center, x).sub(y);
    corners[3].sub(center, x).add(y);
    auto& ui = device_.ui();
    ui.SetTextureDescriptor(texture);
    ui.CacheSetXformWorld(Fidentity);
    ui.CacheSetCullMode(IUIRender::cmNONE);
    ui.StartPrimitive(4, IUIRender::ptTriStrip, IUIRender::pttLIT);
    constexpr float uv[4][2]{{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    for (size_t i = 0; i < 4; ++i)
        ui.PushPoint(corners[i].x, corners[i].y, corners[i].z, packed, uv[i][0], uv[i][1]);
    ui.FlushPrimitive();
}

void VulkanLensFlareRender::Render(CLensFlare& owner, BOOL sun, BOOL flares, BOOL gradient)
{
    if (!owner.m_Current || !g_pGamePersistent) return;
    const float distance = g_pGamePersistent->Environment().CurrentEnv.far_plane * .75f;
    const auto descriptor = [](CLensFlareDescriptor::SFlare& flare)
    {
        auto* resource = dynamic_cast<VulkanFlareRender*>(&*flare.m_pRender);
        return resource ? resource->descriptor() : VK_NULL_HANDLE;
    };
    if (sun && owner.m_Current->m_Flags.is(CLensFlareDescriptor::flSource))
    {
        auto& source = owner.m_Current->m_Source;
        const Fcolor tint = source.ignore_color ? Fcolor().set(1.f, 1.f, 1.f, 1.f) : owner.LightColor;
        draw(owner.vecLight, owner.vecX, owner.vecY, source.fRadius * distance,
            owner.m_StateBlend, tint, descriptor(source));
    }
    if (owner.fBlend > EPS_L && flares &&
        owner.m_Current->m_Flags.is(CLensFlareDescriptor::flFlare))
    {
        Fvector direction, perpendicular;
        direction.normalize(owner.vecAxis);
        perpendicular.crossproduct(direction, owner.vecDir).normalize_safe();
        for (auto& flare : owner.m_Current->m_Flares)
        {
            Fvector position;
            position.mad(owner.vecCenter, owner.vecAxis, flare.fPosition);
            draw(position, direction, perpendicular, flare.fRadius * distance,
                flare.fOpacity * owner.fBlend * owner.m_StateBlend,
                owner.LightColor, descriptor(flare));
        }
    }
    if (owner.fBlend > EPS_L && gradient && owner.fGradientValue > EPS_L &&
        owner.m_Current->m_Flags.is(CLensFlareDescriptor::flGradient))
    {
        auto& flare = owner.m_Current->m_Gradient;
        draw(owner.vecLight, owner.vecX, owner.vecY,
            flare.fRadius * owner.fGradientValue * distance,
            owner.fGradientValue * owner.m_StateBlend, owner.LightColor, descriptor(flare));
    }
}
}
