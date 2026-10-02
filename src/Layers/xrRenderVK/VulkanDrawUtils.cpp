#include "xrEngine/stdafx.h"
#include "VulkanDrawUtils.h"
#include "VulkanGameDevice.h"
#include "xrEngine/GameFont.h"

#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
namespace
{
constexpr float Tau = 6.28318530718f;
Fvector point(float x, float y, float z) { Fvector p; p.set(x, y, z); return p; }
Fvector moved(const Fvector& origin, const Fvector& direction, float distance)
{ Fvector p; p.mad(origin, direction, distance); return p; }
}

VulkanDrawUtils::VulkanDrawUtils(VulkanGameDevice& device) : device_(device) {}
VulkanDrawUtils::~VulkanDrawUtils() { OnDeviceDestroy(); }

void VulkanDrawUtils::OnDeviceCreate()
{
    if (white_) return;
    const uint8_t pixel[4]{255, 255, 255, 255};
    std::string error;
    R_ASSERT2(device_.textures().ui_pixels(pixel, 1, 1, device_.ui_pass(), white_, error),
        error.c_str());
    font_ = xr_new<CGameFont>("stat_font");
    Device.seqRender.Add(this, REG_PRIORITY_LOW - 100);
}

void VulkanDrawUtils::OnDeviceDestroy()
{
    if (font_)
    {
        Device.seqRender.Remove(this);
        xr_delete(font_);
    }
    if (white_)
        device_.textures().release_ui(white_, device_.ui_pass());
    white_ = VK_NULL_HANDLE;
}

void VulkanDrawUtils::OnRender() { if (font_) font_->OnRender(); }

void VulkanDrawUtils::line(const Fvector& a, const Fvector& b, u32 color)
{
    if (!white_) return;
    auto& ui = device_.ui();
    ui.SetTextureDescriptor(white_);
    ui.CacheSetXformWorld(Fidentity);
    ui.CacheSetCullMode(IUIRender::cmNONE);
    ui.StartPrimitive(2, IUIRender::ptLineList, IUIRender::pttLIT);
    ui.PushPoint(a.x, a.y, a.z, color, 0.f, 0.f);
    ui.PushPoint(b.x, b.y, b.z, color, 1.f, 1.f);
    ui.FlushPrimitive();
}

void VulkanDrawUtils::face(const Fvector& a, const Fvector& b, const Fvector& c, u32 color)
{
    if (!white_) return;
    auto& ui = device_.ui();
    ui.SetTextureDescriptor(white_);
    ui.CacheSetXformWorld(Fidentity);
    ui.CacheSetCullMode(IUIRender::cmNONE);
    ui.StartPrimitive(3, IUIRender::ptTriList, IUIRender::pttLIT);
    ui.PushPoint(a.x, a.y, a.z, color, 0.f, 0.f);
    ui.PushPoint(b.x, b.y, b.z, color, 1.f, 0.f);
    ui.PushPoint(c.x, c.y, c.z, color, 0.f, 1.f);
    ui.FlushPrimitive();
}

void VulkanDrawUtils::circle(const Fvector& center, float radius, int plane, u32 color)
{
    if (!(radius > 0.f)) return;
    Fvector previous{};
    for (int i = 0; i <= 32; ++i)
    {
        const float angle = Tau * float(i) / 32.f;
        Fvector current = center;
        const float x = std::cos(angle) * radius, y = std::sin(angle) * radius;
        if (plane == 0) { current.y += x; current.z += y; }
        else if (plane == 1) { current.x += x; current.z += y; }
        else { current.x += x; current.y += y; }
        if (i) line(previous, current, color);
        previous = current;
    }
}

void VulkanDrawUtils::DrawLine(const Fvector& a, const Fvector& b, u32 color)
{ line(a, b, color); }

