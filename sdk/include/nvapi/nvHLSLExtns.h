 /************************************************************************************************************************************\
|*                                                                                                                                    *|
|*     Copyright © 2012 NVIDIA Corporation.  All rights reserved.                                                                     *|
|*                                                                                                                                    *|
|*  NOTICE TO USER:                                                                                                                   *|
|*                                                                                                                                    *|
|*  This software is subject to NVIDIA ownership rights under U.S. and international Copyright laws.                                  *|
|*                                                                                                                                    *|
|*  This software and the information contained herein are PROPRIETARY and CONFIDENTIAL to NVIDIA                                     *|
|*  and are being provided solely under the terms and conditions of an NVIDIA software license agreement.                             *|
|*  Otherwise, you have no rights to use or access this software in any manner.                                                       *|
|*                                                                                                                                    *|
|*  If not covered by the applicable NVIDIA software license agreement:                                                               *|
|*  NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOFTWARE FOR ANY PURPOSE.                                            *|
|*  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.                                                           *|
|*  NVIDIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,                                                                     *|
|*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.                       *|
|*  IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,                               *|
|*  OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT,                         *|
|*  NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.            *|
|*                                                                                                                                    *|
|*  U.S. Government End Users.                                                                                                        *|
|*  This software is a "commercial item" as that term is defined at 48 C.F.R. 2.101 (OCT 1995),                                       *|
|*  consisting  of "commercial computer  software"  and "commercial computer software documentation"                                  *|
|*  as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995) and is provided to the U.S. Government only as a commercial end item.     *|
|*  Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through 227.7202-4 (JUNE 1995),                                          *|
|*  all U.S. Government End Users acquire the software with only those rights set forth herein.                                       *|
|*                                                                                                                                    *|
|*  Any use of this software in individual and commercial software must include,                                                      *|
|*  in the user documentation and internal comments to the code,                                                                      *|
|*  the above Disclaimer (as applicable) and U.S. Government End Users Notice.                                                        *|
|*                                                                                                                                    *|
 \************************************************************************************************************************************/

////////////////////////// NVIDIA SHADER EXTENSIONS /////////////////

// this file is to be #included in the app HLSL shader code to make
// use of nvidia shader extensions


#include "nvHLSLExtnsInternal.h"

//----------------------------------------------------------------------------//
//------------------------- Warp Shuffle Functions ---------------------------//
//----------------------------------------------------------------------------//

// all functions have variants with width parameter which permits sub-division 
// of the warp into segments - for example to exchange data between 4 groups of 
// 8 lanes in a SIMD manner. If width is less than warpSize then each subsection 
// of the warp behaves as a separate entity with a starting logical lane ID of 0. 
// A thread may only exchange data with others in its own subsection. Width must 
// have a value which is a power of 2 so that the warp can be subdivided equally; 
// results are undefined if width is not a power of 2, or is a number greater 
// than warpSize.

//
// simple variant of SHFL instruction
// returns val from the specified lane
// optional width parameter must be a power of two and width <= 32
// 
int NvShfl(int val, uint srcLane, int width = NV_WARP_SIZE)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;                             // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  srcLane;                         // source lane
    g_NvidiaExt[index].src0u.z  =  __NvGetShflMaskFromWidth(width);
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL;
    
    // result is returned as the return value of IncrementCounter on fake UAV slot
    return g_NvidiaExt.IncrementCounter();
}

//
// Copy from a lane with lower ID relative to caller
//
int NvShflUp(int val, uint delta, int width = NV_WARP_SIZE)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;                        // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  delta;                      // relative lane offset
    g_NvidiaExt[index].src0u.z  =  (NV_WARP_SIZE - width) << 8;   // minIndex = maxIndex for shfl_up (src2[4:0] is expected to be 0)
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL_UP;
    return g_NvidiaExt.IncrementCounter();
}

//
// Copy from a lane with higher ID relative to caller
//
int NvShflDown(int val, uint delta, int width = NV_WARP_SIZE)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;           // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  delta;         // relative lane offset
    g_NvidiaExt[index].src0u.z  =  __NvGetShflMaskFromWidth(width);
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL_DOWN;
    return g_NvidiaExt.IncrementCounter();
}

//
// Copy from a lane based on bitwise XOR of own lane ID
//
int NvShflXor(int val, uint laneMask, int width = NV_WARP_SIZE)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;           // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  laneMask;      // laneMask to be XOR'ed with current laneId to get the source lane id
    g_NvidiaExt[index].src0u.z  =  __NvGetShflMaskFromWidth(width); 
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL_XOR;
    return g_NvidiaExt.IncrementCounter();
}


