#ifndef __X_RAY_H__
#define __X_RAY_H__

// refs
class ENGINE_API CGameFont;
class ILoadingScreen;

// definition
class ENGINE_API CApplication : public pureFrame, public IEventReceiver
{
    EVENT eQuit;
    EVENT eConsole;

public:
    void OnEvent(EVENT E, u64 P1, u64 P2) override;

    // Other
    CApplication();
    virtual ~CApplication();

    virtual void OnFrame();
};

extern ENGINE_API CApplication* pApp;

#endif //__XR_BASE_H__
