#ifndef ACCOUNT_MANAGER_CONSOLE
#define ACCOUNT_MANAGER_CONSOLE

#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"

class CCC_CreateGameSpyAccount : public IConsole_Command
{
public:
    CCC_CreateGameSpyAccount(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, "Creates GameSpy account:gs_create_account <nick> <unique_nick> <email> <password>");
    }
}; // CCC_CreateGameSpyAccount

class CCC_GapySpyListProfiles : public IConsole_Command
{
public:
    CCC_GapySpyListProfiles(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I) { xr_strcpy(I, "Lists account profiles : gs_list_profiles <email> <password>"); }
}; // CCC_GapySpyListProfiles

class CCC_GameSpyLogin : public IConsole_Command
{
public:
    CCC_GameSpyLogin(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I) { xr_strcpy(I, "Logins to GameSpy: gs_login <email> <nick> <password>"); }
}; // CCC_GameSpyLogin

class CCC_GameSpyLogout : public IConsole_Command
{
public:
    CCC_GameSpyLogout(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I) { xr_strcpy(I, "Logouts from the GameSpy session."); }
}; // CCC_GameSpyLogout

class CCC_GameSpyPrintProfile : public IConsole_Command
{
public:
    CCC_GameSpyPrintProfile(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I) { xr_strcpy(I, "Prints current profile information."); }
}; // CCC_GameSpyPrintProfile

class CCC_GameSpySuggestUNicks : public IConsole_Command
{
public:
    CCC_GameSpySuggestUNicks(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I) { xr_strcpy(I, "Suggests unique nicks"); }
}; // CCC_GameSpySuggestUNicks

class CCC_GameSpyRegisterUniqueNick : public IConsole_Command
{
public:
    CCC_GameSpyRegisterUniqueNick(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I) { xr_strcpy(I, "Registers new unique nick to the current profile"); }
}; // CCC_GameSpySuggestUNicks

class CCC_GameSpyDeleteProfile : public IConsole_Command
{
public:
    CCC_GameSpyDeleteProfile(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I) { xr_strcpy(I, "Deletes current profile."); }
}; // CCC_GameSpyDeleteProfile

class CCC_GameSpyProfile : public IConsole_Command
{
public:
    CCC_GameSpyProfile(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args);
    virtual void Info(TInfo& I) { xr_strcpy(I, "Loads current profile information."); }
}; // CCC_GameSpyProfile

#endif //#ifndef ACCOUNT_MANAGER_CONSOLE
