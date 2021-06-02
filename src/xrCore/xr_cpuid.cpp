#include "stdafx.h"
#pragma hdrstop

#include "xr_cpuid.h"

#if (defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)) && defined(XR_COMPILER_GCC) && (defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64))
#include <cpuid.h>
#endif

#include <array>
#include <bitset>
#include <memory>

#include <SDL_cpuinfo.h>

#ifdef _EDITOR
unsgined int query_processor_info(processor_info* pinfo)
{
    ZeroMemory(pinfo, sizeof(processor_info));

    pinfo->feature = static_cast<u32>(CpuFeature::Mmx) | static_cast<u32>(CpuFeature::Sse);
    return pinfo->feature;
}
#else

#undef _CPUID_DEBUG

void nativeCpuId(int regs[4], int i)
{
#ifdef XR_PLATFORM_WINDOWS
    __cpuid((int *)regs, (int)i);
#elif (defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)) && defined(XR_COMPILER_GCC)
#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64)
	__cpuid((int)i, regs[0], regs[1], regs[2], regs[3]);
#elif defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64)
	// XXX: add arm-specific code
#elif defined(XR_ARCHITECTURE_E2K)
	// XXX: add e2k-specific code
#else
    asm volatile("cpuid" :
    "=eax" (regs[0]),
    "=ebx" (regs[1]),
    "=ecx" (regs[2]),
    "=edx" (regs[3])
    : "eax" (i));
#endif
#else
#error Cpuid is not implemented
#endif
}

#ifndef XR_PLATFORM_WINDOWS
#include <thread>

void xr_cpuidex(int regs[4], int i, int j)
{
    nativeCpuId(regs, i);
}
#endif

#ifdef XR_PLATFORM_WINDOWS
u32 countSetBits(ULONG_PTR bitMask)
{
    u32 LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    u32 bitSetCount = 0;
    ULONG_PTR bitTest = static_cast<ULONG_PTR>(1) << LSHIFT;
    u32 i;

    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}
#endif

void fillInAvailableCpus(processor_info* pinfo)
{
    // Calculate available processors
#ifdef XR_PLATFORM_WINDOWS
    ULONG_PTR pa_mask_save, sa_mask_stub = 0;
    GetProcessAffinityMask(GetCurrentProcess(), &pa_mask_save, &sa_mask_stub);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    u32 pa_mask_save = 0;
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &my_set);
    pa_mask_save = CPU_COUNT(&my_set);
#else
#pragma TODO("No function to obtain process affinity")
    u32 pa_mask_save = 0;
#endif // XR_PLATFORM_WINDOWS

    u32 processorCoreCount = 0;
    u32 logicalProcessorCount = 0;

#ifdef XR_PLATFORM_WINDOWS
    DWORD returnedLength = 0;
    GetLogicalProcessorInformation(nullptr, &returnedLength);

    auto* buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)xr_alloca(returnedLength);
    GetLogicalProcessorInformation(buffer, &returnedLength);

    u32 byteOffset = 0;
    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnedLength)
    {
        switch (buffer->Relationship)
        {
        case RelationProcessorCore:
            processorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            logicalProcessorCount += countSetBits(buffer->ProcessorMask);
            break;

        default:
            break;
        }

        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        buffer++;
    }
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    processorCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
    logicalProcessorCount = std::thread::hardware_concurrency();
#else
#pragma TODO("No function to obtain processor's core count")
    logicalProcessorCount = std::thread::hardware_concurrency();
    processorCoreCount = logicalProcessorCount;
#endif

    if (logicalProcessorCount != processorCoreCount)
        pinfo->features.set(static_cast<u32>(CpuFeature::HyperThreading), true);

    // All logical processors
    pinfo->n_threads = logicalProcessorCount;
    pinfo->affinity_mask = pa_mask_save;
    pinfo->n_cores = processorCoreCount;
}


