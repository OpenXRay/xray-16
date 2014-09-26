#pragma once
#include "UIWindow.h"

class CUIStatic;
class CEncyclopediaArticle;

class CUIEncyclopediaArticleWnd :public CUIWindow
{
typedef	CUIWindow		inherited;

CUIStatic*				m_UIImage;
CUIStatic*				m_UIText;
CEncyclopediaArticle*	m_Article;

protected:
			void		AdjustLauout				();

public:
					CUIEncyclopediaArticleWnd		();
	virtual			~CUIEncyclopediaArticleWnd		();
			void	Init							(LPCSTR xml_name, LPCSTR start_from);
			void	SetArticle						(CEncyclopediaArticle*);
			void	SetArticle						(LPCSTR);
};