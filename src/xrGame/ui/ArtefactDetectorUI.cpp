#include "StdAfx.h"
#include "ArtefactDetectorUI.h"
#include "UIXmlInit.h"
#include "xrUICore/XML/xrUIXmlParser.h"

void CUIDetectorWave::SetVelocity(float v) { m_curr_v = v; }
void CUIDetectorWave::Update()
{
    Fvector2 P = GetWndPos();

    float dp = m_curr_v * Device.fTimeDelta;

    P.x += dp;
    if (P.x > 0)
        P.x -= m_step;
    else if (P.x < -(2 * m_step))
        P.x += m_step;

    SetWndPos(P);
    inherited::Update();
}

void CUIDetectorWave::InitFromXML(CUIXml& xml, LPCSTR path)
{
    CUIXmlInit::InitFrameLine(xml, path, 0, this);
    m_step = xml.ReadAttribFlt(path, 0, "step");
}
