#include "stdafx.h"
#include "StreamlineDLSS.h"
#include "Layers/xrRender/Backend/VulkanBackend.h"

#if defined(XRAY_USE_DLSS)

#include <nvrhi/vulkan.h>
#include <vulkan/vulkan.h>
#include <wchar.h>
#include <string.h>
#include <string>
#include <vector>
#if defined(XR_PLATFORM_LINUX)
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#endif

#include "nvsdk_ngx.h"
#include "nvsdk_ngx_defs_dlssg.h"
#include "nvsdk_ngx_helpers.h"
#include "nvsdk_ngx_helpers_vk.h"
#include "nvsdk_ngx_helpers_dlssd_vk.h"
#include "nvsdk_ngx_helpers_dlssg_vk.h"

extern ENGINE_API int ps_r_dlss_quality;
extern ENGINE_API int ps_r_dlss_fg;
extern ENGINE_API int ps_r_dlss_rr;
extern ENGINE_API int ps_r_dlss_auto_exposure;

namespace xray::render::fg {
namespace {

constexpr const char* kNgxProjectId = "a0f57b54-1daf-4934-90ae-c4035c19df04";
constexpr unsigned long long kNgxApplicationId = 231313132ull;

bool g_slPreInit = false;
bool g_ngxReady = false;
bool g_dlssAvailable = false;
bool g_rrAvailable = false;
bool g_fgAvailable = false;
NVSDK_NGX_Parameter* g_params = nullptr;
NVSDK_NGX_Handle* g_dlssHandle = nullptr;
NVSDK_NGX_Handle* g_rrHandle = nullptr;
NVSDK_NGX_Handle* g_fgHandle = nullptr;
u32 g_featW = 0, g_featH = 0, g_outW = 0, g_outH = 0;
int g_featQuality = -1;
int g_featCreateFlags = -1;
bool g_featRR = false;
bool g_featNeedsSubmit = false;
bool g_featRecreated = false;
bool g_featForceSR = false;
u32 g_fgW = 0, g_fgH = 0, g_fgRenderW = 0, g_fgRenderH = 0;
u32 g_fgFormat = 0;
nvrhi::TextureHandle g_fgInterp;
nvrhi::TextureHandle g_fgReal;
bool g_fgPresentPending = false;
std::wstring g_appDataPathW;
std::vector<std::wstring> g_searchPathsW;
std::vector<const wchar_t*> g_searchPathPtrs;
NVSDK_NGX_FeatureCommonInfo g_featureInfo{};

std::wstring Utf8ToWide(const xr_string& path)
{
#if defined(XR_PLATFORM_WINDOWS)
    if (path.empty())
        return L".";
    int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring out(n > 0 ? n - 1 : 0, L'\0');
    if (n > 1)
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, out.data(), n);
    return out.empty() ? L"." : out;
#else
    std::wstring out;
    out.reserve(path.size());
    for (unsigned char c : path)
        out.push_back((wchar_t)c);
    return out.empty() ? L"." : out;
#endif
}

xr_string NormalizeFsPath(xr_string path)
{
    for (char& c : path)
    {
        if (c == '\\')
            c = '/';
    }
    while (!path.empty() && (path.back() == '/' || path.back() == '\\'))
        path.pop_back();
    return path;
}

xr_string ResolveNgxAppDataPath()
{
    xr_string path;
    if (auto* p = FS.get_path("$app_data_root$"))
        path = NormalizeFsPath(p->m_Path);
    if (path.empty())
    {
#if defined(XR_PLATFORM_LINUX)
        char cwd[PATH_MAX] = {};
        if (getcwd(cwd, sizeof(cwd)))
            path = cwd;
#endif
    }
    if (path.empty() && Core.ApplicationPath[0])
        path = NormalizeFsPath(Core.ApplicationPath);
    if (path.empty())
        path = ".";
    path += "/ngx";
#if defined(XR_PLATFORM_LINUX)
    mkdir(path.c_str(), 0755);
#endif
    return path;
}

void CollectDllSearchPaths()
{
    g_searchPathsW.clear();
    g_searchPathPtrs.clear();
    auto addPath = [&](const xr_string& p) {
        xr_string n = NormalizeFsPath(p);
        if (n.empty())
            return;
        std::wstring w = Utf8ToWide(n);
        for (const auto& existing : g_searchPathsW)
            if (existing == w)
                return;
        g_searchPathsW.push_back(std::move(w));
    };

#if defined(XR_PLATFORM_LINUX)
    char exePath[PATH_MAX] = {};
    const ssize_t n = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (n > 0)
    {
        exePath[n] = 0;
        char* slash = strrchr(exePath, '/');
        if (slash)
        {
            *slash = 0;
            addPath(xr_string(exePath));
        }
    }
    char cwd[PATH_MAX] = {};
    if (getcwd(cwd, sizeof(cwd)))
        addPath(xr_string(cwd));
#endif
    if (Core.ApplicationPath[0])
        addPath(xr_string(Core.ApplicationPath));

    g_searchPathPtrs.reserve(g_searchPathsW.size());
    for (const auto& w : g_searchPathsW)
        g_searchPathPtrs.push_back(w.c_str());

    memset(&g_featureInfo, 0, sizeof(g_featureInfo));
    g_featureInfo.PathListInfo.Path = g_searchPathPtrs.data();
    g_featureInfo.PathListInfo.Length = (unsigned int)g_searchPathPtrs.size();
}

NVSDK_NGX_PerfQuality_Value MapQuality(int q)
{
    switch (q)
    {
    case 0: return NVSDK_NGX_PerfQuality_Value_UltraPerformance;
    case 1: return NVSDK_NGX_PerfQuality_Value_MaxPerf;
    case 2: return NVSDK_NGX_PerfQuality_Value_Balanced;
    case 3: return NVSDK_NGX_PerfQuality_Value_MaxQuality;
    case 5: return NVSDK_NGX_PerfQuality_Value_DLAA;
    default: return NVSDK_NGX_PerfQuality_Value_MaxQuality;
    }
}

bool MakeResourceVK(nvrhi::ITexture* tex, bool readWrite, NVSDK_NGX_Resource_VK& out)
{
    if (!tex)
        return false;
    const auto img = tex->getNativeObject(nvrhi::ObjectTypes::VK_Image);
    const auto view = tex->getNativeView(nvrhi::ObjectTypes::VK_ImageView);
    if (!img.integer || !view.integer)
        return false;
    const nvrhi::Format fmt = tex->getDesc().format;
    VkImageSubresourceRange range{};
    range.aspectMask = (fmt == nvrhi::Format::D32 ||
        fmt == nvrhi::Format::D16 ||
        fmt == nvrhi::Format::D24S8 ||
        fmt == nvrhi::Format::D32S8)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    range.levelCount = 1;
    range.layerCount = 1;
    out = NVSDK_NGX_Create_ImageView_Resource_VK(
        (VkImageView)view.integer,
        (VkImage)img.integer,
        range,
        (VkFormat)nvrhi::vulkan::convertFormat(tex->getDesc().format),
        tex->getDesc().width,
        tex->getDesc().height,
        readWrite);
    return true;
}

void WaitGpuForNgx()
{
    if (GEnv.Backend)
        GEnv.Backend->WaitForIdle();
}

void DestroyFgFeature()
{
    const bool hadFg = g_fgHandle != nullptr;
    if (hadFg && g_params)
    {
        WaitGpuForNgx();
        NVSDK_NGX_VULKAN_ReleaseFeature(g_fgHandle);
        g_fgHandle = nullptr;
    }
    g_fgInterp = nullptr;
    g_fgReal = nullptr;
    g_fgPresentPending = false;
    g_fgW = g_fgH = g_fgRenderW = g_fgRenderH = 0;
    g_fgFormat = 0;
}

void DestroyFeatures()
{
    const bool hadFeat = g_dlssHandle || g_rrHandle;
    if (hadFeat && g_params)
    {
        WaitGpuForNgx();
        if (g_dlssHandle)
        {
            NVSDK_NGX_VULKAN_ReleaseFeature(g_dlssHandle);
            g_dlssHandle = nullptr;
        }
        if (g_rrHandle)
        {
            NVSDK_NGX_VULKAN_ReleaseFeature(g_rrHandle);
            g_rrHandle = nullptr;
        }
    }
    g_featW = g_featH = g_outW = g_outH = 0;
    g_featQuality = -1;
    g_featCreateFlags = -1;
    g_featRR = false;
    g_featNeedsSubmit = false;
}

void CopyTo44(float dst[4][4], const float src[16])
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            dst[r][c] = src[r * 4 + c];
}

