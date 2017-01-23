#ifndef UIPANELSCLASSFACTORY
#define UIPANELSCLASSFACTORY

#include "UIPlayerItem.h"
#include "UITeamState.h"

class UITeamPanels;

class UIPanelsClassFactory
{
private:
public:
    UIPanelsClassFactory();
    ~UIPanelsClassFactory();

    UITeamState* CreateTeamPanel(shared_str const& teamName, UITeamPanels* teamPanels);
};

#endif