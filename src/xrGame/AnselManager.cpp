#include "StdAfx.h"

#ifdef WINDOWS
#define ANSEL_SDK_DELAYLOAD
#include "AnselSDK.h"
#endif

#include "AnselManager.h"
#include "holder_custom.h"
#include "SDL.h"
#include "SDL_syswm.h"

ENGINE_API extern BOOL bShowPauseString;
ENGINE_API extern BOOL g_bDisableRedText;
BOOL stored_red_text;

/* XXX: Support camera move
 * Before just enabling this we should ensure that the user won't use Ansel for cheating
 * Things that should be in Ansel mode:
 * 1. Camera collision with walls
 * 2. Fly range limit within which the player can fly
 */

AnselManager::AnselManager() : anselModule(nullptr), camera(this, 0), timeDelta(EPS_S)
{
    timer.Start();
}

bool AnselManager::Load()
{
#ifdef XR_X64
    constexpr pcstr anselName = "AnselSDK64";
#else
    constexpr pcstr anselName = "AnselSDK32";
#endif
    anselModule = XRay::LoadModule(anselName);

    return anselModule->IsLoaded();
}

void AnselManager::Unload()
{
    anselModule = nullptr;
}

bool AnselManager::Init() const
{
#ifdef WINDOWS
    if (anselModule->IsLoaded() && ansel::isAnselAvailable())
    {
        ansel::Configuration config;

        config.titleNameUtf8 = u8"S.T.A.L.K.E.R.: Call of Pripyat (OpenXRay)";
        config.right = { 1, 0, 0 };
        config.up = { 0, 1, 0 };
        config.forward = { 0, 0, 1 };
        config.fovType = ansel::kVerticalFov;

        // XXX: Support camera move
        config.isCameraTranslationSupported = false;

        config.rotationalSpeedInDegreesPerSecond = 220.0f;
        config.translationalSpeedInWorldUnitsPerSecond = 50.0f;
        config.captureLatency = 0;
        config.captureSettleLatency = 0;

        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(Device.m_sdlWnd, &info))
            config.gameWindowHandle = info.info.win.window;
        else
        {
            Log("! Couldn't get window information required for Nvidia Ansel: ", SDL_GetError());
            return false;
        }

        static auto mutable_this = const_cast<AnselManager*>(this);

        config.startSessionCallback = [](ansel::SessionConfiguration& conf, void* /*context*/)
        {
            if (!g_pGameLevel)
                return ansel::kDisallowed;

            // close main menu if it is open
            if (MainMenu()->IsActive())
                MainMenu()->Activate(false);

            conf.maximumFovInDegrees = 140;

            bShowPauseString = FALSE;

            Device.IsAnselActive = true;
            Device.Pause(TRUE, TRUE, TRUE, "Nvidia Ansel");

            stored_red_text = g_bDisableRedText;
            g_bDisableRedText = TRUE;
            
            g_pGameLevel->Cameras().AddCamEffector(new AnselCameraEffector());
            Device.seqFrame.Add(mutable_this, REG_PRIORITY_CAPTURE);

            CCameraBase* C = NULL;
            if (IsGameTypeSingle() && Actor())
            {
                if (!Actor()->Holder())
                    C = Actor()->cam_Active();
                else
                    C = Actor()->Holder()->Camera();
            }
            else
            {
                const auto spectr = smart_cast<CSpectator*>(Level().CurrentControlEntity());
                if (spectr)
                    C = spectr->cam_Active(); // updating spectator in pause (pause ability of demo play)
                else
                {
                    Log("! Failed to find camera for Ansel");
                    return ansel::kDisallowed;
                }
            }

            mutable_this->camera.Set(C->Position(), C->Direction(), C->vNormal);

            mutable_this->camera.f_fov = C->Fov();
            mutable_this->camera.f_aspect = C->Aspect();

            return ansel::kAllowed;
        };

        config.stopSessionCallback = [](void* /*context*/)
        {
            bShowPauseString = TRUE;

            Device.IsAnselActive = false;
            Device.Pause(FALSE, FALSE, FALSE, "Nvidia Ansel");

            g_bDisableRedText = stored_red_text;

            g_pGameLevel->Cameras().RemoveCamEffector(cefAnsel);
            Device.seqFrame.Remove(mutable_this);
        };

        config.startCaptureCallback = [](const ansel::CaptureConfiguration& /*conf*/, void* /*context*/)
        {
            // turn non-uniform full screen effects like vignette off here
        };
        config.stopCaptureCallback = [](void* /*context*/)
        {
            // turn disabled effects back on here
        };

        const auto status = ansel::setConfiguration(config);
        switch (status)
        {
        case ansel::kSetConfigurationSuccess:
            Log("Nvidia Ansel is supported and used");
            return true;

        case ansel::kSetConfigurationIncompatibleVersion:
            Log("! Nvidia Ansel: incompatible version");
            break;

        case ansel::kSetConfigurationIncorrectConfiguration:
            Log("! Nvidia Ansel: incorrect configuration");
            break;
        
        case ansel::kSetConfigurationSdkNotLoaded:
            Log("! Nvidia Ansel: SDK wasn't loaded");
            break;
        }
    }
    else
        Log("! Nvidia Ansel:: failed to load AnselSDKxx.dll");
#endif
    return false;
}

void AnselManager::OnFrame()
{
    const float previousFrameTime = timer.GetElapsed_sec();
    timer.Start();
    timeDelta = 0.3f * timeDelta + 0.7f * previousFrameTime;

    clamp(timeDelta, EPS_S, 0.1f);

    Device.fTimeDelta = timeDelta; // fake, to update cam (problem with fov)
    g_pGameLevel->Cameras().UpdateFromCamera(&camera);
    Device.fTimeDelta = 0.0f; // fake, to update cam (problem with fov)
}

AnselCamera::AnselCamera(IGameObject* p, u32 flags) : CCameraBase(p, flags) {}

AnselCameraEffector::AnselCameraEffector()
    : CEffectorCam(cefAnsel, std::numeric_limits<float>::infinity()) {}

BOOL AnselCameraEffector::ProcessCam(SCamEffectorInfo& info)
{
    info.dont_apply = false;

    static ansel::Camera camera;
    static nv::Vec3 right = { info.r.x, info.r.y, info.r.z };
    static nv::Vec3 up = { info.n.x, info.n.y, info.n.z };
    static nv::Vec3 forward = { info.d.x, info.d.y, info.d.z };

    // XXX: Support camera move
    //camera.position = { info.p.x, info.p.y, info.p.z };
    ansel::rotationMatrixVectorsToQuaternion(right, up, forward, camera.rotation);

    camera.fov = info.fFov;
    camera.aspectRatio = info.fAspect;
    camera.nearPlane = info.fNear;
    camera.farPlane = info.fFar;

    camera.projectionOffsetX = info.offsetX;
    camera.projectionOffsetY = info.offsetY;

    ansel::updateCamera(camera);
    ansel::quaternionToRotationMatrixVectors(camera.rotation, right, up, forward);

    info.fFov = camera.fov;
    info.fAspect = camera.aspectRatio;
    info.fNear = camera.nearPlane;
    info.fFar = camera.farPlane;

    info.offsetX = camera.projectionOffsetX;
    info.offsetY = camera.projectionOffsetY;

    // XXX: Support camera move
    //info.p.set(camera.position.x, camera.position.y, camera.position.z);
    info.d.set(forward.x, forward.y, forward.z);
    info.n.set(up.x, up.y, up.z);
    info.r.set(right.x, right.y, right.z);

    return TRUE;
}