void VulkanDrawUtils::DrawCross(const Fvector& p, float x1, float y1, float z1,
    float x2, float y2, float z2, u32 color, BOOL rotated)
{
    const float s = rotated ? .70710678f : 1.f;
    line(point(p.x - x1 * s, p.y - y1 * (rotated ? s : 0.f), p.z),
        point(p.x + x2 * s, p.y + y2 * (rotated ? s : 0.f), p.z), color);
    line(point(p.x, p.y - y1 * s, p.z - z1 * (rotated ? s : 0.f)),
        point(p.x, p.y + y2 * s, p.z + z2 * (rotated ? s : 0.f)), color);
    line(point(p.x - x1 * (rotated ? s : 0.f), p.y, p.z - z1 * s),
        point(p.x + x2 * (rotated ? s : 0.f), p.y, p.z + z2 * s), color);
}

void VulkanDrawUtils::DrawCross(const Fvector& p, float size, u32 color, BOOL rotated)
{ DrawCross(p, size, size, size, size, size, size, color, rotated); }

void VulkanDrawUtils::DrawFlag(const Fvector& p, float heading, float height,
    float size, float flag_size, u32 color, BOOL entity)
{
    const Fvector top = point(p.x, p.y + height, p.z);
    line(p, top, color);
    Fvector tip = point(top.x + std::sin(heading) * size,
        top.y - flag_size, top.z + std::cos(heading) * size);
    face(top, point(top.x, top.y - flag_size, top.z), tip, color);
    if (entity) DrawCross(p, size * .25f, color, FALSE);
}

void VulkanDrawUtils::DrawRomboid(const Fvector& p, float r, u32 color)
{
    const Fvector top = point(p.x, p.y + r, p.z), bottom = point(p.x, p.y - r, p.z);
    const Fvector ring[]{point(p.x + r, p.y, p.z), point(p.x, p.y, p.z + r),
        point(p.x - r, p.y, p.z), point(p.x, p.y, p.z - r)};
    for (int i = 0; i < 4; ++i)
    {
        face(top, ring[i], ring[(i + 1) % 4], color);
        face(bottom, ring[(i + 1) % 4], ring[i], color);
    }
}
void VulkanDrawUtils::DrawJoint(const Fvector& p, float r, u32 color)
{ DrawLineSphere(p, r, color, FALSE); }
void VulkanDrawUtils::DrawSpotLight(const Fvector& p, const Fvector& d,
    float range, float phi, u32 color)
{
    Fvector tip = moved(p, d, range);
    DrawLine(p, tip, color);
    DrawLineSphere(tip, range * std::tan(phi * .5f), color, FALSE);
}
void VulkanDrawUtils::DrawDirectionalLight(const Fvector& p, const Fvector& d,
    float radius, float range, u32 color)
{
    DrawLineSphere(p, radius, color, FALSE);
    DrawLine(p, moved(p, d, range), color);
}
void VulkanDrawUtils::DrawPointLight(const Fvector& p, float radius, u32 color)
{ DrawLineSphere(p, radius, color, TRUE); }
void VulkanDrawUtils::DrawSound(const Fvector& p, float radius, u32 color)
{ DrawLineSphere(p, radius, color, FALSE); }
void VulkanDrawUtils::DrawLineSphere(const Fvector& p, float radius, u32 color, BOOL cross)
{
    for (int plane = 0; plane < 3; ++plane) circle(p, radius, plane, color);
    if (cross) DrawCross(p, radius, color, FALSE);
}

