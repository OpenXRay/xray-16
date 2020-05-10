#include "pch.hpp"
#include "ui_base.h"
#include "Cursor/UICursor.h"
#include "xrCore/XML/XMLDocument.hpp"
#include "XML/UIXmlInitBase.h"
#include "XML/UITextureMaster.h"

CUICursor& GetUICursor() { return GEnv.UI->GetUICursor(); }
UICore& UI() { return *GEnv.UI; }
extern ENGINE_API Fvector2 g_current_font_scale;

void S2DVert::rotate_pt(const Fvector2& pivot, const float cosA, const float sinA, const float kx)
{
    Fvector2 t = pt;
    t.sub(pivot);
    pt.x = t.x * cosA + t.y * sinA;
    pt.y = t.y * cosA - t.x * sinA;
    pt.x *= kx;
    pt.add(pivot);
}
void C2DFrustum::CreateFromRect(const Frect& rect)
{
    m_rect.set(float(rect.x1), float(rect.y1), float(rect.x2), float(rect.y2));
    planes.resize(4);
    planes[0].build(rect.lt, Fvector2().set(-1, 0));
    planes[1].build(rect.lt, Fvector2().set(0, -1));
    planes[2].build(rect.rb, Fvector2().set(+1, 0));
    planes[3].build(rect.rb, Fvector2().set(0, +1));
}

sPoly2D* C2DFrustum::ClipPoly(sPoly2D& S, sPoly2D& D) const
{
    bool bFullTest = false;
    for (u32 j = 0; j < S.size(); j++)
    {
        if (!m_rect.in(S[j].pt))
        {
            bFullTest = true;
            break;
        }
    }

    sPoly2D* src = &D;
    sPoly2D* dest = &S;
    if (!bFullTest)
        return dest;

    for (u32 i = 0; i < planes.size(); i++)
    {
        // cache plane and swap lists
        const Fplane2& P = planes[i];
        std::swap(src, dest);
        dest->clear();

        // classify all points relative to plane #i
        float cls[UI_FRUSTUM_SAFE];
        for (u32 j = 0; j < src->size(); j++)
            cls[j] = P.classify((*src)[j].pt);

        // clip everything to this plane
        cls[src->size()] = cls[0];
        src->push_back((*src)[0]);
        Fvector2 dir_pt, dir_uv;
        float denum, t;
        for (u32 j = 0; j < src->size() - 1; j++)
        {
            if ((*src)[j].pt.similar((*src)[j + 1].pt, EPS_S))
                continue;
            if (negative(cls[j]))
            {
                dest->push_back((*src)[j]);
                if (positive(cls[j + 1]))
                {
                    // segment intersects plane
                    dir_pt.sub((*src)[j + 1].pt, (*src)[j].pt);
                    dir_uv.sub((*src)[j + 1].uv, (*src)[j].uv);
                    denum = P.n.dotproduct(dir_pt);
                    if (denum != 0)
                    {
                        t = -cls[j] / denum; // VERIFY(t<=1.f && t>=0);
                        dest->last().pt.mad((*src)[j].pt, dir_pt, t);
                        dest->last().uv.mad((*src)[j].uv, dir_uv, t);
                        dest->inc();
                    }
                }
            }
            else
            {
                // J - outside
                if (negative(cls[j + 1]))
                {
                    // J+1  - inside
                    // segment intersects plane
                    dir_pt.sub((*src)[j + 1].pt, (*src)[j].pt);
                    dir_uv.sub((*src)[j + 1].uv, (*src)[j].uv);
                    denum = P.n.dotproduct(dir_pt);
                    if (denum != 0)
                    {
                        t = -cls[j] / denum; // VERIFY(t<=1.f && t>=0);
                        dest->last().pt.mad((*src)[j].pt, dir_pt, t);
                        dest->last().uv.mad((*src)[j].uv, dir_uv, t);
                        dest->inc();
                    }
                }
            }
        }

        // here we end up with complete polygon in 'dest' which is inside plane #i
        if (dest->size() < 3)
            return 0;
    }
    return dest;
}

void UICore::ClientToScreenScaled(Fvector2& dest, float left, float top) const
{
    if (m_currentPointType != IUIRender::pttLIT)
        dest.set(ClientToScreenScaledX(left), ClientToScreenScaledY(top));
    else
        dest.set(left, top);
}