bool EnsureFeature(nvrhi::ICommandList* cmd, const UpscaleInputs& inputs, bool wantRR)
{
    if (!g_ngxReady || !g_params || !cmd)
        return false;
    int createFlags =
        NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
        NVSDK_NGX_DLSS_Feature_Flags_DepthInverted;
    if (inputs.renderWidth < inputs.displayWidth || inputs.renderHeight < inputs.displayHeight)
        createFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;
    if (ps_r_dlss_auto_exposure)
        createFlags |= NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;

    if (g_featW == inputs.renderWidth && g_featH == inputs.renderHeight &&
        g_outW == inputs.displayWidth && g_outH == inputs.displayHeight &&
        g_featQuality == ps_r_dlss_quality && g_featRR == wantRR &&
        g_featCreateFlags == createFlags &&
        (wantRR ? g_rrHandle : g_dlssHandle))
        return true;

    DestroyFeatures();

    auto* vkCmd = (VkCommandBuffer)cmd->getNativeObject(nvrhi::ObjectTypes::VK_CommandBuffer).integer;
    auto* backend = dynamic_cast<VulkanBackend*>(GEnv.Backend);
    if (!vkCmd || !backend)
        return false;

    NVSDK_NGX_DLSS_Create_Params create{};
    create.Feature.InWidth = inputs.renderWidth;
    create.Feature.InHeight = inputs.renderHeight;
    create.Feature.InTargetWidth = inputs.displayWidth;
    create.Feature.InTargetHeight = inputs.displayHeight;
    create.Feature.InPerfQualityValue = MapQuality(ps_r_dlss_quality);
    create.InFeatureCreateFlags = createFlags;

    NVSDK_NGX_Result res;
    if (wantRR && g_rrAvailable)
    {
        NVSDK_NGX_DLSSD_Create_Params rrCreate{};
        rrCreate.InDenoiseMode = NVSDK_NGX_DLSS_Denoise_Mode_DLUnified;
        rrCreate.InRoughnessMode = NVSDK_NGX_DLSS_Roughness_Mode_Packed;
        rrCreate.InUseHWDepth = NVSDK_NGX_DLSS_Depth_Type_HW;
        rrCreate.InWidth = inputs.renderWidth;
        rrCreate.InHeight = inputs.renderHeight;
        rrCreate.InTargetWidth = inputs.displayWidth;
        rrCreate.InTargetHeight = inputs.displayHeight;
        rrCreate.InPerfQualityValue = MapQuality(ps_r_dlss_quality);
        rrCreate.InFeatureCreateFlags = create.InFeatureCreateFlags;
        res = NGX_VULKAN_CREATE_DLSSD_EXT1(
            backend->GetVkDevice(), vkCmd, 1, 1, &g_rrHandle, g_params, &rrCreate);
        if (NVSDK_NGX_FAILED(res))
        {
            Msg("! [Upscale] DLSS-RR CreateFeature failed 0x%08x — falling back to DLSS SR", (u32)res);
            wantRR = false;
        }
    }

    if (!wantRR)
    {
        res = NGX_VULKAN_CREATE_DLSS_EXT1(
            backend->GetVkDevice(), vkCmd, 1, 1, &g_dlssHandle, g_params, &create);
        if (NVSDK_NGX_FAILED(res))
        {
            Msg("! [Upscale] DLSS CreateFeature failed 0x%08x", (u32)res);
            return false;
        }
    }

    g_featW = inputs.renderWidth;
    g_featH = inputs.renderHeight;
    g_outW = inputs.displayWidth;
    g_outH = inputs.displayHeight;
    g_featQuality = ps_r_dlss_quality;
    g_featCreateFlags = createFlags;
    g_featRR = wantRR && g_rrHandle != nullptr;
    g_featNeedsSubmit = true;
    g_featRecreated = true;
    return true;
}

}