void VulkanDrawUtils::dbgDrawPlacement(const Fvector& p, int size, u32 color,
    LPCSTR caption, u32 text_color)
{
    DrawCross(p, float(size) * .01f, color, TRUE);
    if (caption) OutText(p, caption, text_color, 0xff000000);
}
void VulkanDrawUtils::dbgDrawVert(const Fvector& p, u32 color, LPCSTR caption)
{ dbgDrawPlacement(p, 10, color, caption, color); }
void VulkanDrawUtils::dbgDrawEdge(const Fvector& a, const Fvector& b, u32 color, LPCSTR caption)
{
    line(a, b, color);
    if (caption) { Fvector center; center.add(a, b).mul(.5f); OutText(center, caption, color, 0xff000000); }
}
void VulkanDrawUtils::dbgDrawFace(const Fvector& a, const Fvector& b,
    const Fvector& c, u32 color, LPCSTR caption)
{
    DrawFace(a, b, c, color, color, TRUE, TRUE);
    if (caption) { Fvector center; center.add(a, b).add(c).div(3.f); OutText(center, caption, color, 0xff000000); }
}
void VulkanDrawUtils::DrawFace(const Fvector& a, const Fvector& b, const Fvector& c,
    u32 solid, u32 wire, BOOL draw_solid, BOOL draw_wire)
{
    if (draw_solid) face(a, b, c, solid);
    if (draw_wire) { line(a, b, wire); line(b, c, wire); line(c, a, wire); }
}
void VulkanDrawUtils::DrawLink(const Fvector& a, const Fvector& b, float size, u32 color)
{ line(a, b, color); DrawCross(b, size, color, FALSE); }
void VulkanDrawUtils::DrawFaceNormal(const Fvector& a, const Fvector& b,
    const Fvector& c, float size, u32 color)
{
    Fvector center, normal;
    center.add(a, b).add(c).div(3.f);
    normal.mknormal(a, b, c);
    DrawFaceNormal(center, normal, size, color);
}
void VulkanDrawUtils::DrawFaceNormal(const Fvector* p, float size, u32 color)
{ if (p) DrawFaceNormal(p[0], p[1], p[2], size, color); }
void VulkanDrawUtils::DrawFaceNormal(const Fvector& center, const Fvector& normal,
    float size, u32 color)
{ line(center, moved(center, normal, size), color); }

void VulkanDrawUtils::DrawSelectionBox(const Fvector& center, const Fvector& size, u32* colors)
{
    Fvector low, high;
    low.sub(center, size);
    high.add(center, size);
    DrawAABB(low, high, colors ? colors[0] : 0x40ffffff,
        colors ? colors[1] : 0xffffffff, TRUE, TRUE);
}
void VulkanDrawUtils::DrawSelectionBoxB(const Fbox& box, u32* colors)
{ DrawAABB(box.vMin, box.vMax, colors ? colors[0] : 0x40ffffff,
    colors ? colors[1] : 0xffffffff, TRUE, TRUE); }
void VulkanDrawUtils::DrawIdentSphere(BOOL solid, BOOL wire, u32 solid_color, u32 wire_color)
{ DrawSphere(Fidentity, point(0, 0, 0), 1.f, solid_color, wire_color, solid, wire); }
void VulkanDrawUtils::DrawIdentSpherePart(BOOL solid, BOOL wire, u32 solid_color, u32 wire_color)
{
    DrawIdentSphere(solid, wire, solid_color, wire_color);
}
void VulkanDrawUtils::DrawIdentCone(BOOL solid, BOOL wire, u32 solid_color, u32 wire_color)
{ DrawCone(Fidentity, point(0, 1, 0), point(0, -1, 0), 2.f, 1.f,
    solid_color, wire_color, solid, wire); }
void VulkanDrawUtils::DrawIdentCylinder(BOOL solid, BOOL wire, u32 solid_color, u32 wire_color)
{ DrawCylinder(Fidentity, point(0, 0, 0), point(0, 1, 0), 2.f, 1.f,
    solid_color, wire_color, solid, wire); }
void VulkanDrawUtils::DrawIdentBox(BOOL solid, BOOL wire, u32 solid_color, u32 wire_color)
{ DrawBox(point(0, 0, 0), point(1, 1, 1), solid, wire, solid_color, wire_color); }

