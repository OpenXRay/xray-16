#ifndef _INC_CPUID
#define _INC_CPUID

#ifdef WINDOWS
#include <intrin.h>
#else // POSIX
#include <unistd.h>
#include <cpuid.h>
#define _GNU_SOURCE
#include <sched.h>
#endif // WINDOWS
#include <thread>

enum class CpuFeature : u32
{
    FEATURE_MMX = 0x0001,
    FEATURE_SSE = 0x0002,
    FEATURE_SSE2 = 0x0004,
    FEATURE_3DNOW = 0x0008,
    FEATURE_SSE3 = 0x0010,
    FEATURE_SSSE3 = 0x0020,
    FEATURE_SSE41 = 0x0040,
    FEATURE_SSE42 = 0x0080,
    FEATURE_MWAIT = 0x1000,
    FEATURE_HT = 0x0200
};

struct processor_info {
    string32 vendor; // Vendor name
    string64 modelName; // Name of model

    unsigned char family; // Family of the processor
    unsigned char model; // Model of processor
    unsigned char stepping; // Processor revision number

    unsigned int features; // Processor Features

    unsigned int n_cores; // Number of available physical cores
    unsigned int n_threads; // Number of available logical threads

    unsigned int affinity_mask; // Recommended affinity mask

    bool hasFeature(const CpuFeature feature) const noexcept
    {
        return features & static_cast<u32>(feature);
    }
};

unsigned int query_processor_info(processor_info*);

#endif // _INC_CPUID
