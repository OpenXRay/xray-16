#include "StdAfx.h"
#include "UIEncyclopediaArticleWnd.h"
#include "xrUICore/Static/UIStatic.h"
#include "../encyclopedia_article.h"
#include "UIXmlInit.h"
#include "xrEngine/StringTable/StringTable.h"

CUIEncyclopediaArticleWnd::CUIEncyclopediaArticleWnd() : CUIWindow("CUIEncyclopediaArticleWnd"), m_Article(NULL) {}

CUIEncyclopediaArticleWnd::~CUIEncyclopediaArticleWnd() {}

void CUIEncyclopediaArticleWnd::Init(LPCSTR xml_name, LPCSTR start_from)
{
    CUIXml uiXml;
    bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_name);
    R_ASSERT3(xml_result, "xml file not found", xml_name);

    CUIXmlInit xml_init;

    string512 str;

    strcpy_s(str, sizeof(str), start_from);
    xml_init.InitWindow(uiXml, str, 0, this);

    strconcat(sizeof(str), str, start_from, ":image");
    m_UIImage = xr_new<CUIStatic>();
    m_UIImage->SetAutoDelete(true);
    xml_init.InitStatic(uiXml, str, 0, m_UIImage);
    AttachChild(m_UIImage);

    strconcat(sizeof(str), str, start_from, ":text_cont");
    m_UIText = xr_new<CUIStatic>();
    m_UIText->SetAutoDelete(true);
    xml_init.InitStatic(uiXml, str, 0, m_UIText);
    if (ShadowOfChernobylMode)
        m_UIText->SetTextAlignment(CGameFont::alLeft);
    AttachChild(m_UIText);
}

void CUIEncyclopediaArticleWnd::SetArticle(CEncyclopediaArticle* article)
{
    const bool has_image = article->data()->image.GetShader()->inited();
    m_UIImage->Show(has_image);
    if (has_image)
    {
        m_UIImage->SetShader(article->data()->image.GetShader());
        m_UIImage->SetTextureRect(article->data()->image.GetTextureRect());
        m_UIImage->SetWndSize(article->data()->image.GetWndSize());

        float img_x = (GetWidth() - m_UIImage->GetWidth()) / 2.0f;
        img_x = _max(0.0f, img_x);
        m_UIImage->SetWndPos(img_x, m_UIImage->GetWndPos().y);
    };
    m_UIText->SetText(CStringTable().translate(article->data()->text.c_str()).c_str());
    if (ShadowOfChernobylMode)
        m_UIText->SetTextAlignment(CGameFont::alLeft);
    m_UIText->AdjustHeightToText();

    AdjustLauout();
}

void CUIEncyclopediaArticleWnd::Draw()
{
    if (ShadowOfChernobylMode)
        m_UIText->SetTextAlignment(CGameFont::alLeft);

    inherited::Draw();
}

void CUIEncyclopediaArticleWnd::AdjustLauout()
{
    const float image_bottom = m_UIImage->IsShown() ? m_UIImage->GetWndPos().y + m_UIImage->GetHeight() : 0.0f;
    m_UIText->SetWndPos(m_UIText->GetWndPos().x, image_bottom);
    SetHeight(image_bottom + m_UIText->GetHeight());
}

void CUIEncyclopediaArticleWnd::SetArticle(LPCSTR article)
{
    CEncyclopediaArticle A;
    A.Load(article);
    SetArticle(&A);
}
