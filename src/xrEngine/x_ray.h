#ifndef __X_RAY_H__
#define __X_RAY_H__

// refs
class ENGINE_API CGameFont;
class ILoadingScreen;

// definition
class ENGINE_API CApplication : public pureFrame, public IEventReceiver
{
    // levels
    struct sLevelInfo
    {
        char* folder;
        char* name;
    };

private:
    ILoadingScreen* loadingScreen;

    int max_load_stage;

    int load_stage;

    u32 ll_dwReference;
    bool loaded;

private:
    EVENT eQuit;
    EVENT eStart;
    EVENT eStartLoad;
    EVENT eDisconnect;
    EVENT eConsole;
    EVENT eStartMPDemo;

    void Level_Append(pcstr lname);

public:
    CGameFont* pFontSystem;

    bool IsLoaded() { return loaded; }
    // Levels
    xr_vector<sLevelInfo> Levels;
    u32 Level_Current;
    void Level_Scan();
    int Level_ID(pcstr name, pcstr ver, bool bSet);
    void Level_Set(u32 ID);
    void LoadAllArchives();
    static CInifile* GetArchiveHeader(pcstr name, pcstr ver);

    // Loading
    void LoadBegin();
    void LoadEnd();
    void LoadTitleInt(pcstr str1, pcstr str2, pcstr str3);
    void LoadStage();
    void LoadSwitch();
    void LoadDraw();
    void LoadForceDrop();
    void LoadForceFinish();

    void SetLoadStageTitle(pcstr ls_title);

    virtual void OnEvent(EVENT E, u64 P1, u64 P2);

    // Other
    CApplication();
    virtual ~CApplication();

    virtual void OnFrame();
    void load_draw_internal(bool precaching = false);

    void SetLoadingScreen(ILoadingScreen* newScreen);
    void DestroyLoadingScreen();
    void ShowLoadingScreen(bool show);
};

extern ENGINE_API CApplication* pApp;

#endif //__XR_BASE_H__
