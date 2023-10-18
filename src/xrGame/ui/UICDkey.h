//
#pragma once
#include "xrUICore/EditBox/UIEditBox.h"

class CUICDkey final : public CUIEditBox
{
private:
    typedef CUIEditBox inherited;

public:
    CUICDkey();
    virtual void SetText(LPCSTR str) {}
    virtual LPCSTR GetText();

    // CUIOptionsItem
    virtual void SetCurrentOptValue(); // opt->current
    virtual void SaveBackUpOptValue(); // current->backup
    virtual void SaveOptValue(); // current->opt
    virtual void UndoOptValue(); // backup->current
    virtual bool IsChangedOptValue() const; // backup!=current

    void CreateCDKeyEntry();
    void assign_callbacks();

    virtual void Show(bool status);
    virtual void Draw();
    virtual void OnFocusLost();

    pcstr GetDebugType() override { return "CUICDkey"; }

private:
    void paste_from_clipboard();

private:
    string512 m_opt_backup_value;
    bool m_view_access;
}; // class CUICDkey

class CUIMPPlayerName final : public CUIEditBox
{
private:
    typedef CUIEditBox inherited;

public:
    CUIMPPlayerName() = default;

    //	virtual	void	SetText			(LPCSTR str) {}

    //	virtual void	SetCurrentValue();
    //	virtual void	SaveValue();
    //	virtual bool	IsChanged();

    void OnFocusLost() override;

    pcstr GetDebugType() override { return "CUIMPPlayerName"; }
}; // class CUIMPPlayerName

extern void GetCDKey_FromRegistry(char* cdkey);
extern void WriteCDKey_ToRegistry(pstr cdkey);
extern void GetPlayerName_FromRegistry(char* name, u32 const name_size);
extern void WritePlayerName_ToRegistry(pstr name);
