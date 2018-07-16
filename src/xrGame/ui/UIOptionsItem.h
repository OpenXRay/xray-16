#pragma once
#include "UIOptionsManager.h"

// fwd. decl.
struct xr_token;

class CUIOptionsItem
{
public:
    enum ESystemDepends
    {
        sdNothing,
        sdVidRestart,
        sdSndRestart,
        sdUIRestart,
        sdSystemRestart,
        sdApplyOnChange
    };

public:
    CUIOptionsItem();
    virtual ~CUIOptionsItem();
    virtual void AssignProps(const shared_str& entry, const shared_str& group);
    void SetSystemDepends(ESystemDepends val) { m_dep = val; }
    static CUIOptionsManager* GetOptionsManager() { return &m_optionsManager; }
    virtual void OnMessage(LPCSTR message);

    virtual void SetCurrentOptValue() = 0 {}; // opt->current
    virtual void SaveBackUpOptValue() = 0 {}; // current->backup
    virtual void SaveOptValue() = 0; // current->opt
    virtual void UndoOptValue() = 0; // backup->current
    virtual bool IsChangedOptValue() const = 0 {}; // backup!=current
    void OnChangedOptValue();

protected:
    void SendMessage2Group(LPCSTR group, LPCSTR message);

    // string
    LPCSTR GetOptStringValue();
    void SaveOptStringValue(LPCSTR val);
    // integer
    void GetOptIntegerValue(int& val, int& min, int& max);
    void SaveOptIntegerValue(int val);
    // float
    void GetOptFloatValue(float& val, float& min, float& max);
    void SaveOptFloatValue(float val);
    // bool
    bool GetOptBoolValue();
    void SaveOptBoolValue(bool val);
    // token
    LPCSTR GetOptTokenValue();
    const xr_token* GetOptToken();

    shared_str m_entry;
    ESystemDepends m_dep;

    static CUIOptionsManager m_optionsManager;
};