bool Streamline_PreInstanceInit()
{
    g_slPreInit = true;
    return true;
}

void Streamline_GetRequiredInstanceExtensions(xr_vector<const char*>& outExts)
{
    unsigned int instCount = 0, devCount = 0;
    const char** instExts = nullptr;
    const char** devExts = nullptr;
    if (NVSDK_NGX_SUCCEED(NVSDK_NGX_VULKAN_RequiredExtensions(&instCount, &instExts, &devCount, &devExts)))
    {
        for (unsigned int i = 0; i < instCount; ++i)
            outExts.push_back(instExts[i]);
        (void)devExts;
        (void)devCount;
    }
}

void Streamline_GetRequiredDeviceExtensions(void* /*physicalDevice*/, xr_vector<const char*>& outExts)
{
    unsigned int instCount = 0, devCount = 0;
    const char** instExts = nullptr;
    const char** devExts = nullptr;
    if (NVSDK_NGX_SUCCEED(NVSDK_NGX_VULKAN_RequiredExtensions(&instCount, &instExts, &devCount, &devExts)))
    {
        for (unsigned int i = 0; i < devCount; ++i)
            outExts.push_back(devExts[i]);
        (void)instExts;
        (void)instCount;
    }
}

bool Streamline_SetVulkanInfo(const StreamlineVulkanInfo& info)
{
    if (!info.instance || !info.physicalDevice || !info.device)
        return false;
    if (g_ngxReady)
        return g_dlssAvailable;

    const xr_string appData = ResolveNgxAppDataPath();
    g_appDataPathW = Utf8ToWide(appData);
    CollectDllSearchPaths();
    Msg("* [Upscale] NGX init appData=%s dllPaths=%u", appData.c_str(), (u32)g_searchPathPtrs.size());
    FlushLog();

    Msg("* [Upscale] NGX calling VULKAN_Init (ApplicationId)...");
    FlushLog();
    NVSDK_NGX_Result res = NVSDK_NGX_VULKAN_Init(
        kNgxApplicationId,
        g_appDataPathW.c_str(),
        (VkInstance)info.instance,
        (VkPhysicalDevice)info.physicalDevice,
        (VkDevice)info.device,
        vkGetInstanceProcAddr,
        vkGetDeviceProcAddr,
        g_searchPathPtrs.empty() ? nullptr : &g_featureInfo,
        NVSDK_NGX_Version_API);

    if (NVSDK_NGX_FAILED(res))
    {
        Msg("! [Upscale] NGX ApplicationId Init failed 0x%08x — trying ProjectID", (u32)res);
        FlushLog();
        res = NVSDK_NGX_VULKAN_Init_with_ProjectID(
            kNgxProjectId,
            NVSDK_NGX_ENGINE_TYPE_CUSTOM,
            "1.6.0",
            g_appDataPathW.c_str(),
            (VkInstance)info.instance,
            (VkPhysicalDevice)info.physicalDevice,
            (VkDevice)info.device,
            vkGetInstanceProcAddr,
            vkGetDeviceProcAddr,
            g_searchPathPtrs.empty() ? nullptr : &g_featureInfo,
            NVSDK_NGX_Version_API);
    }

    if (NVSDK_NGX_FAILED(res))
    {
        Msg("! [Upscale] NGX Vulkan Init failed 0x%08x", (u32)res);
        FlushLog();
        return false;
    }
    Msg("* [Upscale] NGX Init OK (0x%08x)", (u32)res);
    FlushLog();

    res = NVSDK_NGX_VULKAN_GetCapabilityParameters(&g_params);
    if (NVSDK_NGX_FAILED(res) || !g_params)
    {
        Msg("! [Upscale] NGX GetCapabilityParameters failed");
        NVSDK_NGX_VULKAN_Shutdown1((VkDevice)info.device);
        return false;
    }

    int needsUpdatedDriver = 0;
    unsigned int minDriver = 0;
    NVSDK_NGX_Parameter_GetI(g_params, NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);
    NVSDK_NGX_Parameter_GetUI(g_params, NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minDriver);
    int dlssSupported = 0;
    NVSDK_NGX_Parameter_GetI(g_params, NVSDK_NGX_Parameter_SuperSampling_Available, &dlssSupported);
    g_dlssAvailable = dlssSupported != 0;

    int rrSupported = 0;
    NVSDK_NGX_Parameter_GetI(g_params, NVSDK_NGX_Parameter_SuperSamplingDenoising_Available, &rrSupported);
    g_rrAvailable = rrSupported != 0;

    int fgSupported = 0;
    NVSDK_NGX_Parameter_GetI(g_params, NVSDK_NGX_Parameter_FrameGeneration_Available, &fgSupported);
    if (!fgSupported)
        NVSDK_NGX_Parameter_GetI(g_params, NVSDK_NGX_Parameter_FrameInterpolation_Available, &fgSupported);
    g_fgAvailable = fgSupported != 0;
    if (!g_fgAvailable)
    {
        int fgInit = 0;
        unsigned int fgMinMaj = 0, fgMinMin = 0;
        int fgNeedsDriver = 0;
        NVSDK_NGX_Parameter_GetI(g_params, NVSDK_NGX_Parameter_FrameGeneration_FeatureInitResult, &fgInit);
        NVSDK_NGX_Parameter_GetUI(g_params, NVSDK_NGX_Parameter_FrameGeneration_MinDriverVersionMajor, &fgMinMaj);
        NVSDK_NGX_Parameter_GetUI(g_params, NVSDK_NGX_Parameter_FrameGeneration_MinDriverVersionMinor, &fgMinMin);
        NVSDK_NGX_Parameter_GetI(g_params, NVSDK_NGX_Parameter_FrameGeneration_NeedsUpdatedDriver, &fgNeedsDriver);
        Msg("! [Upscale] DLSS-FG unavailable initResult=0x%08x needsDriver=%d minDriver=%u.%u (RTX 40+ required)",
            (u32)fgInit, fgNeedsDriver, fgMinMaj, fgMinMin);
    }

    g_ngxReady = true;
    Msg("* [Upscale] NGX ready dlss=%d rr=%d fg=%d needsDriver=%d minMajor=%u",
        g_dlssAvailable ? 1 : 0, g_rrAvailable ? 1 : 0, g_fgAvailable ? 1 : 0,
        needsUpdatedDriver, minDriver);
    if (!g_dlssAvailable)
        Msg("! [Upscale] DLSS plugin unavailable — check libnvidia-ngx-dlss.so next to xr_3da / LD_LIBRARY_PATH");
    return g_dlssAvailable;
}

