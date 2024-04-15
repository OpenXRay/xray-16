#pragma once
#include "UIOptionsManager.h"

// fwd. decl.
struct xr_token;

class XRUICORE_API CUIOptionsItem
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

    virtual void SetCurrentOptValue() {} // opt->current
    virtual void SaveBackUpOptValue() {} // current->backup
    virtual void SaveOptValue(); // current->opt
    virtual void UndoOptValue(); // backup->current
    virtual bool IsChangedOptValue() const = 0; // backup!=current
    void OnChangedOptValue();

protected:
    void SendMessage2Group(pcstr group, pcstr message);

    // string
    pcstr GetOptStringValue() const;
    void SaveOptStringValue(pcstr val) const;
    // integer
    void GetOptIntegerValue(int& val, int& min, int& max) const;
    void SaveOptIntegerValue(int val) const;
    // float
    void GetOptFloatValue(float& val, float& min, float& max) const;
    void SaveOptFloatValue(float val) const;
    // bool
    bool GetOptBoolValue() const;
    void SaveOptBoolValue(bool val) const;
    // token
    pcstr GetOptTokenValue() const;
    const xr_token* GetOptToken() const;

    Fvector3 GetOptVector3Value() const;
    void SaveOptVector3Value(Fvector3 val) const;

    Fvector4 GetOptVector4Value() const;
    void SaveOptVector4Value(Fvector4 val) const;

    shared_str m_entry;
    ESystemDepends m_dep;

    static CUIOptionsManager m_optionsManager;
};
