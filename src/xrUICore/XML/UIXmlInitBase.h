#pragma once
#include "xrUICore/XML/xrUIXmlParser.h"
#include "xrCore/_rect.h"
#include "xrCommon/xr_map.h"

class ITextureOwner;
class CUIWindow;
class CUIFrameWindow;
class CUIStaticItem;
class CUIStatic;
class CUICheckButton;
class CUICustomSpin;
class CUIButton;
class CUI3tButton;
class CUIDragDropList;
class CUIProgressBar;
class CUIProgressShape;
class CUITabControl;
class CUIFrameLineWnd;
class CUITextFrameLineWnd;
class CUIEditBoxEx;
class CUIEditBox;
class CUICustomEdit;
class CUIAnimatedStatic;
class CUISleepStatic;
class CUIOptionsItem;
class CUIScrollView;
class CUIListWnd;
class CUIListBox;
class CUIDragDropListEx;
class CUIComboBox;
class CUITrackBar;
class CUILines;
class CUITextWnd;
class CGameFont;

class XRUICORE_API CUIXmlInitBase
{
public:
    CUIXmlInitBase();
    virtual ~CUIXmlInitBase();

    static bool InitWindow(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pWnd, bool fatal = true);
    static bool InitFrameWindow(CUIXml& xml_doc, LPCSTR path, int index, CUIFrameWindow* pWnd, bool fatal = true);
    static bool InitFrameLine(CUIXml& xml_doc, LPCSTR path, int index, CUIFrameLineWnd* pWnd, bool fatal = true);
    static bool InitTextFrameLine(CUIXml& xml_doc, LPCSTR path, int index, CUITextFrameLineWnd* pWnd, bool fatal = true);
    static bool InitCustomEdit(CUIXml& xml_doc, LPCSTR paht, int index, CUICustomEdit* pWnd, bool fatal = true);
    static bool InitEditBox(CUIXml& xml_doc, LPCSTR paht, int index, CUIEditBox* pWnd, bool fatal = true);
    static bool InitStatic(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd, bool fatal = true);
    static bool InitTextWnd(CUIXml& xml_doc, LPCSTR path, int index, CUITextWnd* pWnd, bool fatal = true);
    static bool InitCheck(CUIXml& xml_doc, LPCSTR path, int index, CUICheckButton* pWnd, bool fatal = true);
    static bool InitSpin(CUIXml& xml_doc, LPCSTR path, int index, CUICustomSpin* pWnd, bool fatal = true);
    static bool InitText(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd);
    static bool InitText(CUIXml& xml_doc, LPCSTR path, int index, CUILines* pLines);
    static bool Init3tButton(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd, bool fatal = true);
    static bool InitProgressBar(CUIXml& xml_doc, LPCSTR path, int index, CUIProgressBar* pWnd, bool fatal = true);
    static bool InitProgressShape(CUIXml& xml_doc, LPCSTR path, int index, CUIProgressShape* pWnd, bool fatal = true);
    static bool InitFont(CUIXml& xml_doc, LPCSTR path, int index, u32& color, CGameFont*& pFnt);
    static bool InitTabControl(CUIXml& xml_doc, LPCSTR path,
        int index, CUITabControl* pWnd, bool fatal = true, bool defaultIdsAllowed = false);
    static bool InitAnimatedStatic(CUIXml& xml_doc, LPCSTR path, int index, CUIAnimatedStatic* pWnd, bool fatal = true);
    static bool InitTextureOffset(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd);
    static bool InitSound(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd);
    static bool InitMultiTexture(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd);
    static bool InitTexture(CUIXml& xml_doc, LPCSTR path, int index, ITextureOwner* pWnd, bool fatal = true);
    static bool InitOptionsItem(CUIXml& xml_doc, LPCSTR paht, int index, CUIOptionsItem* pWnd);
    static bool InitScrollView(CUIXml& xml_doc, LPCSTR path, int index, CUIScrollView* pWnd, bool fatal = true);
    static bool InitListWnd(CUIXml& xml_doc, pcstr path, int index, CUIListWnd* pWnd, bool fatal = true);
    static bool InitListBox(CUIXml& xml_doc, LPCSTR path, int index, CUIListBox* pWnd, bool fatal = true);
    static bool InitComboBox(CUIXml& xml_doc, LPCSTR path, int index, CUIComboBox* pWnd);
    static bool InitTrackBar(CUIXml& xml_doc, LPCSTR path, int index, CUITrackBar* pWnd, bool fatal = true);
    static Frect GetFRect(CUIXml& xml_doc, LPCSTR path, int index);
    static u32 GetColor(CUIXml& xml_doc, LPCSTR path, int index, u32 def_clr);

    static bool InitAlignment(CUIXml& xml_doc, const char* path, int index, float& x, float& y, CUIWindow* pWnd);

    static void InitAutoStaticGroup(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pParentWnd);
    static void InitAutoFrameLineGroup(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pParentWnd);

    static float ApplyAlignX(float coord, u32 align);
    static float ApplyAlignY(float coord, u32 align);
    static void ApplyAlign(float& x, float& y, u32 align);

    // Initialize and store predefined colors
    using ColorDefs = xr_map<shared_str, u32>;

    static const ColorDefs* GetColorDefs()
    {
        R_ASSERT(m_pColorDefs);
        return m_pColorDefs;
    }

    static void InitColorDefs();
    static void DeleteColorDefs() { xr_delete(m_pColorDefs); }
    static void AssignColor(LPCSTR name, u32 clr);

private:
    static ColorDefs* m_pColorDefs;
};
