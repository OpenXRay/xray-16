#include "stdafx.h"

#include "UIStatsWnd.h"
#include "UIXmlInit.h"
#include "../UI.h"
#include "../string_table.h"

//////////////////////////////////////////////////////////////////////////

const char * const STATS_XML = "stats.xml";

CUIStatsWnd::CUIStatsWnd(LPCSTR XML)
	: m_uHighlightedItem(0xffffffff)
{
	Init(XML);
}

CUIStatsWnd::~CUIStatsWnd()
{

}

void CUIStatsWnd::Init(LPCSTR XML)
{
	CUIXml uiXml;
	if (XML) strcpy_s(XML_NAME, XML);
	else strcpy_s(XML_NAME, STATS_XML);

	uiXml.Load(CONFIG_PATH, UI_PATH, XML_NAME);

	CUIXmlInit xml_init;
	Fvector2	pos, size;
	pos.set		(CUIXmlInit::ApplyAlignX(0, alCenter),CUIXmlInit::ApplyAlignY(0, alCenter));
	size.set	(UI_BASE_WIDTH, UI_BASE_HEIGHT);
	CUIWindow::SetWndPos(pos);
	CUIWindow::SetWndSize(size);
	// Читаем из xml файла параметры окна и контролов
	AttachChild(&UIFrameWnd);
	xml_init.InitFrameWindow(uiXml, "frame_window", 0, &UIFrameWnd);

	UIFrameWnd.AttachChild(&UIStatsList);
	xml_init.InitListWnd(uiXml, "list", 0, &UIStatsList);
	UIStatsList.SetMessageTarget(this);
	UIStatsList.EnableScrollBar(true);

	xml_init.InitMultiTextStatic(uiXml, "headers_mt_static", 0, &UIHeader);
	UIFrameWnd.AttachChild(&UIHeader);
}

//////////////////////////////////////////////////////////////////////////

CUIStatsListItem * CUIStatsWnd::AddItem()
{
	CUIStatsListItem *pNewItem = xr_new<CUIStatsListItem>();
	UIStatsList.AddItem<CUIListItem>(pNewItem); 
	UIStatsList.ScrollToBegin();

	CUIXml uiXml;
	uiXml.Load(CONFIG_PATH, UI_PATH, XML_NAME);

	pNewItem->XmlInit("list", uiXml);
//	pNewItem->SetMessageTarget(this);
	return pNewItem;
}

//////////////////////////////////////////////////////////////////////////

CUIStatsListItem * CUIStatsWnd::FindFrom(const u32 beg_pos, const char *strCaption)
{
	for (int i = 0; i < UIStatsList.GetItemsCount(); ++i)
	{
		CUIStatsListItem *pSLItem = smart_cast<CUIStatsListItem*>(UIStatsList.GetItem(i));
		R_ASSERT(beg_pos < pSLItem->FieldsVector.size());
		for (FIELDS_VECTOR_it it = pSLItem->FieldsVector.begin() + beg_pos; it < pSLItem->FieldsVector.end(); ++it)
		{
			if (0 == xr_strcmp(strCaption, (*it)->GetText()))
			{
				return pSLItem;
			}
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

void CUIStatsWnd::RemoveItemFrom(const u32 beg_pos, const char *strCaption)
{
	if (CUIStatsListItem *pSLItem = FindFrom(beg_pos, strCaption))
	{
		UIStatsList.RemoveItem(pSLItem->GetIndex());
	}
}

//////////////////////////////////////////////////////////////////////////

void CUIStatsWnd::HighlightItem(const u32 uItem)
{
	R_ASSERT(static_cast<int>(uItem) < UIStatsList.GetItemsCount());
	if (m_uHighlightedItem != uItem)
	{
		if (m_uHighlightedItem != 0xffffffff)
			smart_cast<CUIStatsListItem*>(UIStatsList.GetItem(m_uHighlightedItem))->Highlight(false);
		if (uItem != 0xffffffff)
			smart_cast<CUIStatsListItem*>(UIStatsList.GetItem(uItem))->Highlight(true);
		m_uHighlightedItem = uItem;
	}
}

//////////////////////////////////////////////////////////////////////////

void CUIStatsWnd::SelectItem(const u32 uItem)
{
	R_ASSERT(static_cast<int>(uItem) < UIStatsList.GetItemsCount());
	UIStatsList.SetFocusedItem(static_cast<signed int>(uItem));
}

//////////////////////////////////////////////////////////////////////////

void CUIStatsWnd::SetHeaderColumnText(const u32 headerItem, const shared_str &text)
{
	UIHeader.GetPhraseByIndex(headerItem)->str = CStringTable().translate(text);
}

//////////////////////////////////////////////////////////////////////////
//  CUIStatsListItem - класс элемента списка в листе
//////////////////////////////////////////////////////////////////////////

void CUIStatsListItem::XmlInit(const char *path, CUIXml &uiXml)
{
	CUIXmlInit	xml_init;
//	CUIStatic	*pStatic;
	CUIButton	*pButton;

	string256 buf;
	strconcat(sizeof(buf),buf, path, ":static");

	int tabsCount = uiXml.GetNodesNum(path, 0, "static");

	XML_NODE* tab_node = uiXml.NavigateToNode(path,0);
	uiXml.SetLocalRoot(tab_node);

	for (int i = 0; i < tabsCount; ++i)
	{
		pButton = xr_new<CUIButton>();
		pButton->SetAutoDelete(true);
		xml_init.InitStatic(uiXml, "static", i, pButton);
		pButton->SetTextAlignment(CGameFont::alLeft);
		AttachChild(pButton);
		FieldsVector.push_back(pButton);
	}

	FieldsVector[0]->SetEllipsis(1, 0);
}

//////////////////////////////////////////////////////////////////////////

void CUIStatsListItem::Highlight(bool bHighlight)
{
	for (FIELDS_VECTOR_it it = FieldsVector.begin(); it != FieldsVector.end(); ++it)
	{
		(*it)->HighlightItem(bHighlight);
	}
}

//////////////////////////////////////////////////////////////////////////

void CUIStatsListItem::SetSubItemColor(u32 uItemIndex, u32 uColor)
{
	R_ASSERT(uItemIndex < FieldsVector.size());
	FieldsVector[uItemIndex]->SetTextColor(uColor);
}