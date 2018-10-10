#ifndef SPECTATOR_CAMERA_FIRST_EYE
#define SPECTATOR_CAMERA_FIRST_EYE

#include "xrCore/FTimer.h"
#include "CameraFirstEye.h"

class CSpectrCameraFirstEye : public CCameraFirstEye
{
private:
    typedef CCameraFirstEye inherited;
    float const& m_fTimeDelta;

public:
    CSpectrCameraFirstEye(float const& fTimeDelta, IGameObject* p, u32 flags = 0);
    virtual ~CSpectrCameraFirstEye();
    CSpectrCameraFirstEye& operator=(CSpectrCameraFirstEye& copy){};

    virtual void Move(int cmd, float val = 0, float factor = 1.0f);
}; // class SpectrCameraFirstEye

#endif //#ifndef SPECTATOR_CAMERA_FIRST_EYE
