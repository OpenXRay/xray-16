#include "stdafx.h"
#pragma hdrstop

#include "cpuid.h"

#include <array>
#include <bitset>
#include <memory>

#ifdef _EDITOR
unsgined int query_processor_info(processor_info* pinfo)
{
    ZeroMemory(pinfo, sizeof(processor_info));

    pinfo->feature = static_cast<u32>(CpuFeature::Mmx) | static_cast<u32>(CpuFeature::Sse);
    return pinfo->feature;
}
#else

#undef _CPUID_DEBUG

#if defined(WINDOWS)
DWORD countSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = static_cast<ULONG_PTR>(1) << LSHIFT;
    DWORD i;

    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}
#endif

unsigned int query_processor_info(processor_info* pinfo)
{
    ZeroMemory(pinfo, sizeof(processor_info));

#if defined(WINDOWS)
    std::bitset<32> f_1_ECX;
    std::bitset<32> f_1_EDX;
    /*std::bitset<32> f_7_EBX;
    std::bitset<32> f_7_ECX;
    std::bitset<32> f_81_ECX;*/
    std::bitset<32> f_81_EDX;

    xr_vector<std::array<int, 4>> data;
    std::array<int, 4> cpui;

    __cpuid(cpui.data(), 0);
    const int nIds = cpui[0];

    for (int i = 0; i <= nIds; ++i)
    {
        __cpuidex(cpui.data(), i, 0);
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

    __cpuid(cpui.data(), 0x80000000);
    const int nExIds_ = cpui[0];
    data.clear();

    for (int i = 0x80000000; i <= nExIds_; ++i)
    {
        __cpuidex(cpui.data(), i, 0);
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

    if (f_1_EDX[23]) pinfo->features |= static_cast<u32>(CpuFeature::Mmx);
    if (f_1_EDX[25]) pinfo->features |= static_cast<u32>(CpuFeature::Sse);
    if (f_1_EDX[26]) pinfo->features |= static_cast<u32>(CpuFeature::Sse2);
    if (isAmd && f_81_EDX[31]) pinfo->features |= static_cast<u32>(CpuFeature::_3dNow);

    if (f_1_ECX[0]) pinfo->features |= static_cast<u32>(CpuFeature::Sse3);
    if (f_1_ECX[9]) pinfo->features |= static_cast<u32>(CpuFeature::Ssse3);
    if (f_1_ECX[19]) pinfo->features |= static_cast<u32>(CpuFeature::Sse41);
    if (f_1_ECX[20]) pinfo->features |= static_cast<u32>(CpuFeature::Sse42);

    __cpuid(cpui.data(), 1);

    const bool hasMWait = (cpui[2] & 0x8) > 0;
    if (hasMWait) pinfo->features |= static_cast<u32>(CpuFeature::MWait);

    pinfo->family = (cpui[0] >> 8) & 0xf;
    pinfo->model = (cpui[0] >> 4) & 0xf;
    pinfo->stepping = cpui[0] & 0xf;

    // Calculate available processors
    ULONG_PTR pa_mask_save, sa_mask_stub = 0;
    GetProcessAffinityMask(GetCurrentProcess(), &pa_mask_save, &sa_mask_stub);

    DWORD returnedLength = 0;
    DWORD byteOffset = 0;
    GetLogicalProcessorInformation(nullptr, &returnedLength);

    auto buffer = std::make_unique<u8[]>(returnedLength);
    auto ptr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(buffer.get());
    GetLogicalProcessorInformation(ptr, &returnedLength);

    auto processorCoreCount = 0u;
    auto logicalProcessorCount = 0u;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnedLength)
    {
        switch (ptr->Relationship)
        {
            case RelationProcessorCore:
                processorCoreCount++;

                // A hyperthreaded core supplies more than one logical processor.
                logicalProcessorCount += countSetBits(ptr->ProcessorMask);
                break;

            default:
                break;
        }

        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }

    if (logicalProcessorCount != processorCoreCount) pinfo->features |= static_cast<u32>(CpuFeature::HT);

    // All logical processors
    pinfo->n_threads = logicalProcessorCount;
    pinfo->affinity_mask = pa_mask_save;
    pinfo->n_cores = processorCoreCount;
#endif

    return pinfo->features;
}
#endif