//----------------------------------------------------------------------------//
//----------------------------- Warp Vote Functions---------------------------//
//----------------------------------------------------------------------------//

// returns 0xFFFFFFFF if the predicate is true for any thread in the warp, returns 0 otherwise
uint NvAny(int predicate)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  predicate;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_VOTE_ANY;
    return g_NvidiaExt.IncrementCounter();
}

// returns 0xFFFFFFFF if the predicate is true for ALL threads in the warp, returns 0 otherwise
uint NvAll(int predicate)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  predicate;
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_VOTE_ALL;
    return g_NvidiaExt.IncrementCounter();
}

// returns a mask of all threads in the warp with bits set for threads that have predicate true
uint NvBallot(int predicate)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  predicate;
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_VOTE_BALLOT;
    return g_NvidiaExt.IncrementCounter();
}


//----------------------------------------------------------------------------//
//----------------------------- Utility Functions ----------------------------//
//----------------------------------------------------------------------------//

// returns the lane index of the current thread (thread index in warp)
int NvGetLaneId()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_GET_LANE_ID;
    return g_NvidiaExt.IncrementCounter();
}


//----------------------------------------------------------------------------//
//----------------------------- FP16 Atmoic Functions-------------------------//
//----------------------------------------------------------------------------//

// The functions below performs atomic operations on two consecutive fp16 
// values in the given raw UAV. 
// The uint paramater 'fp16x2Val' is treated as two fp16 values byteAddress must be multiple of 4
// The returned value are the two fp16 values packed into a single uint

uint NvInterlockedAddFp16x2(RWByteAddressBuffer uav, uint byteAddress, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWByteAddressBuffer uav, uint byteAddress, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWByteAddressBuffer uav, uint byteAddress, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, fp16x2Val, NV_EXTN_ATOM_MAX);
}


// versions of the above functions taking two fp32 values (internally converted to fp16 values)
uint NvInterlockedAddFp16x2(RWByteAddressBuffer uav, uint byteAddress, float2 val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWByteAddressBuffer uav, uint byteAddress, float2 val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWByteAddressBuffer uav, uint byteAddress, float2 val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MAX);
}


//----------------------------------------------------------------------------//

// The functions below perform atomic operation on a R16G16_FLOAT UAV at the given address
// the uint paramater 'fp16x2Val' is treated as two fp16 values
// the returned value are the two fp16 values (.x and .y components) packed into a single uint
// Warning: Behaviour of these set of functions is undefined if the UAV is not 
// of R16G16_FLOAT format (might result in app crash or TDR)

uint NvInterlockedAddFp16x2(RWTexture1D<float2> uav, uint address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture1D<float2> uav, uint address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture1D<float2> uav, uint address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

uint NvInterlockedAddFp16x2(RWTexture2D<float2> uav, uint2 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture2D<float2> uav, uint2 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture2D<float2> uav, uint2 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

uint NvInterlockedAddFp16x2(RWTexture3D<float2> uav, uint3 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture3D<float2> uav, uint3 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture3D<float2> uav, uint3 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}


// versions taking two fp32 values (internally converted to fp16)
uint NvInterlockedAddFp16x2(RWTexture1D<float2> uav, uint address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture1D<float2> uav, uint address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture1D<float2> uav, uint address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MAX);
}

uint NvInterlockedAddFp16x2(RWTexture2D<float2> uav, uint2 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture2D<float2> uav, uint2 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture2D<float2> uav, uint2 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MAX);
}

uint NvInterlockedAddFp16x2(RWTexture3D<float2> uav, uint3 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture3D<float2> uav, uint3 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture3D<float2> uav, uint3 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MAX);
}


//----------------------------------------------------------------------------//

// The functions below perform Atomic operation on a R16G16B16A16_FLOAT UAV at the given address
// the uint2 paramater 'fp16x2Val' is treated as four fp16 values 
// i.e, fp16x2Val.x = uav.xy and fp16x2Val.y = uav.yz
// The returned value are the four fp16 values (.xyzw components) packed into uint2
// Warning: Behaviour of these set of functions is undefined if the UAV is not 
// of R16G16B16A16_FLOAT format (might result in app crash or TDR)