#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64)
bool query_processor_info(processor_info* pinfo)
{
    ZeroMemory(pinfo, sizeof(processor_info));

    std::bitset<32> f_1_ECX;
    std::bitset<32> f_1_EDX;
    /*std::bitset<32> f_7_EBX;
    std::bitset<32> f_7_ECX;
    std::bitset<32> f_81_ECX;*/
    std::bitset<32> f_81_EDX;
    //std::bitset<32> f_87_ECX;
    std::bitset<32> f_87_EDX;

    xr_vector<std::array<int, 4>> data;
    std::array<int, 4> cpui;

    nativeCpuId(cpui.data(), 0);
    const int nIds = cpui[0];

    for (int i = 0; i <= nIds; ++i)
    {
#ifdef XR_PLATFORM_WINDOWS
        __cpuidex(cpui.data(), i, 0);
#else
        xr_cpuidex(cpui.data(), i, 0);
#endif
        data.push_back(cpui);
    }

    memset(pinfo->vendor, 0, sizeof(pinfo->vendor));
    *reinterpret_cast<int*>(pinfo->vendor) = data[0][1];
    *reinterpret_cast<int*>(pinfo->vendor + 4) = data[0][3];
    *reinterpret_cast<int*>(pinfo->vendor + 8) = data[0][2];

    //const bool isIntel = std::strncmp(pinfo->vendor, "GenuineIntel", 12);
    const bool isAmd = strncmp(pinfo->vendor, "AuthenticAMD", 12) != 0;

    // load bitset with flags for function 0x00000001
    if (nIds >= 1)
    {
        f_1_ECX = data[1][2];
        f_1_EDX = data[1][3];
    }

    // load bitset with flags for function 0x00000007
    /*if (nIds >= 7)
    {
    f_7_EBX = data[7][1];
    f_7_ECX = data[7][2];
    }*/

    nativeCpuId(cpui.data(), 0x80000000);
    const int nExIds_ = cpui[0];
    data.clear();

    for (int i = 0x80000000; i <= nExIds_; ++i)
    {
#ifdef XR_PLATFORM_WINDOWS
        __cpuidex(cpui.data(), i, 0);
#else
        xr_cpuidex(cpui.data(), i, 0);
#endif
        data.push_back(cpui);
    }

    // load bitset with flags for function 0x80000001
    if (nExIds_ >= 0x80000001)
    {
        //f_81_ECX = data[1][2];
        f_81_EDX = data[1][3];
    }

    memset(pinfo->modelName, 0, sizeof(pinfo->modelName));

    // Interpret CPU brand string if reported
    if (nExIds_ >= 0x80000004)
    {
        memcpy(pinfo->modelName, data[2].data(), sizeof(cpui));
        memcpy(pinfo->modelName + 16, data[3].data(), sizeof(cpui));
        memcpy(pinfo->modelName + 32, data[4].data(), sizeof(cpui));
    }

    // Read invariant TSC support
    if (nExIds_ >= 0x80000007)
    {
        //f_87_ECX = data[7][2];
        f_87_EDX = data[7][3];
    }

    if (f_87_EDX[8])
        pinfo->features.set(static_cast<u32>(CpuFeature::InvariantTSC), true);

    if (f_1_EDX[23])
        pinfo->features.set(static_cast<u32>(CpuFeature::MMX), true);
    if (isAmd && f_81_EDX[31])
        pinfo->features.set(static_cast<u32>(CpuFeature::_3DNow), true);

    pinfo->features.set(static_cast<u32>(CpuFeature::AltiVec), SDL_HasAltiVec()); // XXX: replace

    if (f_1_EDX[25])
        pinfo->features.set(static_cast<u32>(CpuFeature::SSE), true);
    if (f_1_EDX[26])
        pinfo->features.set(static_cast<u32>(CpuFeature::SSE2), true);
    if (f_1_ECX[0])
        pinfo->features.set(static_cast<u32>(CpuFeature::SSE3), true);
    if (f_1_ECX[9])
        pinfo->features.set(static_cast<u32>(CpuFeature::SSSE3), true);
    if (f_1_ECX[19])
        pinfo->features.set(static_cast<u32>(CpuFeature::SSE41), true);
    if (f_1_ECX[20])
        pinfo->features.set(static_cast<u32>(CpuFeature::SSE42), true);

    pinfo->features.set(static_cast<u32>(CpuFeature::AVX), SDL_HasAVX()); // XXX: replace
    pinfo->features.set(static_cast<u32>(CpuFeature::AVX2), SDL_HasAVX2()); // XXX: replace

    nativeCpuId(cpui.data(), 1);

    const bool hasMWait = (cpui[2] & 0x8) > 0;
    if (hasMWait)
        pinfo->features.set(static_cast<u32>(CpuFeature::MWait), true);

    pinfo->family = (cpui[0] >> 8) & 0xf;
    pinfo->model = (cpui[0] >> 4) & 0xf;
    pinfo->stepping = cpui[0] & 0xf;

    fillInAvailableCpus(pinfo);

    return pinfo->features.get() != 0;
}

#elif defined(XR_ARCHITECTURE_E2K)
bool query_processor_info(processor_info* pinfo)
{
    *pinfo = {};

    strcpy(pinfo->vendor, "MCST");
    xr_sprintf(pinfo->modelName, "%s (%s)", __builtin_cpu_name(), __builtin_cpu_arch());

#if defined(__MMX__)
    pinfo->features.set(static_cast<u32>(CpuFeature::MMX), true);
#endif

#if defined(__3dNOW__)
    pinfo->features.set(static_cast<u32>(CpuFeature::_3DNow), true);
#endif

#if defined(__SSE__)
    pinfo->features.set(static_cast<u32>(CpuFeature::SSE), true);
#endif

#if defined(__SSE2__)
    pinfo->features.set(static_cast<u32>(CpuFeature::SSE2), true);
#endif

#if defined(__SSE3__)
    pinfo->features.set(static_cast<u32>(CpuFeature::SSE3), true);
#endif

#if defined(__SSSE3__)
    pinfo->features.set(static_cast<u32>(CpuFeature::SSSE3), true);
#endif

#if defined(__SSE4_1__)
    pinfo->features.set(static_cast<u32>(CpuFeature::SSE41), true);
#endif

#if defined(__SSE4_2__)
    pinfo->features.set(static_cast<u32>(CpuFeature::SSE42), true);
#endif

#if defined(__AVX__)
    pinfo->features.set(static_cast<u32>(CpuFeature::AVX), true);
#endif

#if defined(__AVX2__)
    pinfo->features.set(static_cast<u32>(CpuFeature::AVX2), true);
#endif

    fillInAvailableCpus(pinfo);

    return true;
}

#elif defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64)
bool query_processor_info(processor_info* pinfo)
{
    *pinfo = {};

    fillInAvailableCpus(pinfo);

    return true;
}

#endif

#endif
