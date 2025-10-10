#define SM_5_0
#include "common.h"

struct TestData
{
    uint value;
    float x;
    float y;
    float z;
};

StructuredBuffer<TestData> g_input : register(t0);

RWStructuredBuffer<TestData> g_output : register(u0);

RWStructuredBuffer<uint> g_counter : register(u1);

cbuffer TestParams : register(b0)
{
    uint g_element_count;
    float g_multiplier;
    float g_pad0;
    float g_pad1;
};

[numthreads(256, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint idx = dispatch_thread_id.x;

    if (idx >= g_element_count)
        return;

    TestData input_data = g_input[idx];

    TestData output_data;
    output_data.value = input_data.value * 2 + idx;
    output_data.x = input_data.x * g_multiplier;
    output_data.y = input_data.y * g_multiplier;
    output_data.z = input_data.z * g_multiplier;

    g_output[idx] = output_data;

    InterlockedAdd(g_counter[0], 1);
}
