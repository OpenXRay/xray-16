#pragma once

#include "_flags.h"

enum class CpuFeature : u32
{
    MMX             = 1u << 0u,
    _3DNow          = 1u << 1u,
    AltiVec         = 1u << 2u,

    SSE             = 1u << 3u,
    SSE2            = 1u << 4u,
    SSE3            = 1u << 5u,
    SSSE3           = 1u << 6u,
    SSE41           = 1u << 7u,
    SSE42           = 1u << 8u,

    AVX             = 1u << 9u,
    AVX2            = 1u << 10u,

    MWait           = 1u << 11u,
    HyperThreading  = 1u << 12u,
    InvariantTSC    = 1u << 13u,
};

struct processor_info
{
    string32 vendor; // vendor name
    string64 modelName; // Name of model eg. Intel_Pentium_Pro

    u8 family; // family of the processor, eg. Intel_Pentium_Pro is family 6 processor
    u8 model; // model of processor, eg. Intel_Pentium_Pro is model 1 of family 6 processor
    u8 stepping; // Processor revision number

    Flags32 features; // processor Feature (same as return value).

    u32 n_cores; // number of available physical cores
    u32 n_threads; // number of available logical threads

    u32 affinity_mask; // recommended affinity mask
    // all processors available to process
    // except 2nd (and upper) logical threads
    // of the same physical core

    bool hasFeature(CpuFeature feature) const XR_NOEXCEPT
    {
        return features.test(static_cast<u32>(feature));
    }
};

bool query_processor_info(processor_info*);

