#pragma once

#include "UIDialogWnd.h"
#include "UIFrameWindow.h"
#include "UIListWnd.h"
#include "UIListItem.h"
#include "UIXmlInit.h"
#include "UIButton.h"
#include "UIMultiTextStatic.h"

DEF_VECTOR (FIELDS_VECTOR, CUIButton*)

// Класс для определения нового члена списка
class CUIStatsListItem: public CUIListItem
{
	typedef CUIListItem inherited;
public:
	virtual ~CUIStatsListItem() {};
	void XmlInit(const char *path, CUIXml &uiXml);
	void Highlight(bool bHighlight);
	void SetSubItemColor(u32 uItemIndex, u32 uColor);

	// поля записи
	FIELDS_VECTOR FieldsVector;
};

class CUIStatsWnd: public CUIDialogWnd
{
private:
	typedef CUIDialogWnd inherited;

	string1024	XML_NAME;
public:
	CUIStatsWnd(LPCSTR XML = NULL);
	virtual ~CUIStatsWnd();

	virtual void Init(LPCSTR XML = NULL);
//	virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);

	// Добавить 1 элемент. Заполнить поля необходимо самостоятельно. Возвращает указатель 
	// на добавленный элемент
	CUIStatsListItem * AddItem();
	// Получить элемент, при п		омощи поиска в полях по строке. Можно искать
	// элемент начиная с заданного номера
	CUIStatsListItem * FindFrom(u32 beg_pos, const char *strCaption);
	// Удалить элемент в котором есть статик с текстом strCaption. В каждом Item'е поиск 
	// начать с позиции beg_pos
	void RemoveItemFrom(u32 beg_pos, const char *strCaption);
	// Подсветить нужный элемент
	void HighlightItem(u32 uItem);
	// Получить номер подсвеченого эл-та
	u32	GetHighlightedItem() { return m_uHighlightedItem; }
	// Выделить нужный элемент
	void SelectItem(u32 uItem);
	// Установить текст заголовка нужной колонки
	void SetHeaderColumnText(u32 headerItem, const shared_str &text);
	
	Frect GetFrameRect () { return UIFrameWnd.GetWndRect();};
	void RemoveItem (const u32 Index) {UIStatsList.RemoveItem(Index);};

	CUIFrameWindow*		GetFrameWindow	()	{return &UIFrameWnd;};
protected:
//	CUIButton			UIBtn;
	// Фрейм - оболочка
	CUIFrameWindow		UIFrameWnd;
	// Лист для отображения списка статичтики игроков
	CUIListWnd			UIStatsList;
	// Подсвеченый элемент
	u32					m_uHighlightedItem;
	// Заголовок
	CUIMultiTextStatic	UIHeader;
};
