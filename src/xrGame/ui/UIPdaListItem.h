//////////////////////////////////////////////////////////////////////
// UIPdaListItem.h: элемент окна списка в PDA
// для отображения информации о контакте PDA
//////////////////////////////////////////////////////////////////////

#pragma once
#include "xrUICore/Windows/UIWindow.h"

class CUIFrameWindow;
class CUICharacterInfo;
class CInventoryOwner;
class CPda;

class CUIPdaListItem : public CUIWindow
{
private:
    typedef CUIWindow inherited;
public:
    CUIPdaListItem();
    virtual ~CUIPdaListItem();
    virtual void Init(u16 dark);
    CPda* m_data;
protected:
    //информация о персонаже
    CUIFrameWindow* UIMask;
    CUICharacterInfo* UIInfo;
};
