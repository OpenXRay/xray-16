#include <vulkan/vulkan.h>
#pragma once
#include "Include/xrRender/DrawUtils.h"
#include "xrEngine/device.h"
#include "xrCore/_obb.h"

namespace xray::render::vulkan
{
class VulkanGameDevice;
class VulkanDrawUtils final : public CDUInterface, public pureRender
{
public:
    explicit VulkanDrawUtils(VulkanGameDevice& device);
    ~VulkanDrawUtils();
    void OnDeviceCreate();
    void OnRender() override;
    void DrawCross(const Fvector& p, float szx1, float szy1, float szz1, float szx2, float szy2,
        float szz2, u32 clr, BOOL bRot45) override;
    void DrawCross(const Fvector& p, float sz, u32 clr, BOOL bRot45) override;
    void DrawFlag(
        const Fvector& p, float heading, float height, float sz, float sz_fl, u32 clr, BOOL bDrawEntity) override;
    void DrawRomboid(const Fvector& p, float radius, u32 clr) override;
    void DrawJoint(const Fvector& p, float radius, u32 clr) override;

    void DrawSpotLight(const Fvector& p, const Fvector& d, float range, float phi, u32 clr) override;
    void DrawDirectionalLight(
        const Fvector& p, const Fvector& d, float radius, float range, u32 clr) override;
    void DrawPointLight(const Fvector& p, float radius, u32 clr) override;

    void DrawSound(const Fvector& p, float radius, u32 clr) override;
    void DrawLineSphere(const Fvector& p, float radius, u32 clr, BOOL bCross) override;

    void dbgDrawPlacement(
        const Fvector& p, int sz, u32 clr, LPCSTR caption, u32 clr_font) override;
    void dbgDrawVert(const Fvector& p0, u32 clr, LPCSTR caption) override;
    void dbgDrawEdge(const Fvector& p0, const Fvector& p1, u32 clr, LPCSTR caption) override;
    void dbgDrawFace(
        const Fvector& p0, const Fvector& p1, const Fvector& p2, u32 clr, LPCSTR caption) override;

    void DrawFace(
        const Fvector& p0, const Fvector& p1, const Fvector& p2, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) override;
    void DrawLine(const Fvector& p0, const Fvector& p1, u32 clr) override;
    void DrawLink(const Fvector& p0, const Fvector& p1, float sz, u32 clr) override;
    void DrawFaceNormal(
        const Fvector& p0, const Fvector& p1, const Fvector& p2, float size, u32 clr) override;
    void DrawFaceNormal(const Fvector* p, float size, u32 clr) override;
    void DrawFaceNormal(const Fvector& C, const Fvector& N, float size, u32 clr) override;
    void DrawSelectionBox(const Fvector& center, const Fvector& size, u32* c) override;
    void DrawSelectionBoxB(const Fbox& box, u32* c) override;
    void DrawIdentSphere(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) override;
    void DrawIdentSpherePart(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) override;
    void DrawIdentCone(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) override;
    void DrawIdentCylinder(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) override;
    void DrawIdentBox(BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) override;

    void DrawBox(
        const Fvector& offs, const Fvector& Size, BOOL bSolid, BOOL bWire, u32 clr_s, u32 clr_w) override;
    void DrawAABB(
        const Fvector& p0, const Fvector& p1, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) override;
    void DrawAABB(const Fmatrix& parent, const Fvector& center, const Fvector& size, u32 clr_s,
        u32 clr_w, BOOL bSolid, BOOL bWire) override;
    void DrawOBB(const Fmatrix& parent, const Fobb& box, u32 clr_s, u32 clr_w) override;
    void DrawSphere(
        const Fmatrix& parent, const Fvector& center, float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) override;
    void DrawSphere(
        const Fmatrix& parent, const Fsphere& S, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) override;
    void DrawCylinder(const Fmatrix& parent, const Fvector& center, const Fvector& dir, float height,
        float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) override;
    void DrawCone(const Fmatrix& parent, const Fvector& apex, const Fvector& dir, float height,
        float radius, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) override;
    void DrawPlane(const Fvector& center, const Fvector2& scale, const Fvector& rotate, u32 clr_s,
        u32 clr_w, BOOL bCull, BOOL bSolid, BOOL bWire) override;
    void DrawPlane(const Fvector& p, const Fvector& n, const Fvector2& scale, u32 clr_s, u32 clr_w,
        BOOL bCull, BOOL bSolid, BOOL bWire) override;
    void DrawRectangle(
        const Fvector& o, const Fvector& u, const Fvector& v, u32 clr_s, u32 clr_w, BOOL bSolid, BOOL bWire) override;

    void DrawGrid() override;
    void DrawPivot(const Fvector& pos, float sz = 5.f) override;
    void DrawAxis(const Fmatrix& T) override;
    void DrawObjectAxis(const Fmatrix& T, float sz, BOOL sel) override;
    void DrawSelectionRect(const Ivector2& m_SelStart, const Ivector2& m_SelEnd) override;

    void DrawIndexedPrimitive(int prim_type, u32 pc, const Fvector& pos, const Fvector* vb,
        const u32& vb_size, const u32* ib, const u32& ib_size, const u32& clr_argb, float scale) override;

    void OutText(
        const Fvector& pos, LPCSTR text, u32 color, u32 shadow_color) override;

    void OnDeviceDestroy() override;

private:
    void line(const Fvector& a, const Fvector& b, u32 color);
    void face(const Fvector& a, const Fvector& b, const Fvector& c, u32 color);
    void circle(const Fvector& center, float radius, int plane, u32 color);
    VulkanGameDevice& device_;
    VkDescriptorSet white_{};
    CGameFont* font_{};
};
}
