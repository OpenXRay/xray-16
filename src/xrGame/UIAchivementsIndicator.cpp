#include "StdAfx.h"
#include "UIAchivementsIndicator.h"
#include "ui/UIXmlInit.h"
#include "ui/UIGameLog.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "ui/UIPdaMsgListItem.h"
#include "profile_data_types.h"

CUIAchivementIndicator::CUIAchivementIndicator()
{
    m_achivement_log = new CUIGameLog();
    AttachChild(m_achivement_log);
    CUIXml tmp_xml;
    tmp_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_mp_achivements.xml");
    CUIXmlInit::InitWindow(tmp_xml, "mp_achivement_wnd", 0, this);
    CUIXmlInit::InitScrollView(tmp_xml, "mp_achivement_wnd:achivement_list", 0, m_achivement_log);
}

CUIAchivementIndicator::~CUIAchivementIndicator() { xr_delete(m_achivement_log); }
void CUIAchivementIndicator::Update() { inherited::Update(); }
void CUIAchivementIndicator::AddAchivement(
    shared_str const& achivement_name, shared_str const& color_animation, u32 const width, u32 const height)
{
    CUIPdaMsgListItem* tmp_item = m_achivement_log->AddPdaMessage();
    VERIFY(tmp_item);
    tmp_item->UIIcon.InitTexture(achivement_name.c_str());
    tmp_item->SetColorAnimation(color_animation.c_str(), LA_ONLYALPHA | LA_TEXTURECOLOR);
    Fvector2 sz;
    sz.set(int(width), int(height));
    tmp_item->SetWndSize(sz);
    tmp_item->UIIcon.SetWndSize(sz);
}