void Streamline_Shutdown()
{
    DestroyFgFeature();
    DestroyFeatures();
    if (g_params || g_ngxReady)
        WaitGpuForNgx();
    if (g_params)
    {
        NVSDK_NGX_VULKAN_DestroyParameters(g_params);
        g_params = nullptr;
    }
    if (g_ngxReady)
    {
        auto* backend = dynamic_cast<VulkanBackend*>(GEnv.Backend);
        if (backend)
            NVSDK_NGX_VULKAN_Shutdown1(backend->GetVkDevice());
    }
    g_ngxReady = false;
    g_dlssAvailable = g_rrAvailable = g_fgAvailable = false;
    g_featRecreated = false;
    g_featForceSR = false;
}

bool Streamline_IsDLSSAvailable() { return g_dlssAvailable; }
bool Streamline_IsFGAvailable() { return g_fgAvailable; }
bool Streamline_IsRRAvailable() { return g_rrAvailable; }

bool Streamline_ConsumeFeatureReset()
{
    const bool r = g_featRecreated;
    g_featRecreated = false;
    return r;
}

void Streamline_ReleaseFeatures()
{
    DestroyFgFeature();
    DestroyFeatures();
    g_featForceSR = false;
}

bool EvaluateDLSSSR(VkCommandBuffer vkCmd, const UpscaleInputs& inputs,
    NVSDK_NGX_Resource_VK& color, NVSDK_NGX_Resource_VK& depth, NVSDK_NGX_Resource_VK& mv,
    NVSDK_NGX_Resource_VK& out, NVSDK_NGX_Resource_VK* exposure)
{
    if (!g_dlssHandle)
        return false;
    NVSDK_NGX_VK_DLSS_Eval_Params eval{};
    eval.Feature.pInColor = &color;
    eval.Feature.pInOutput = &out;
    eval.Feature.InSharpness = inputs.sharpness;
    eval.pInDepth = &depth;
    eval.pInMotionVectors = &mv;
    eval.InJitterOffsetX = inputs.jitterX;
    eval.InJitterOffsetY = inputs.jitterY;
    eval.InRenderSubrectDimensions = { inputs.renderWidth, inputs.renderHeight };
    eval.InReset = inputs.reset ? 1 : 0;
    eval.InMVScaleX = (float)inputs.renderWidth;
    eval.InMVScaleY = (float)inputs.renderHeight;
    if (exposure)
        eval.pInExposureTexture = exposure;
    const NVSDK_NGX_Result res = NGX_VULKAN_EVALUATE_DLSS_EXT(vkCmd, g_dlssHandle, g_params, &eval);
    if (NVSDK_NGX_FAILED(res))
    {
        Msg("! [Upscale] DLSS SR Evaluate failed 0x%08x", (u32)res);
        return false;
    }
    return true;
}