uint2 NvInterlockedAddFp16x4(RWTexture1D<float4> uav, uint address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture1D<float4> uav, uint address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture1D<float4> uav, uint address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedAddFp16x4(RWTexture2D<float4> uav, uint2 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture2D<float4> uav, uint2 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture2D<float4> uav, uint2 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedAddFp16x4(RWTexture3D<float4> uav, uint3 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture3D<float4> uav, uint3 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture3D<float4> uav, uint3 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

// versions taking four fp32 values (internally converted to fp16)
uint2 NvInterlockedAddFp16x4(RWTexture1D<float4> uav, uint address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture1D<float4> uav, uint address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture1D<float4> uav, uint address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedAddFp16x4(RWTexture2D<float4> uav, uint2 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture2D<float4> uav, uint2 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture2D<float4> uav, uint2 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedAddFp16x4(RWTexture3D<float4> uav, uint3 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture3D<float4> uav, uint3 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture3D<float4> uav, uint3 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MAX);
}


//----------------------------------------------------------------------------//
//----------------------------- FP32 Atmoic Functions-------------------------//
//----------------------------------------------------------------------------//

// The functions below performs atomic add on the given UAV treating the value as float
// byteAddress must be multiple of 4
// The returned value is the value present in memory location before the atomic add

float NvInterlockedAddFp32(RWByteAddressBuffer uav, uint byteAddress, float val)
{
    return __NvAtomicAddFP32(uav, byteAddress, val);
}

//----------------------------------------------------------------------------//

// The functions below perform atomic add on a R32_FLOAT UAV at the given address
// the returned value is the value before performing the atomic add
// Warning: Behaviour of these set of functions is undefined if the UAV is not 
// of R32_FLOAT format (might result in app crash or TDR)

float NvInterlockedAddFp32(RWTexture1D<float> uav, uint address, float val)
{
    return __NvAtomicAddFP32(uav, address, val);
}

float NvInterlockedAddFp32(RWTexture2D<float> uav, uint2 address, float val)
{
    return __NvAtomicAddFP32(uav, address, val);
}

float NvInterlockedAddFp32(RWTexture3D<float> uav, uint3 address, float val)
{
    return __NvAtomicAddFP32(uav, address, val);
}


//----------------------------------------------------------------------------//
//--------------------------- UINT64 Atmoic Functions-------------------------//
//----------------------------------------------------------------------------//

// The functions below performs atomic operation on the given UAV treating the value as uint64
// byteAddress must be multiple of 8
// The returned value is the value present in memory location before the atomic operation
// uint2 vector type is used to represent a single uint64 value with the x component containing the low 32 bits and y component the high 32 bits.

uint2 NvInterlockedAddUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMaxUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedMinUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedAndUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_AND);
}

uint2 NvInterlockedOrUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_OR);
}

uint2 NvInterlockedXorUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_XOR);
}

uint2 NvInterlockedCompareExchangeUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 compare_value, uint2 value)
{
    return __NvAtomicCompareExchangeUINT64(uav, byteAddress, compare_value, value);
}

uint2 NvInterlockedExchangeUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_SWAP);
}

//----------------------------------------------------------------------------//

// The functions below perform atomic operation on a R32G32_UINT UAV at the given address treating the value as uint64
// the returned value is the value before performing the atomic operation
// uint2 vector type is used to represent a single uint64 value with the x component containing the low 32 bits and y component the high 32 bits.
// Warning: Behaviour of these set of functions is undefined if the UAV is not of R32G32_UINT format (might result in app crash or TDR)

uint2 NvInterlockedAddUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMaxUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedMinUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedAndUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_AND);
}

uint2 NvInterlockedOrUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_OR);
}

uint2 NvInterlockedXorUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_XOR);
}

uint2 NvInterlockedCompareExchangeUint64(RWTexture1D<uint2> uav, uint address, uint2 compare_value, uint2 value)
{
    return __NvAtomicCompareExchangeUINT64(uav, address, compare_value, value);
}

uint2 NvInterlockedExchangeUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_SWAP);
}

uint2 NvInterlockedAddUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMaxUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedMinUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedAndUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_AND);
}

uint2 NvInterlockedOrUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_OR);
}

uint2 NvInterlockedXorUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_XOR);
}

uint2 NvInterlockedCompareExchangeUint64(RWTexture2D<uint2> uav, uint2 address, uint2 compare_value, uint2 value)
{
    return __NvAtomicCompareExchangeUINT64(uav, address, compare_value, value);
}

uint2 NvInterlockedExchangeUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_SWAP);
}

uint2 NvInterlockedAddUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMaxUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedMinUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedAndUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_AND);
}

uint2 NvInterlockedOrUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_OR);
}

uint2 NvInterlockedXorUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_XOR);
}

uint2 NvInterlockedCompareExchangeUint64(RWTexture3D<uint2> uav, uint3 address, uint2 compare_value, uint2 value)
{
    return __NvAtomicCompareExchangeUINT64(uav, address, compare_value, value);
}

uint2 NvInterlockedExchangeUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_SWAP);
}