void VulkanDrawUtils::DrawBox(const Fvector& offset, const Fvector& size,
    BOOL solid, BOOL wire, u32 solid_color, u32 wire_color)
{
    Fvector low, high;
    low.sub(offset, size);
    high.add(offset, size);
    DrawAABB(low, high, solid_color, wire_color, solid, wire);
}
void VulkanDrawUtils::DrawAABB(const Fvector& low, const Fvector& high,
    u32 solid_color, u32 wire_color, BOOL solid, BOOL wire)
{
    const Fvector p[8]{point(low.x, low.y, low.z), point(high.x, low.y, low.z),
        point(high.x, high.y, low.z), point(low.x, high.y, low.z),
        point(low.x, low.y, high.z), point(high.x, low.y, high.z),
        point(high.x, high.y, high.z), point(low.x, high.y, high.z)};
    constexpr int quads[6][4]{{0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}};
    for (const auto& q : quads)
    {
        if (solid) { face(p[q[0]], p[q[1]], p[q[2]], solid_color);
            face(p[q[0]], p[q[2]], p[q[3]], solid_color); }
        if (wire) for (int i = 0; i < 4; ++i)
            line(p[q[i]], p[q[(i + 1) % 4]], wire_color);
    }
}
void VulkanDrawUtils::DrawAABB(const Fmatrix& parent, const Fvector& center,
    const Fvector& size, u32 solid_color, u32 wire_color, BOOL solid, BOOL wire)
{
    Fvector local[8];
    for (int i = 0; i < 8; ++i)
    {
        Fvector p = point(center.x + (i & 1 ? size.x : -size.x),
            center.y + (i & 2 ? size.y : -size.y), center.z + (i & 4 ? size.z : -size.z));
        parent.transform_tiny(local[i], p);
    }
    constexpr int edges[12][2]{{0,1},{2,3},{4,5},{6,7},{0,2},{1,3},
        {4,6},{5,7},{0,4},{1,5},{2,6},{3,7}};
    if (wire) for (auto& edge : edges) line(local[edge[0]], local[edge[1]], wire_color);
    if (solid)
    {
        constexpr int faces[6][4]{{0,1,3,2},{4,5,7,6},{0,1,5,4},
            {2,3,7,6},{0,2,6,4},{1,3,7,5}};
        for (auto& q : faces)
        { face(local[q[0]], local[q[1]], local[q[2]], solid_color);
          face(local[q[0]], local[q[2]], local[q[3]], solid_color); }
    }
}
void VulkanDrawUtils::DrawOBB(const Fmatrix& parent, const Fobb& box,
    u32 solid_color, u32 wire_color)
{
    Fmatrix local, world;
    box.xform_get(local);
    world.mul_43(parent, local);
    DrawAABB(world, point(0, 0, 0), box.m_halfsize,
        solid_color, wire_color, TRUE, TRUE);
}
void VulkanDrawUtils::DrawSphere(const Fmatrix& parent, const Fvector& center,
    float radius, u32 solid_color, u32 wire_color, BOOL solid, BOOL wire)
{
    if (!(radius > 0.f)) return;
    constexpr int slices = 16, stacks = 8;
    for (int stack = 0; stack < stacks; ++stack)
    for (int slice = 0; slice < slices; ++slice)
    {
        const auto vertex = [&](int s, int t)
        {
            const float longitude = Tau * float(s) / slices;
            const float latitude = 3.14159265f * float(t) / stacks;
            const Fvector local = point(center.x + radius * std::sin(latitude) * std::cos(longitude),
                center.y + radius * std::cos(latitude),
                center.z + radius * std::sin(latitude) * std::sin(longitude));
            Fvector result; parent.transform_tiny(result, local); return result;
        };
        const Fvector a = vertex(slice, stack), b = vertex(slice + 1, stack);
        const Fvector c = vertex(slice, stack + 1), d = vertex(slice + 1, stack + 1);
        if (solid) { face(a, b, c, solid_color); face(b, d, c, solid_color); }
        if (wire) { line(a, b, wire_color); line(a, c, wire_color); }
    }
}
void VulkanDrawUtils::DrawSphere(const Fmatrix& parent, const Fsphere& sphere,
    u32 solid_color, u32 wire_color, BOOL solid, BOOL wire)
{ DrawSphere(parent, sphere.P, sphere.R, solid_color, wire_color, solid, wire); }