bool Streamline_EvaluateDLSS(nvrhi::ICommandList* cmd, const UpscaleInputs& inputs)
{
    if (!g_ngxReady || !cmd || !inputs.color || !inputs.output || !inputs.depth || !inputs.motionVectors)
        return false;

    bool wantRR = !g_featForceSR && inputs.enableRR && g_rrAvailable &&
        inputs.normals && inputs.diffuseAlbedo && inputs.specularAlbedo;
    if (!EnsureFeature(cmd, inputs, wantRR))
        return false;
    UpscaleInputs evalIn = inputs;
    if (g_featNeedsSubmit)
    {
        g_featNeedsSubmit = false;
        evalIn.reset = true;
    }
    wantRR = g_featRR && wantRR;

    NVSDK_NGX_Resource_VK color{}, depth{}, mv{}, out{}, exposure{};
    if (!MakeResourceVK(evalIn.color, false, color) ||
        !MakeResourceVK(evalIn.depth, false, depth) ||
        !MakeResourceVK(evalIn.motionVectors, false, mv) ||
        !MakeResourceVK(evalIn.output, true, out))
        return false;
    const bool hasExposure = evalIn.exposure && MakeResourceVK(evalIn.exposure, false, exposure);

    auto* vkCmd = (VkCommandBuffer)cmd->getNativeObject(nvrhi::ObjectTypes::VK_CommandBuffer).integer;
    if (!vkCmd)
        return false;

    bool ok = false;
    if (g_featRR && g_rrHandle && wantRR)
    {
        NVSDK_NGX_Resource_VK normals{}, diffAlb{}, specAlb{}, diffHit{}, specHit{};
        if (!MakeResourceVK(evalIn.normals, false, normals) ||
            !MakeResourceVK(evalIn.diffuseAlbedo, false, diffAlb) ||
            !MakeResourceVK(evalIn.specularAlbedo, false, specAlb))
        {
            Msg("! [Upscale] DLSS-RR missing G-buffer — falling back to SR");
        }
        else
        {
            const bool hasDiffHit = evalIn.diffuseHitDistance && MakeResourceVK(evalIn.diffuseHitDistance, false, diffHit);
            const bool hasSpecHit = evalIn.specularHitDistance && MakeResourceVK(evalIn.specularHitDistance, false, specHit);

            NVSDK_NGX_VK_DLSSD_Eval_Params eval{};
            eval.pInColor = &color;
            eval.pInOutput = &out;
            eval.pInDepth = &depth;
            eval.pInMotionVectors = &mv;
            eval.pInDiffuseAlbedo = &diffAlb;
            eval.pInSpecularAlbedo = &specAlb;
            eval.pInNormals = &normals;
            eval.pInRoughness = &normals;
            if (hasDiffHit)
                eval.pInDiffuseHitDistance = &diffHit;
            if (hasSpecHit)
                eval.pInSpecularHitDistance = &specHit;
            eval.pInWorldToViewMatrix = const_cast<float*>(evalIn.worldToView);
            eval.pInViewToClipMatrix = const_cast<float*>(evalIn.viewToClip);
            eval.InJitterOffsetX = evalIn.jitterX;
            eval.InJitterOffsetY = evalIn.jitterY;
            eval.InRenderSubrectDimensions = { evalIn.renderWidth, evalIn.renderHeight };
            eval.InReset = evalIn.reset ? 1 : 0;
            eval.InMVScaleX = (float)evalIn.renderWidth;
            eval.InMVScaleY = (float)evalIn.renderHeight;
            eval.InFrameTimeDeltaInMsec = evalIn.frameTimeMs;
            eval.InPreExposure = 1.f;
            eval.InExposureScale = 1.f;
            if (hasExposure)
                eval.pInExposureTexture = &exposure;

            const NVSDK_NGX_Result res = NGX_VULKAN_EVALUATE_DLSSD_EXT(vkCmd, g_rrHandle, g_params, &eval);
            if (NVSDK_NGX_SUCCEED(res))
            {
                ok = true;
                static bool s_rrOk = false;
                if (!s_rrOk)
                {
                    s_rrOk = true;
                    Msg("* [Upscale] DLSS-RR Evaluate OK");
                }
            }
            else
            {
                Msg("! [Upscale] DLSS-RR Evaluate failed 0x%08x — falling back to SR next frame", (u32)res);
                g_featForceSR = true;
            }
        }
    }

    if (!ok && g_dlssHandle)
        ok = EvaluateDLSSSR(vkCmd, evalIn, color, depth, mv, out, hasExposure ? &exposure : nullptr);

    return ok;
}

