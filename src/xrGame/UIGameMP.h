#ifndef UIGAMEMP_H
#define UIGAMEMP_H

#include "UIGameCustom.h"

class CUIDemoPlayControl;
class CUIServerInfo;
class game_cl_mp;

class UIGameMP : public CUIGameCustom
{
    typedef CUIGameCustom inherited;

public:
    UIGameMP();
    virtual ~UIGameMP();

    void ShowDemoPlayControl();

    void SetServerLogo(u8 const* data_ptr, u32 data_size);
    void SetServerRules(u8 const* data_ptr, u32 data_size);

    bool IsServerInfoShown();
    bool ShowServerInfo(); // shows only if it has some info ...

    virtual bool IR_UIOnKeyboardPress(int dik);
    virtual bool IR_UIOnKeyboardRelease(int dik);
    virtual void SetClGame(game_cl_GameState* g);

protected:
    CUIDemoPlayControl* m_pDemoPlayControl;
    CUIServerInfo* m_pServerInfo;
    game_cl_mp* m_game;
}; // class UIGameMP

#endif //#ifndef UIGAMEMP_H
