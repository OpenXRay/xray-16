#include "xrEngine/stdafx.h"
#include "VulkanFontRender.h"
#include "VulkanGameDevice.h"

#include "xrEngine/GameFont.h"
#include "xrEngine/xr_level_controller.h"
#include "xrCore/Text/StringConversion.hpp"

#include <cmath>

extern ENGINE_API Fvector2 g_current_font_scale;

namespace xray::render::vulkan
{
VulkanFontRender::VulkanFontRender(VulkanGameDevice& device)
    : device_(device), shader_(device.textures(), device.ui_pass())
{}

void VulkanFontRender::Initialize(cpcstr shader, cpcstr texture)
{
    shader_.create(shader, texture);
    R_ASSERT2(shader_.inited(), "Vulkan font atlas could not be loaded");
}

void VulkanFontRender::glyph(CGameFont& owner, u16 codepoint, float& x,
    float y, float height, u32 top, u32 bottom)
{
    const Fvector& cell = owner.GetCharTC(codepoint);
    const float width = cell.z * g_current_font_scale.x;
    if (!fis_zero(cell.z))
    {
        const float u = cell.x / owner.vTS.x;
        const float v = cell.y / owner.vTS.y;
        const float u2 = u + cell.z / owner.vTS.x;
        const float v2 = v + owner.fTCHeight;
        auto& ui = device_.ui();
        ui.PushPoint(x, y + height, 0, bottom, u, v2);
        ui.PushPoint(x, y, 0, top, u, v);
        ui.PushPoint(x + width, y + height, 0, bottom, u2, v2);
        ui.PushPoint(x + width, y + height, 0, bottom, u2, v2);
        ui.PushPoint(x, y, 0, top, u, v);
        ui.PushPoint(x + width, y, 0, top, u2, v);
    }
    x += width * owner.vInterval.x;
}

void VulkanFontRender::OnRender(CGameFont& owner)
{
    if (!shader_.inited()) return;
    if (!(owner.uFlags & CGameFont::fsValid))
    {
        Fvector2 size;
        R_ASSERT2(shader_.GetBaseTextureResolution(size), "Vulkan font atlas has no size");
        owner.vTS.set(static_cast<int>(size.x), static_cast<int>(size.y));
        owner.fTCHeight = owner.fHeight / size.y;
        owner.uFlags |= CGameFont::fsValid;
    }
    auto& ui = device_.ui();
    ui.SetShader(shader_);
    ui.CacheSetCullMode(IUIRender::cmNONE);
    for (const auto& line : owner.strings)
    {
        xr_wide_char wide[MAX_MB_CHARS]{};
        const bool multibyte = owner.IsMultibyte();
        const u16 length = multibyte ?
            mbhMulti2Wide(wide, nullptr, MAX_MB_CHARS, line.string) :
            static_cast<u16>(xr_strlen(line.string));
        if (!length) continue;
        float x = std::floor(line.x), y = std::floor(line.y);
        if (line.align != CGameFont::alLeft)
        {
            const float width = multibyte ? owner.SizeOf_(wide) : owner.SizeOf_(line.string);
            x -= line.align == CGameFont::alCenter ?
                std::floor(width * 0.5f) * g_current_font_scale.x : std::floor(width);
        }
        const float height = line.height * g_current_font_scale.y;
        u32 bottom = line.c;
        if (owner.uFlags & CGameFont::fsGradient)
            bottom = color_rgba(color_get_R(line.c) / 2, color_get_G(line.c) / 2,
                color_get_B(line.c) / 2, color_get_A(line.c));
        // Action bindings can expand a single encoded character to a string.
        // Reserve for that expansion rather than overflowing StartPrimitive.
        const auto [actions, expanded] = owner.get_actions_text_length(line.string);
        const size_t max_glyphs = size_t(length) + expanded;
        R_ASSERT2(max_glyphs <= UINT32_MAX / 6, "Vulkan font line is too long");
        ui.StartPrimitive(static_cast<u32>(max_glyphs * 6), IUIRender::ptTriList, IUIRender::pttTL);
        for (u16 i = 0; i < length; ++i)
        {
            const u16 codepoint = multibyte ? wide[i + 1] :
                static_cast<u8>(line.string[i]);
            u16 spacing_codepoint = codepoint;
            if (codepoint == GAME_ACTION_MARK && i + 1 < length)
            {
                ++i;
                spacing_codepoint = multibyte ? wide[i + 1] :
                    static_cast<u8>(line.string[i]);
                const auto action = static_cast<EGameActions>(spacing_codepoint);
                pcstr binding = GetActionBinding(action);
                if (multibyte)
                {
                    xr_wide_char converted[MAX_MB_CHARS]{};
                    const u16 count = mbhMulti2Wide(converted, nullptr, MAX_MB_CHARS, binding);
                    for (u16 n = 0; n < count; ++n)
                        glyph(owner, converted[n + 1], x, y, height, line.c, bottom);
                }
                else
                    while (*binding)
                        glyph(owner, static_cast<u8>(*binding++), x, y, height, line.c, bottom);
            }
            else
                glyph(owner, codepoint, x, y, height, line.c, bottom);
            if (multibyte)
            {
                x -= 2;
                if (IsNeedSpaceCharacter(spacing_codepoint)) x += owner.fXStep;
            }
        }
        ui.FlushPrimitive();
    }
}
}