void VulkanDrawUtils::DrawCylinder(const Fmatrix& parent, const Fvector& center,
    const Fvector& direction, float height, float radius,
    u32 solid_color, u32 wire_color, BOOL solid, BOOL wire)
{
    if (!(height > 0.f) || !(radius > 0.f)) return;
    Fvector axis = direction, basis, side;
    axis.normalize_safe();
    basis.crossproduct(axis, point(0, std::abs(axis.y) < .9f ? 1.f : 0.f,
        std::abs(axis.y) < .9f ? 0.f : 1.f)).normalize_safe();
    side.crossproduct(axis, basis).normalize_safe();
    const Fvector top = moved(center, axis, height * .5f);
    const Fvector bottom = moved(center, axis, -height * .5f);
    for (int i = 0; i < 24; ++i)
    {
        const auto ring = [&](const Fvector& origin, int segment)
        {
            const float a = Tau * segment / 24.f;
            Fvector local = moved(moved(origin, basis, radius * std::cos(a)),
                side, radius * std::sin(a));
            Fvector world; parent.transform_tiny(world, local); return world;
        };
        const Fvector a = ring(bottom, i), b = ring(bottom, i + 1);
        const Fvector c = ring(top, i), d = ring(top, i + 1);
        if (solid) { face(a, b, c, solid_color); face(b, d, c, solid_color); }
        if (wire) { line(a, b, wire_color); line(c, d, wire_color); line(a, c, wire_color); }
    }
}
void VulkanDrawUtils::DrawCone(const Fmatrix& parent, const Fvector& apex,
    const Fvector& direction, float height, float radius,
    u32 solid_color, u32 wire_color, BOOL solid, BOOL wire)
{
    if (!(height > 0.f) || !(radius > 0.f)) return;
    Fvector axis = direction, basis, side;
    axis.normalize_safe();
    basis.crossproduct(axis, point(0, std::abs(axis.y) < .9f ? 1.f : 0.f,
        std::abs(axis.y) < .9f ? 0.f : 1.f)).normalize_safe();
    side.crossproduct(axis, basis).normalize_safe();
    Fvector tip; parent.transform_tiny(tip, apex);
    const Fvector center = moved(apex, axis, height);
    for (int i = 0; i < 24; ++i)
    {
        const auto ring = [&](int segment)
        {
            const float a = Tau * segment / 24.f;
            Fvector local = moved(moved(center, basis, radius * std::cos(a)),
                side, radius * std::sin(a));
            Fvector world; parent.transform_tiny(world, local); return world;
        };
        const Fvector a = ring(i), b = ring(i + 1);
        if (solid) face(tip, a, b, solid_color);
        if (wire) { line(a, b, wire_color); line(tip, a, wire_color); }
    }
}

