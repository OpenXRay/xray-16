#pragma once

#include "UIDialogWnd.h"
#include "xrScriptEngine/script_space_forward.hpp"

struct SCallbackInfo;

class CUIDialogWndEx : public CUIDialogWnd, public FactoryObjectBase
{
    typedef CUIDialogWnd inherited;
    typedef xr_vector<SCallbackInfo*> CALLBACKS;
    typedef CALLBACKS::iterator CALLBACK_IT;

private:
    CALLBACKS m_callbacks;
    void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = nullptr) override;
    SCallbackInfo* NewCallback();

public:
    bool Load(pcstr xml_name);
    void Register(CUIWindow* pChild);
    void Register(CUIWindow* pChild, pcstr name);

    CUIDialogWndEx();
    ~CUIDialogWndEx() override;

    void AddCallback(pcstr control_id, s16 event, const luabind::functor<void>& lua_function);
    void AddCallback(pcstr control_id, s16 event, const luabind::functor<void>& functor, const luabind::object& object);

    template <typename T>
    T* GetControl(pcstr name);

    pcstr GetDebugType() override { return "CUIDialogWndEx"; }
};
