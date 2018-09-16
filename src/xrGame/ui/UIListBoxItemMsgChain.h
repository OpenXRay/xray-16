#ifndef UILISTBOXITEMMSGCHAIN_H_INCLUDED
#define UILISTBOXITEMMSGCHAIN_H_INCLUDED

#include "UIListBoxItem.h"

class CUIListBoxItemMsgChain : public CUIListBoxItem
{
    typedef CUIListBoxItem inherited;

public:
    CUIListBoxItemMsgChain(float height) : CUIListBoxItem(height){};
    virtual ~CUIListBoxItemMsgChain(){};

    virtual bool OnMouseDown(int mouse_btn);
};

#endif //#ifndef UILISTBOXITEMMSGCHAIN_H_INCLUDED