void VulkanDrawUtils::DrawPlane(const Fvector& center, const Fvector2& scale,
    const Fvector& rotation, u32 solid_color, u32 wire_color,
    BOOL cull, BOOL solid, BOOL wire)
{
    Fmatrix transform;
    transform.setXYZ(rotation);
    Fvector normal;
    normal.set(0.f, 1.f, 0.f);
    transform.transform_dir(normal);
    DrawPlane(center, normal, scale, solid_color, wire_color, cull, solid, wire);
}
void VulkanDrawUtils::DrawPlane(const Fvector& center, const Fvector& normal,
    const Fvector2& scale, u32 solid_color, u32 wire_color,
    BOOL, BOOL solid, BOOL wire)
{
    Fvector up = normal, right, forward;
    up.normalize_safe();
    right.crossproduct(up, point(0, std::abs(up.y) < .9f ? 1.f : 0.f,
        std::abs(up.y) < .9f ? 0.f : 1.f)).normalize_safe();
    forward.crossproduct(up, right).normalize_safe();
    const Fvector origin = moved(moved(center, right, -scale.x), forward, -scale.y);
    Fvector u, v; u.mul(right, 2.f * scale.x); v.mul(forward, 2.f * scale.y);
    DrawRectangle(origin, u, v, solid_color, wire_color, solid, wire);
}
void VulkanDrawUtils::DrawRectangle(const Fvector& origin, const Fvector& u,
    const Fvector& v, u32 solid_color, u32 wire_color, BOOL solid, BOOL wire)
{
    const Fvector a = origin, b = moved(origin, u, 1.f),
        c = moved(b, v, 1.f), d = moved(origin, v, 1.f);
    if (solid) { face(a, b, c, solid_color); face(a, c, d, solid_color); }
    if (wire) { line(a, b, wire_color); line(b, c, wire_color);
        line(c, d, wire_color); line(d, a, wire_color); }
}
void VulkanDrawUtils::DrawGrid()
{
    for (int i = -10; i <= 10; ++i)
    {
        const u32 color = i ? 0x60404040 : 0xff909090;
        line(point(float(i), 0, -10), point(float(i), 0, 10), color);
        line(point(-10, 0, float(i)), point(10, 0, float(i)), color);
    }
}
void VulkanDrawUtils::DrawPivot(const Fvector& pos, float size)
{ DrawCross(pos, size, 0xffffffff, FALSE); }
void VulkanDrawUtils::DrawAxis(const Fmatrix& transform)
{ DrawObjectAxis(transform, 1.f, FALSE); }
void VulkanDrawUtils::DrawObjectAxis(const Fmatrix& transform, float size, BOOL selected)
{
    const Fvector center = transform.c;
    line(center, moved(center, transform.i, size), selected ? 0xffffff80 : 0xffff0000);
    line(center, moved(center, transform.j, size), selected ? 0xffffff80 : 0xff00ff00);
    line(center, moved(center, transform.k, size), selected ? 0xffffff80 : 0xff0000ff);
}
void VulkanDrawUtils::DrawSelectionRect(const Ivector2& start, const Ivector2& end)
{
    if (!white_) return;
    auto& ui = device_.ui();
    const Fvector screen[4]{point(float(start.x), float(start.y), 0),
        point(float(end.x), float(start.y), 0), point(float(end.x), float(end.y), 0),
        point(float(start.x), float(end.y), 0)};
    ui.SetTextureDescriptor(white_);
    ui.CacheSetCullMode(IUIRender::cmNONE);
    ui.StartPrimitive(5, IUIRender::ptLineStrip, IUIRender::pttTL);
    for (int i = 0; i < 5; ++i)
    {
        const auto& p = screen[i % 4];
        ui.PushPoint(p.x, p.y, 0.f, 0xffffffff, 0.f, 0.f);
    }
    ui.FlushPrimitive();
}
void VulkanDrawUtils::DrawIndexedPrimitive(int primitive, u32 count,
    const Fvector& position, const Fvector* vertices, const u32& vertex_count,
    const u32* indices, const u32& index_count, const u32& color, float scale)
{
    if (!vertices || !indices) return;
    const u32 requested = primitive == 2 ? count * 2 : count * 3;
    const u32 end = std::min(index_count, requested);
    if (primitive == 2)
    {
        for (u32 i = 0; i + 1 < end; i += 2)
            if (indices[i] < vertex_count && indices[i + 1] < vertex_count)
                line(moved(position, vertices[indices[i]], scale),
                    moved(position, vertices[indices[i + 1]], scale), color);
    }
    else if (primitive == 4)
    {
        for (u32 i = 0; i + 2 < end; i += 3)
            if (indices[i] < vertex_count && indices[i + 1] < vertex_count &&
                indices[i + 2] < vertex_count)
                face(moved(position, vertices[indices[i]], scale),
                    moved(position, vertices[indices[i + 1]], scale),
                    moved(position, vertices[indices[i + 2]], scale), color);
    }
}
void VulkanDrawUtils::OutText(const Fvector& position, LPCSTR text,
    u32 color, u32 shadow_color)
{
    if (!font_ || !text || !*text) return;
    const float w = position.x * Device.mFullTransform._14 +
        position.y * Device.mFullTransform._24 +
        position.z * Device.mFullTransform._34 + Device.mFullTransform._44;
    if (w <= 0.f) return;
    Fvector projected;
    Device.mFullTransform.transform(projected, position);
    const float x = (projected.x + 1.f) * .5f * Device.dwWidth;
    const float y = (1.f - projected.y) * .5f * Device.dwHeight;
    font_->SetColor(shadow_color);
    font_->Out(x + 1.f, y + 1.f, "%s", text);
    font_->SetColor(color);
    font_->Out(x, y, "%s", text);
}
}