void UICore::ClientToScreenScaled(Fvector2& src_and_dest) const
{
    if (m_currentPointType != IUIRender::pttLIT)
        src_and_dest.set(ClientToScreenScaledX(src_and_dest.x), ClientToScreenScaledY(src_and_dest.y));
}

void UICore::ClientToScreenScaledWidth(float& src_and_dest) const
{
    if (m_currentPointType != IUIRender::pttLIT)
        src_and_dest /= m_current_scale->x;
}

void UICore::ClientToScreenScaledHeight(float& src_and_dest) const
{
    if (m_currentPointType != IUIRender::pttLIT)
        src_and_dest /= m_current_scale->y;
}

void UICore::AlignPixel(float& src_and_dest) const
{
    if (m_currentPointType != IUIRender::pttLIT)
        src_and_dest = (float)iFloor(src_and_dest);
}

void UICore::PushScissor(const Frect& r_tgt, bool overlapped)
{
    if (UI().m_currentPointType == IUIRender::pttLIT)
        return;

    Frect r_top = {0.0f, 0.0f, UI_BASE_WIDTH, UI_BASE_HEIGHT};
    Frect result = r_tgt;
    if (!m_Scissors.empty() && !overlapped)
    {
        r_top = m_Scissors.top();
    }
    if (!result.intersection(r_top, r_tgt))
        result.set(0.0f, 0.0f, 0.0f, 0.0f);

    if (!(result.x1 >= 0 && result.y1 >= 0 && result.x2 <= UI_BASE_WIDTH && result.y2 <= UI_BASE_HEIGHT))
    {
        Msg("! r_tgt [%.3f][%.3f][%.3f][%.3f]", r_tgt.x1, r_tgt.y1, r_tgt.x2, r_tgt.y2);
        Msg("! result [%.3f][%.3f][%.3f][%.3f]", result.x1, result.y1, result.x2, result.y2);
        VERIFY(result.x1 >= 0 && result.y1 >= 0 && result.x2 <= UI_BASE_WIDTH && result.y2 <= UI_BASE_HEIGHT);
    }
    m_Scissors.push(result);

    result.lt.x = ClientToScreenScaledX(result.lt.x);
    result.lt.y = ClientToScreenScaledY(result.lt.y);
    result.rb.x = ClientToScreenScaledX(result.rb.x);
    result.rb.y = ClientToScreenScaledY(result.rb.y);

    Irect r;
    r.x1 = iFloor(result.x1);
    r.x2 = iFloor(result.x2 + 0.5f);
    r.y1 = iFloor(result.y1);
    r.y2 = iFloor(result.y2 + 0.5f);
    GEnv.UIRender->SetScissor(&r);
}

void UICore::PopScissor()
{
    if (UI().m_currentPointType == IUIRender::pttLIT)
        return;

    VERIFY(!m_Scissors.empty());
    m_Scissors.pop();

    if (m_Scissors.empty())
        GEnv.UIRender->SetScissor(NULL);
    else
    {
        const Frect& top = m_Scissors.top();
        Irect tgt;
        tgt.lt.x = iFloor(ClientToScreenScaledX(top.lt.x));
        tgt.lt.y = iFloor(ClientToScreenScaledY(top.lt.y));
        tgt.rb.x = iFloor(ClientToScreenScaledX(top.rb.x));
        tgt.rb.y = iFloor(ClientToScreenScaledY(top.rb.y));

        GEnv.UIRender->SetScissor(&tgt);
    }
}

UICore::UICore()
{
    if (!GEnv.isDedicatedServer)
    {
        m_pUICursor = xr_new<CUICursor>();
        m_pFontManager = xr_new<CFontManager>();
    }
    else
    {
        m_pUICursor = nullptr;
        m_pFontManager = nullptr;
    }
    m_bPostprocess = false;

    OnDeviceReset();
    OnUIReset();

    m_current_scale = &m_scale_;
    g_current_font_scale.set(1.0f, 1.0f);
    m_currentPointType = IUIRender::pttTL;
}