bool Streamline_EvaluateDLSSRR(nvrhi::ICommandList* cmd, const UpscaleInputs& inputs)
{
    UpscaleInputs rrInputs = inputs;
    rrInputs.enableRR = true;
    return Streamline_EvaluateDLSS(cmd, rrInputs);
}

bool EnsureFgFeature(nvrhi::ICommandList* cmd, const DlssFgInputs& inputs, u32 vkFormat)
{
    if (!g_ngxReady || !g_params || !g_fgAvailable || !cmd)
        return false;
    if (g_fgHandle && g_fgW == inputs.displayWidth && g_fgH == inputs.displayHeight &&
        g_fgRenderW == inputs.renderWidth && g_fgRenderH == inputs.renderHeight &&
        g_fgFormat == vkFormat && g_fgInterp && g_fgReal)
        return true;

    DestroyFgFeature();

    auto* vkCmd = (VkCommandBuffer)cmd->getNativeObject(nvrhi::ObjectTypes::VK_CommandBuffer).integer;
    if (!vkCmd)
        return false;

    NVSDK_NGX_DLSSG_Create_Params create{};
    create.Width = inputs.displayWidth;
    create.Height = inputs.displayHeight;
    create.NativeBackbufferFormat = vkFormat;
    create.RenderWidth = inputs.renderWidth;
    create.RenderHeight = inputs.renderHeight;
    create.DynamicResolutionScaling = false;

    NVSDK_NGX_Parameter_SetUI(g_params, "Enable.OFA", 1);
    NVSDK_NGX_Parameter_SetUI(g_params, "DLSSG.EnableInterp", 1);

    const NVSDK_NGX_Result res = NGX_VK_CREATE_DLSSG(vkCmd, 1, 1, &g_fgHandle, g_params, &create);
    if (NVSDK_NGX_FAILED(res) || !g_fgHandle)
    {
        Msg("! [Upscale] DLSS-FG CreateFeature failed 0x%08x", (u32)res);
        g_fgHandle = nullptr;
        return false;
    }

    nvrhi::IDevice* nv = cmd->getDevice();
    nvrhi::TextureDesc td;
    td.width = inputs.displayWidth;
    td.height = inputs.displayHeight;
    td.format = inputs.backbuffer->getDesc().format;
    td.isUAV = true;
    td.isShaderResource = true;
    td.isRenderTarget = true;
    td.initialState = nvrhi::ResourceStates::UnorderedAccess;
    td.keepInitialState = true;
    td.debugName = "rt_DlssFg_Interp";
    g_fgInterp = nv->createTexture(td);
    td.debugName = "rt_DlssFg_Real";
    g_fgReal = nv->createTexture(td);
    if (!g_fgInterp || !g_fgReal)
    {
        DestroyFgFeature();
        return false;
    }

    g_fgW = inputs.displayWidth;
    g_fgH = inputs.displayHeight;
    g_fgRenderW = inputs.renderWidth;
    g_fgRenderH = inputs.renderHeight;
    g_fgFormat = vkFormat;
    Msg("* [Upscale] DLSS-FG CreateFeature OK %ux%u (render %ux%u)",
        g_fgW, g_fgH, g_fgRenderW, g_fgRenderH);
    return true;
}

