#ifndef UI_ACHIVEMENTS_INDICATOR_INCLUDED
#define UI_ACHIVEMENTS_INDICATOR_INCLUDED

#include "xrUICore/Static/UIStatic.h"
#include "ui/KillMessageStruct.h"
#include "xrCore/Containers/AssociativeVector.hpp"

class CUIXml;
class CUIGameLog;

class CUIAchivementIndicator final : public CUIWindow
{
    typedef CUIWindow inherited;

public:
    CUIAchivementIndicator();
    virtual ~CUIAchivementIndicator();
    virtual void Update();

    void AddAchivement(
        shared_str const& achivement_name, shared_str const& color_animation, u32 const width, u32 const height);

    pcstr GetDebugType() override { return "CUIAchivementIndicator"; }

protected:
    CUIGameLog* m_achivement_log;
};

#endif //#ifndef UI_ACHIVEMENTS_INDICATOR_INCLUDED
