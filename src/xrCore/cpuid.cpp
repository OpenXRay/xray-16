#include "stdafx.h"
#pragma hdrstop

#include "cpuid.h"
#include <array>
#include <bitset>
#include <memory>

#ifdef _EDITOR
unsgined int query_processor_info(processor_info* pinfo)
{
    memset(pinfo, 0, sizeof(processor_info));

    pinfo->feature = static_cast<u32>(CpuFeature::FEATURE_MMX) | static_cast<u32>(CpuFeature::FEATURE_SSE);
    return pinfo->feature;
}
#else // !_EDITOR
void nativeCpuId(unsigned int regs[4], unsigned int i)
{
#ifdef WINDOWS
    __cpuid((int *)regs, (int)i);
#elif defined(LINUX) && defined(GCC)
    __cpuid ((int)i, (int *)regs);
#elif defined(LINUX)
    asm volatile("cpuid" :
    "=eax" (regs[0]),
    "=ebx" (regs[1]),
    "=ecx" (regs[2]),
    "=edx" (regs[3])
    : "eax" (i));
#else
#error Cpuid is not implemented
#endif
}

unsigned int query_processor_info(processor_info* pinfo)
{
    memset(pinfo, 0, sizeof(processor_info));

    std::bitset<32> f_1_ECX;
    std::bitset<32> f_1_EDX;
    /*std::bitset<32> f_7_EBX;
    std::bitset<32> f_7_ECX;
    std::bitset<32> f_81_ECX;*/
    std::bitset<32> f_81_EDX;

    std::vector<std::array<int, 4>> data;
    std::array<int, 4> cpui;

    nativeCpuId(cpui.data(), 0);
    const int nIds = cpui[0];

    for (int i = 0; i <= nIds; ++i)
    {
        nativeCpuId(cpui.data(), i);
        data.push_back(cpui);
    }

    memset(pinfo->vendor, 0, sizeof(pinfo->vendor));
    *reinterpret_cast<int*>(pinfo->vendor) = data[0][1];
    *reinterpret_cast<int*>(pinfo->vendor + 4) = data[0][3];
    *reinterpret_cast<int*>(pinfo->vendor + 8) = data[0][2];

    //const bool isIntel = std::strncmp(pinfo->vendor, "GenuineIntel", 12);
    const bool isAmd = std::strncmp(pinfo->vendor, "AuthenticAMD", 12) != 0;

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
        nativeCpuId(cpui.data(), i);
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

    if (f_1_EDX[23])
        pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_MMX);

    if (f_1_EDX[25])
        pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_SSE);

    if (f_1_EDX[26])
        pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_SSE2);

    if (isAmd && f_81_EDX[31])
        pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_3DNOW);

    if (f_1_ECX[0])
        pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_SSE3);

    if (f_1_ECX[9])
        pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_SSSE3);

    if (f_1_ECX[19])
        pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_SSE41);

    if (f_1_ECX[20])
        pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_SSE42);

    nativeCpuId(cpui.data(), 1);

    const bool hasMWait = (cpui[2] & 0x8) > 0;
    if (hasMWait) pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_MWAIT);

    pinfo->family   = (cpui[0] >> 8) & 0xf;
    pinfo->model    = (cpui[0] >> 4) & 0xf;
    pinfo->stepping = cpui[0] & 0xf;

    // Calculate available processors
#ifdef WINDOWS
    unsigned int pa_mask_save, sa_mask_stub = 0;
    GetProcessAffinityMask( GetCurrentProcess() , &pa_mask_save , &sa_mask_stub );
#elif defined(LINUX)
    unsigned int pa_mask_save = 0;
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    sched_getaffinity(0, sizeof(cpu_set_t), &my_set);
    pa_mask_save = CPU_COUNT(&my_set);
#else
#warning "No Function to obtain process affinity"
    unsigned int pa_mask_save = 0;
#endif

    int cpu_cores, processorCoreCount = 0;
#ifdef WINDOWS
    SYSTEM_INFO systeminfo;
    GetSystemInfo(&systeminfo);
    cpu_cores = systeminfo.dwNumberOfProcessors;
#elif defined(LINUX) // POSIX
    cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
#else
    #warning Get cpu_cores not implemented
    cpu_cores = 1;
#endif // WINDOWS

    if (pinfo->features |= static_cast<u32>(CpuFeature::FEATURE_HT))
    {
        processorCoreCount = cpu_cores / 2;
    }
    else
    {
        processorCoreCount = cpu_cores;
    }

    // All logical processors
    pinfo->n_threads = std::thread::hardware_concurrency();
    pinfo->affinity_mask = pa_mask_save;
    pinfo->n_cores = processorCoreCount;

    return pinfo->features;
}
#endif // !_EDITOR