bool Streamline_EvaluateDLSSG(nvrhi::ICommandList* cmd, const DlssFgInputs& inputs)
{
    g_fgPresentPending = false;
    if (!g_ngxReady || !cmd || !inputs.backbuffer || !inputs.depth || !inputs.motionVectors)
        return false;
    if (!g_fgAvailable || !ps_r_dlss_fg)
        return false;

    const u32 vkFormat = (u32)nvrhi::vulkan::convertFormat(inputs.backbuffer->getDesc().format);
    if (!EnsureFgFeature(cmd, inputs, vkFormat))
        return false;

    NVSDK_NGX_Resource_VK color{}, depth{}, mv{}, hudless{}, interp{}, real{};
    if (!MakeResourceVK(inputs.backbuffer, false, color) ||
        !MakeResourceVK(inputs.depth, false, depth) ||
        !MakeResourceVK(inputs.motionVectors, false, mv) ||
        !MakeResourceVK(g_fgInterp.Get(), true, interp) ||
        !MakeResourceVK(g_fgReal.Get(), true, real))
        return false;

    const bool hasHudless = inputs.hudless && MakeResourceVK(inputs.hudless, false, hudless);

    cmd->setTextureState(inputs.backbuffer, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
    cmd->setTextureState(inputs.depth, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
    cmd->setTextureState(inputs.motionVectors, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
    if (hasHudless)
        cmd->setTextureState(inputs.hudless, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
    cmd->setTextureState(g_fgInterp, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
    cmd->setTextureState(g_fgReal, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
    cmd->commitBarriers();

    auto* vkCmd = (VkCommandBuffer)cmd->getNativeObject(nvrhi::ObjectTypes::VK_CommandBuffer).integer;
    if (!vkCmd || !g_fgHandle)
        return false;

    NVSDK_NGX_Parameter_SetUI(g_params, "DLSSG.EnableInterp", 1);
    NVSDK_NGX_Parameter_SetI(g_params, "DLSSG.NumFrames", 1);
    NVSDK_NGX_Parameter_SetUI(g_params, "DLSSG.IsRecording", 1);

    NVSDK_NGX_VK_DLSSG_Eval_Params eval{};
    eval.pBackbuffer = &color;
    eval.pDepth = &depth;
    eval.pMVecs = &mv;
    if (hasHudless)
        eval.pHudless = &hudless;
    eval.pOutputInterpFrame = &interp;
    eval.pOutputRealFrame = &real;

    NVSDK_NGX_DLSSG_Opt_Eval_Params opt{};
    opt.multiFrameCount = 1;
    opt.multiFrameIndex = 1;
    CopyTo44(opt.cameraViewToClip, inputs.viewToClip);
    CopyTo44(opt.clipToCameraView, inputs.clipToView);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            opt.clipToLensClip[i][j] = (i == j) ? 1.f : 0.f;
    CopyTo44(opt.clipToPrevClip, inputs.clipToPrevClip);
    CopyTo44(opt.prevClipToClip, inputs.prevClipToClip);
    opt.jitterOffset[0] = inputs.jitterX;
    opt.jitterOffset[1] = inputs.jitterY;
    opt.mvecScale[0] = (float)inputs.renderWidth;
    opt.mvecScale[1] = (float)inputs.renderHeight;
    opt.cameraPos[0] = inputs.cameraPos[0];
    opt.cameraPos[1] = inputs.cameraPos[1];
    opt.cameraPos[2] = inputs.cameraPos[2];
    opt.cameraUp[0] = inputs.cameraUp[0];
    opt.cameraUp[1] = inputs.cameraUp[1];
    opt.cameraUp[2] = inputs.cameraUp[2];
    opt.cameraRight[0] = inputs.cameraRight[0];
    opt.cameraRight[1] = inputs.cameraRight[1];
    opt.cameraRight[2] = inputs.cameraRight[2];
    opt.cameraFwd[0] = inputs.cameraFwd[0];
    opt.cameraFwd[1] = inputs.cameraFwd[1];
    opt.cameraFwd[2] = inputs.cameraFwd[2];
    opt.cameraNear = inputs.cameraNear;
    opt.cameraFar = inputs.cameraFar;
    opt.cameraFOV = inputs.cameraFOV;
    opt.cameraAspectRatio = inputs.cameraAspect;
    {
        const auto fmt = inputs.backbuffer->getDesc().format;
        opt.colorBuffersHDR =
            fmt == nvrhi::Format::RGBA16_FLOAT || fmt == nvrhi::Format::RGBA32_FLOAT ||
            fmt == nvrhi::Format::RGB32_FLOAT || fmt == nvrhi::Format::RG16_FLOAT;
    }
    opt.depthInverted = true;
    opt.cameraMotionIncluded = true;
    opt.reset = inputs.reset;
    opt.motionVectorsDilated = false;
    opt.mvecsSubrectSize = { inputs.renderWidth, inputs.renderHeight };
    opt.depthSubrectSize = { inputs.renderWidth, inputs.renderHeight };
    opt.backbufferSubrectSize = { inputs.displayWidth, inputs.displayHeight };
    if (hasHudless)
        opt.hudLessSubrectSize = { inputs.displayWidth, inputs.displayHeight };

    const NVSDK_NGX_Result res = NGX_VK_EVALUATE_DLSSG(vkCmd, g_fgHandle, g_params, &eval, &opt);
    cmd->clearState();
    if (NVSDK_NGX_FAILED(res))
    {
        Msg("! [Upscale] DLSS-FG Evaluate failed 0x%08x", (u32)res);
        return false;
    }

    g_fgPresentPending = true;
    static bool s_fgOk = false;
    if (!s_fgOk)
    {
        s_fgOk = true;
        Msg("* [Upscale] DLSS-FG Evaluate OK");
    }
    return true;
}

bool Streamline_TakeFgPresent(nvrhi::ITexture*& outInterp, nvrhi::ITexture*& outReal)
{
    if (!g_fgPresentPending || !g_fgInterp || !g_fgReal)
        return false;
    outInterp = g_fgInterp.Get();
    outReal = g_fgReal.Get();
    g_fgPresentPending = false;
    return true;
}

}

#else

namespace xray::render::fg {

bool Streamline_PreInstanceInit() { return false; }
void Streamline_GetRequiredInstanceExtensions(xr_vector<const char*>&) {}
void Streamline_GetRequiredDeviceExtensions(void*, xr_vector<const char*>&) {}
bool Streamline_SetVulkanInfo(const StreamlineVulkanInfo&) { return false; }
void Streamline_Shutdown() {}
bool Streamline_IsDLSSAvailable() { return false; }
bool Streamline_IsFGAvailable() { return false; }
bool Streamline_IsRRAvailable() { return false; }
bool Streamline_ConsumeFeatureReset() { return false; }
void Streamline_ReleaseFeatures() {}
bool Streamline_EvaluateDLSS(nvrhi::ICommandList*, const UpscaleInputs&) { return false; }
bool Streamline_EvaluateDLSSRR(nvrhi::ICommandList*, const UpscaleInputs&) { return false; }
bool Streamline_EvaluateDLSSG(nvrhi::ICommandList*, const DlssFgInputs&) { return false; }
bool Streamline_TakeFgPresent(nvrhi::ITexture*&, nvrhi::ITexture*&) { return false; }

}

#endif
