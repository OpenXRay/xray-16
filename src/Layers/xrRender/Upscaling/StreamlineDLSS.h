#pragma once

#include <nvrhi/nvrhi.h>
#include "IUpscaleBackend.h"

namespace xray::render::fg {

struct StreamlineVulkanInfo
{
    void* instance = nullptr;
    void* physicalDevice = nullptr;
    void* device = nullptr;
    void* graphicsQueue = nullptr;
    u32 graphicsQueueFamily = 0;
    void* computeQueue = nullptr;
    u32 computeQueueFamily = 0;
    void* opticalFlowQueue = nullptr;
    u32 opticalFlowQueueFamily = 0;
};

struct DlssFgInputs
{
    nvrhi::ITexture* backbuffer = nullptr;
    nvrhi::ITexture* hudless = nullptr;
    nvrhi::ITexture* depth = nullptr;
    nvrhi::ITexture* motionVectors = nullptr;
    u32 displayWidth = 0;
    u32 displayHeight = 0;
    u32 renderWidth = 0;
    u32 renderHeight = 0;
    float jitterX = 0.f;
    float jitterY = 0.f;
    bool reset = false;
    float viewToClip[16]{};
    float clipToView[16]{};
    float clipToPrevClip[16]{};
    float prevClipToClip[16]{};
    float cameraPos[3]{};
    float cameraUp[3]{};
    float cameraRight[3]{};
    float cameraFwd[3]{};
    float cameraNear = 0.2f;
    float cameraFar = 600.f;
    float cameraFOV = 1.f;
    float cameraAspect = 1.f;
};

bool Streamline_PreInstanceInit();
void Streamline_GetRequiredInstanceExtensions(xr_vector<const char*>& outExts);
void Streamline_GetRequiredDeviceExtensions(void* physicalDevice, xr_vector<const char*>& outExts);
bool Streamline_SetVulkanInfo(const StreamlineVulkanInfo& info);
void Streamline_Shutdown();

bool Streamline_IsDLSSAvailable();
bool Streamline_IsFGAvailable();
bool Streamline_IsRRAvailable();
bool Streamline_ConsumeFeatureReset();
void Streamline_ReleaseFeatures();

bool Streamline_EvaluateDLSS(nvrhi::ICommandList* cmd, const UpscaleInputs& inputs);
bool Streamline_EvaluateDLSSRR(nvrhi::ICommandList* cmd, const UpscaleInputs& inputs);
bool Streamline_EvaluateDLSSG(nvrhi::ICommandList* cmd, const DlssFgInputs& inputs);
bool Streamline_TakeFgPresent(nvrhi::ITexture*& outInterp, nvrhi::ITexture*& outReal);
bool Streamline_FgEvaluatedLastFrame();
bool Streamline_FgPresentedLastFrame();
void Streamline_NotifyFgPresented(bool ok);

}
