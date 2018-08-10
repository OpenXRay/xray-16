#pragma once
#include "xrUICore/Windows/UIFrameLineWnd.h"

class CUITextWnd;
class CUIStatic;

class XRUICORE_API CUIListBoxItem : public CUIFrameLineWnd, public CUISelectable
{
    typedef CUIFrameLineWnd inherited;

public:
    CUIListBoxItem(float height);

    virtual void Draw();
    virtual bool OnMouseDown(int mouse_btn);
    virtual void OnFocusReceive();
    void InitDefault();
    void SetTAG(u32 value);
    u32 GetTAG();

    void SetData(void* data);
    void* GetData();

    CUITextWnd* AddTextField(LPCSTR txt, float width);
    CUIStatic* AddIconField(float width);

    CUITextWnd* GetTextItem() { return m_text; }
    // TextControl
    void SetText(LPCSTR txt);
    LPCSTR GetText();
    void SetTextColor(u32 color);
    u32 GetTextColor();
    void SetFont(CGameFont* F);
    CGameFont* GetFont();

protected:
    CUITextWnd* m_text;
    u32 tag;
    void* pData;
    float FieldsLength() const;
};
