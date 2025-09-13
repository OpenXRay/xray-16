#include "stdafx.h"
#include "UIEncyclopediaArticleWnd.h"
#include "xrUICore/Static/UIStatic.h"
#include "../encyclopedia_article.h"
#include "UIXmlInit.h"

CUIEncyclopediaArticleWnd::CUIEncyclopediaArticleWnd() :
    CUIWindow("CUIEncyclopediaArticleWnd"),
    m_UIImage(NULL),
    m_UIText(NULL),
    m_Article(NULL)
{
}

CUIEncyclopediaArticleWnd::~CUIEncyclopediaArticleWnd()
{
}

bool CUIEncyclopediaArticleWnd::Init(LPCSTR xml_name, LPCSTR start_from)
{
    CUIXml uiXml;

    if (!uiXml.Load(CONFIG_PATH, UI_PATH, xml_name, ShadowOfChernobylMode))
        return false;

    CUIXmlInit xml_init;

    string512 str;

    strcpy_s(str, sizeof(str), start_from);
    xml_init.InitWindow(uiXml, str, 0, this);

    strconcat(sizeof(str), str, start_from, ":image");
    m_UIImage = xr_new<CUIStatic>("Image static"); // Конструктор с именем
    m_UIImage->SetAutoDelete(true);
    xml_init.InitStatic(uiXml, str, 0, m_UIImage);
    AttachChild(m_UIImage);

    strconcat(sizeof(str), str, start_from, ":text_cont");
    m_UIText = xr_new<CUIStatic>("Text static"); // Конструктор с именем
    m_UIText->SetAutoDelete(true);
    xml_init.InitStatic(uiXml, str, 0, m_UIText);
    AttachChild(m_UIText);

    return true;
}

void CUIEncyclopediaArticleWnd::SetArticle(CEncyclopediaArticle* article)
{
    if (article->data() && article->data()->image.GetConstHeading()) {
        m_UIImage->SetShader(article->data()->image.GetShader());
        m_UIImage->SetWndRect(article->data()->image.GetStaticItem()->GetTextureRect());
        m_UIImage->SetWndSize(article->data()->image.GetWndSize());

        float img_x = (GetWidth() - m_UIImage->GetWidth()) / 2.0f;
        img_x = _max(0.0f, img_x);
        m_UIImage->SetWndPos(Fvector2(img_x, m_UIImage->GetWndPos().y));
    };
    m_UIText->SetText(*CStringTable().translate(article->data()->text.c_str()));
    m_UIText->AdjustHeightToText();

    AdjustLauout();
}

void CUIEncyclopediaArticleWnd::AdjustLauout()
{
    m_UIText->SetWndPos(Fvector2(m_UIText->GetWndPos().x, m_UIImage->GetWndPos().y + m_UIImage->GetHeight()));
    SetHeight(m_UIImage->GetWndPos().y + m_UIImage->GetHeight() + m_UIText->GetHeight());
}

void CUIEncyclopediaArticleWnd::SetArticle(LPCSTR article)
{
    CEncyclopediaArticle A;
    A.Load(article);
    SetArticle(&A);
}