//----------------------------------------------------------------------------//
//--------------------------- VPRS functions ---------------------------------//
//----------------------------------------------------------------------------//

// Returns the shading rate and the number of per-pixel shading passes for current VPRS pixel
uint3 NvGetShadingRate()
{
    uint3 shadingRate = (uint3)0;
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_GET_SHADING_RATE;
    g_NvidiaExt[index].numOutputsForIncCounter = 3;
    shadingRate.x = g_NvidiaExt.IncrementCounter();
    shadingRate.y = g_NvidiaExt.IncrementCounter();
    shadingRate.z = g_NvidiaExt.IncrementCounter();
    return shadingRate;
}

float NvEvaluateAttributeAtSampleForVPRS(float attrib, uint sampleIndex, int2 pixelOffset)
{
    float value = (float)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float2 NvEvaluateAttributeAtSampleForVPRS(float2 attrib, uint sampleIndex, int2 pixelOffset)
{
    float2 value = (float2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float3 NvEvaluateAttributeAtSampleForVPRS(float3 attrib, uint sampleIndex, int2 pixelOffset)
{
    float3 value = (float3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    value.z = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float4 NvEvaluateAttributeAtSampleForVPRS(float4 attrib, uint sampleIndex, int2 pixelOffset)
{
    float4 value = (float4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    value.z = asfloat(g_NvidiaExt.IncrementCounter());
    value.w = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

int NvEvaluateAttributeAtSampleForVPRS(int attrib, uint sampleIndex, int2 pixelOffset)
{
    int value = (int)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int2 NvEvaluateAttributeAtSampleForVPRS(int2 attrib, uint sampleIndex, int2 pixelOffset)
{
    int2 value = (int2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int3 NvEvaluateAttributeAtSampleForVPRS(int3 attrib, uint sampleIndex, int2 pixelOffset)
{
    int3 value = (int3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    value.z = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int4 NvEvaluateAttributeAtSampleForVPRS(int4 attrib, uint sampleIndex, int2 pixelOffset)
{
    int4 value = (int4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    value.z = asint(g_NvidiaExt.IncrementCounter());
    value.w = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint NvEvaluateAttributeAtSampleForVPRS(uint attrib, uint sampleIndex, int2 pixelOffset)
{
    uint value = (uint)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint2 NvEvaluateAttributeAtSampleForVPRS(uint2 attrib, uint sampleIndex, int2 pixelOffset)
{
    uint2 value = (uint2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint3 NvEvaluateAttributeAtSampleForVPRS(uint3 attrib, uint sampleIndex, int2 pixelOffset)
{
    uint3 value = (uint3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    value.z = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint4 NvEvaluateAttributeAtSampleForVPRS(uint4 attrib, uint sampleIndex, int2 pixelOffset)
{
    uint4 value = (uint4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    value.z = asuint(g_NvidiaExt.IncrementCounter());
    value.w = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}


float NvEvaluateAttributeSnappedForVPRS(float attrib, uint2 offset)
{
    float value = (float)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float2 NvEvaluateAttributeSnappedForVPRS(float2 attrib, uint2 offset)
{
    float2 value = (float2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float3 NvEvaluateAttributeSnappedForVPRS(float3 attrib, uint2 offset)
{
    float3 value = (float3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    value.z = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float4 NvEvaluateAttributeSnappedForVPRS(float4 attrib, uint2 offset)
{
    float4 value = (float4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    value.z = asfloat(g_NvidiaExt.IncrementCounter());
    value.w = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

int NvEvaluateAttributeSnappedForVPRS(int attrib, uint2 offset)
{
    int value = (int)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int2 NvEvaluateAttributeSnappedForVPRS(int2 attrib, uint2 offset)
{
    int2 value = (int2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int3 NvEvaluateAttributeSnappedForVPRS(int3 attrib, uint2 offset)
{
    int3 value = (int3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    value.z = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int4 NvEvaluateAttributeSnappedForVPRS(int4 attrib, uint2 offset)
{
    int4 value = (int4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    value.z = asint(g_NvidiaExt.IncrementCounter());
    value.w = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint NvEvaluateAttributeSnappedForVPRS(uint attrib, uint2 offset)
{
    uint value = (uint)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint2 NvEvaluateAttributeSnappedForVPRS(uint2 attrib, uint2 offset)
{
    uint2 value = (uint2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint3 NvEvaluateAttributeSnappedForVPRS(uint3 attrib, uint2 offset)
{
    uint3 value = (uint3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    value.z = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint4 NvEvaluateAttributeSnappedForVPRS(uint4 attrib, uint2 offset)
{
    uint4 value = (uint4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    value.z = asuint(g_NvidiaExt.IncrementCounter());
    value.w = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}