void UICore::OnDeviceReset()
{
    m_scale_.set(float(Device.dwWidth) / UI_BASE_WIDTH, float(Device.dwHeight) / UI_BASE_HEIGHT);

    m_2DFrustum.CreateFromRect(Frect().set(0.0f, 0.0f, float(Device.dwWidth), float(Device.dwHeight)));
}

void UICore::OnUIReset()
{
    CUIXmlInitBase::DeleteColorDefs();
    CUITextureMaster::FreeTexInfo();

    ReadTextureInfo();
    CUIXmlInitBase::InitColorDefs();
}

UICore::~UICore()
{
    xr_delete(m_pFontManager);
    xr_delete(m_pUICursor);
    CUIXmlInitBase::DeleteColorDefs();
    CUITextureMaster::FreeTexInfo();
}

void UICore::ReadTextureInfo()
{
    string_path buf;
    FS_FileSet files;

    const auto ParseFileSet = [&](pcstr path)
    {
        FS.file_list(files, "$game_config$", FS_ListFiles,
            strconcat(sizeof(buf), buf, path, DELIMITER "textures_descr" DELIMITER "*.xml")
        );
        for (const auto& file : files)
        {
            string_path path, name;
            _splitpath(file.name.c_str(), nullptr, path, name, nullptr);
            xr_strcat(name, ".xml");
            path[xr_strlen(path) - 1] = '\0'; // cut the latest '\\'

            CUITextureMaster::ParseShTexInfo(path, name);
        }
    };

    ParseFileSet(UI_PATH_DEFAULT);

    if (0 != xr_strcmp(UI_PATH, UI_PATH_DEFAULT))
        ParseFileSet(UI_PATH);

    if (pSettings->section_exist("texture_desc"))
    {
        string256 single_item;

        cpcstr itemsList = pSettings->r_string("texture_desc", "files");
        const u32 itemsCount = _GetItemCount(itemsList);

        for (u32 i = 0; i < itemsCount; i++)
        {
            _GetItem(itemsList, i, single_item);
            xr_strcat(single_item, ".xml");
            CUITextureMaster::ParseShTexInfo(single_item);
        }
    }
}

void UICore::pp_start()
{
    m_bPostprocess = true;

    m_pp_scale_.set(float(GEnv.Render->getTarget()->get_width()) / float(UI_BASE_WIDTH),
        float(GEnv.Render->getTarget()->get_height()) / float(UI_BASE_HEIGHT));
    m_2DFrustumPP.CreateFromRect(Frect().set(0.0f, 0.0f, float(GEnv.Render->getTarget()->get_width()),
        float(GEnv.Render->getTarget()->get_height())));

    m_current_scale = &m_pp_scale_;

    g_current_font_scale.set(float(GEnv.Render->getTarget()->get_width()) / float(Device.dwWidth),
        float(GEnv.Render->getTarget()->get_height()) / float(Device.dwHeight));
}

void UICore::pp_stop()
{
    m_bPostprocess = false;
    m_current_scale = &m_scale_;
    g_current_font_scale.set(1.0f, 1.0f);
}

void UICore::RenderFont() { Font().Render(); }
bool UICore::is_widescreen()
{
    return (Device.dwWidth) / float(Device.dwHeight) > (UI_BASE_WIDTH / UI_BASE_HEIGHT + 0.01f);
}

float UICore::get_current_kx()
{
    float h = float(Device.dwHeight);
    float w = float(Device.dwWidth);

    float res = (h / w) / (UI_BASE_HEIGHT / UI_BASE_WIDTH);
    return res;
}

shared_str UICore::get_xml_name(pcstr path, pcstr fn)
{
    string_path str;
    if (!is_widescreen())
    {
        xr_sprintf(str, "%s", fn);
        if (NULL == strext(fn))
            xr_strcat(str, ".xml");
    }
    else
    {
        if (strext(fn))
        {
            xr_strcpy(str, fn);
            *strext(str) = 0;
            xr_strcat(str, "_16.xml");
        }
        else
            xr_sprintf(str, "%s_16", fn);

        string_path str_;
        if (!FS.exist(str_, "$game_config$", path, str))
        {
            xr_sprintf(str, "%s", fn);
            if (nullptr == strext(fn))
                xr_strcat(str, ".xml");
        }
    }
    return str;
}
