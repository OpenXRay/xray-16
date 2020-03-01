#pragma once

class AnselCamera : public CCameraBase
{
public:
    AnselCamera(IGameObject* p, u32 flags);
};

class AnselCameraEffector : public CEffectorCam
{

public:
    AnselCameraEffector();

    BOOL ProcessCam(SCamEffectorInfo& info) override;
};

class AnselManager : public CGameObject, public pureFrame
{
    XRay::Module anselModule;
    AnselCamera camera;
    AnselCameraEffector effector;
    CTimer timer; // for pause case (in demo mode)
    float timeDelta;

public:
    AnselManager();

    bool Load();
    void Unload();

    bool Init() const;

    static bool IsActive()
    {
        return Device.IsAnselActive;
    }

    void OnFrame() override;
};
